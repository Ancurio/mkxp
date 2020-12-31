#include "miniffi.h"
#include <assert.h>

#if defined(__linux__) || defined(__APPLE__)
mffi_value miniffi_call_intern(MINIFFI_FUNC target, MiniFFIFuncArgs *p, int nparams) {
    assert(nparams <= 8);
    return target(p->params[0], p->params[1], p->params[2], p->params[3],
                                 p->params[4], p->params[5], p->params[6], p->params[7]);
}
#else
mffi_value miniffi_call_intern(MINIFFI_FUNC target, MiniFFIFuncArgs *p, size_t nparams) {
    return call_asm(target, p, nparams);
}

#define INTEL_ASM ".intel_syntax noprefix\n"
#ifndef __MINGW64__
mffi_value call_asm(MINIFFI_FUNC target, MINIFFIFuncArgs *p, size_t nparams) {
    mffi_value ret;
    void *old_esp = 0;

    asm volatile(INTEL_ASM

                "MiniFFI_call_asm:\n"
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
                : "b"(nparams), "S"(params), "d"(target), "D"(&old_esp)
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
mffi_value call_asm(MINIFFI_FUNC target, MINIFFIFuncArgs *p, size_t nparams) {
    mffi_value ret;
    void *old_rsp = 0;
    asm volatile(INTEL_ASM

                "MiniFFI_call_asm:\n"
                "mov [rdi], rsp\n"
                "test rbx, rbx\n"
                "jz mffi_call_void\n"
                
                "shl rbx, 2\n"
                "mov rcx, rbx\n"
                
                "mffi_call_loop:\n"
                "sub rcx, 4\n"
                "mov rbx, [rsi+rcx]\n"
                "push rbx\n"
                "test rcx, rcx\n"
                "jnz mffi_call_loop\n"
                
                "mffi_call_void:\n"
                "call rdx\n"
                
                : "=a"(ret)
                : "b"(nparams), "S"(params), "d"(target), "D"(&old_rsp)
                : "rcx"
    );
    asm volatile(INTEL_ASM
                "mov rdx, [rdi]\n"
                "cmp rdx, rsp\n"
                "cmovne rsp, rdx"
                :
                : "D"(&old_rsp)
                : "rdx"
    );
    return ret;
}
#endif
#endif
