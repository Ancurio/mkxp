#include "miniffi.h"
#include <assert.h>

#if defined(__linux__) || defined(__APPLE__)
mffi_value miniffi_call_intern(MINIFFI_FUNC target, MiniFFIFuncArgs *p, int nparams, bool is64) {
    assert(nparams <= 8);
    return target(p->params[0], p->params[1], p->params[2], p->params[3],
                                 p->params[4], p->params[5], p->params[6], p->params[7]);
}
// It does not actually matter whether the library is 64-bit or not,
// on Linux and macOS we only load the same architecture
// In the future this will probably only check so that it's possible
// to raise an error
bool libIs64(void *handle) {
    return true;
}
#else
#include <windows.h>
mffi_value miniffi_call_intern(MINIFFI_FUNC target, MiniFFIFuncArgs *p, int nparams, bool is64) {
#ifdef __MINGW64__
    if (is64)
        return call_asm64(target, p, nparams);
    return call_asm32(target, p, nparams);
}
#define INTEL_ASM ".intel_syntax noprefix\n"
__attribute__((noinline))
mffi_value call_asm32(MINIFFI_FUNC target, MiniFFIFuncArgs *p, int nparams) {
    mffi_value ret;
    void *old_esp = 0;

    asm volatile(INTEL_ASM

                "mov [edi], esp\n"
                "test ebx, ebx\n"
                "jz mffi_call_void32\n"
                
                "shl ebx, 2\n"
                "mov ecx, ebx\n"
                
                "mffi_call_loop32:\n"
                "sub ecx, 4\n"
                "mov ebx, [esi+ecx]\n"
                "push ebx\n"
                "test ecx, ecx\n"
                "jnz mffi_call_loop32\n"
                
                "mffi_call_void32:\n"
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
#ifdef __MINGW64__
__attribute__((noinline))
mffi_value call_asm64(MINIFFI_FUNC target, MiniFFIFuncArgs *p, int nparams) {
    mffi_value ret;
    void *old_rsp = 0;
    asm volatile(INTEL_ASM

                "mov [rdi], rsp\n"
                "test rbx, rbx\n"
                "jz mffi_call_void64\n"
                
                "shl rbx, 2\n"
                "mov rcx, rbx\n"
                
                "mffi_call_loop64:\n"
                "sub rcx, 4\n"
                "mov rbx, [rsi+rcx]\n"
                "push rbx\n"
                "test rcx, rcx\n"
                "jnz mffi_call_loop64\n"
                
                "mffi_call_void64:\n"
                "call rdx\n"
                
                : "=a"(ret)
                : "b"(nparams), "S"(p), "d"(target), "D"(&old_rsp)
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

bool libIs64(const char *lib) {
    LOADED_IMAGE *img = ImageLoad(lib, NULL);

    // Just assume 32-bit for now
    if (!img)
        return false;
        
    IMAGE_NT_HEADERS *imgheaders = img->FileHeader;
    bool ret = imgheaders->FileHeader->Machine != IMAGE_FILE_MACHINE_I386;
    ImageUnload(img);
    return ret;
}
#endif
