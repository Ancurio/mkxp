#include "miniffi.h"
#include <assert.h>

#if defined(__linux__) || defined(__APPLE__)
mffi_value miniffi_call_intern(MINIFFI_FUNC target, MiniFFIFuncArgs *p, int nparams) {
    assert(nparams <= 8);
    return target(p->params[0], p->params[1], p->params[2], p->params[3],
                                 p->params[4], p->params[5], p->params[6], p->params[7]);
}
#else
#define INTEL_ASM ".intel_syntax noprefix\n"
__attribute__((noinline))
#ifndef __MINGW64__
mffi_value miniffi_call_intern(MINIFFI_FUNC target, MiniFFIFuncArgs *p, int nparams) {
    mffi_value ret;
    void *old_esp = 0;

    asm volatile(INTEL_ASM

                "mov [edi], esp\n"
                "test ebx, ebx\n"
                "jz mffi_call_void\n"
                
                "shl ebx, 2\n"
                "mov ecx, ebx\n"
                
                "mffi_call_loop:\n"
                "sub ecx, 4\n"
                "mov ebx, [esi+ecx]\n"
                "push ebx\n"
                "test ecx, ecx\n"
                "jnz mffi_call_loop\n"
                
                "mffi_call_void:\n"
                "call edx\n"
                
                : "=a"(ret)
                : "b"(nparams), "S"(p), "d"(target), "D"(&old_esp)
                : "ecx"
    );


    // If esp doesn't match, this was probably a cdecl and not a stdcall.
    // Move the stack pointer back to where it should be
    asm volatile(INTEL_ASM
                "mov edx, [edi]\n"
                "cmp edx, esp\n"
                "cmovne esp, edx"
                :
                : "D"(&old_esp)
                : "edx"
    );
    return ret;
}
#else
mffi_value miniffi_call_intern(MINIFFI_FUNC target, MiniFFIFuncArgs *p, int nparams) {
    mffi_value ret;
    asm volatile(INTEL_ASM

                "test rax, rax\n"
                "jz mffi_call_void\n"

                "cmp rax, 4\n"
                "jle mffi_call_low\n"
                
                "shl rax, 3\n"
                 
                "mffi_call_loop:\n"
                "sub rax, 8\n"
                "mov rbx, [rsi+rax]\n"
                "push rbx\n"
                "cmp rax, 32\n"
                "jg mffi_call_loop\n"
                
                "mffi_call_low:\n"
                "mov rcx, [rsi]\n"
                "mov rdx, [rsi+8]\n"
                "mov r8, [rsi+16]\n"
                "mov r9, [rsi+24]\n"

                "mffi_call_void:\n"
                "call rdi\n"
                
                : "=a"(ret)
                : "a"(nparams), "S"(p), "D"(target)
                : "r8", "r9", "rbx", "rcx", "rdx"
    );
    return ret;
}
#endif
#endif
