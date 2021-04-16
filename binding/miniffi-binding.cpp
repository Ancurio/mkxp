// Most of the MiniFFI class was taken from Ruby 1.8's Win32API.c,
// it's just as basic but should work fine for the moment

#include "system/fake-api.h"
#include <SDL.h>
#include <cstdint>

#include "filesystem/filesystem.h"
#include "miniffi.h"
#include "binding-util.h"

#if defined(__linux__) || defined(__APPLE__)
#define MVAL2RB(v) ULONG2NUM(v)
#define RB2MVAL(v) (mffi_value)NUM2ULONG(v)
#else
#ifdef __MINGW64__
#define MVAL2RB(v) ULL2NUM(v)
#define RB2MVAL(v) (mffi_value)NUM2ULL(v)
#else
#define MVAL2RB(v) UINT2NUM(v)
#define RB2MVAL(v) (mffi_value)NUM2UINT(v)
#endif
#endif

#define _T_VOID 0
#define _T_NUMBER 1
#define _T_POINTER 2
#define _T_INTEGER 3
#define _T_BOOL 4

#if RAPI_FULL > 187
DEF_TYPE_CUSTOMFREE(MiniFFI, SDL_UnloadObject);
#else
DEF_ALLOCFUNC_CUSTOMFREE(MiniFFI, SDL_UnloadObject);
#endif

static void *MiniFFI_GetFunctionHandle(void *libhandle, const char *func) {
#ifdef MKXPZ_ESSENTIALS_DEBUG
#define CAPTURE(n)                                                             \
    if (!strcmp(#n, func))                                                       \
        return (void *)&MKXP_##n
        CAPTURE(GetCurrentThreadId);
    CAPTURE(GetWindowThreadProcessId);
    CAPTURE(FindWindowEx);
    CAPTURE(GetForegroundWindow);
    CAPTURE(GetClientRect);
    CAPTURE(GetCursorPos);
    CAPTURE(ScreenToClient);
    CAPTURE(SetWindowPos);
    CAPTURE(SetWindowTextA);
    CAPTURE(GetWindowRect);
    CAPTURE(GetKeyboardState);
#ifndef __WIN32__
    // Functions only needed on Linux and macOS go here
    CAPTURE(RtlMoveMemory);
    CAPTURE(LoadLibrary);
    CAPTURE(FreeLibrary);
    CAPTURE(GetAsyncKeyState);
    CAPTURE(GetSystemPowerStatus);
    CAPTURE(ShowWindow);
    CAPTURE(GetSystemMetrics);
    CAPTURE(SetCapture);
    CAPTURE(ReleaseCapture);
    CAPTURE(ShowCursor);
    CAPTURE(GetPrivateProfileString);
    CAPTURE(GetUserDefaultLangID);
    CAPTURE(GetUserName);
    CAPTURE(RegisterHotKey);
    CAPTURE(SetWindowLong);
#endif
#endif
    if (!libhandle)
        return 0;
    return SDL_LoadFunction(libhandle, func);
}

// MiniFFI.new(library, function[, imports[, exports]])
// Yields itself in blocks

RB_METHOD(MiniFFI_initialize) {
    VALUE libname, func, imports, exports;
    rb_scan_args(argc, argv, "22", &libname, &func, &imports, &exports);
    SafeStringValue(libname);
    SafeStringValue(func);
#ifdef __MACOSX__
    void *hlib = SDL_LoadObject(mkxp_fs::normalizePath(RSTRING_PTR(libname), 1, 1).c_str());
#else
    void *hlib = SDL_LoadObject(RSTRING_PTR(libname));
#endif
    setPrivateData(self, hlib);
    void *hfunc = MiniFFI_GetFunctionHandle(hlib, RSTRING_PTR(func));
#ifdef __WIN32__
    if (hlib && !hfunc) {
        VALUE func_a = rb_str_new3(func);
        func_a = rb_str_cat(func_a, "A", 1);
        hfunc = SDL_LoadFunction(hlib, RSTRING_PTR(func_a));
    }
#endif
    if (!hfunc)
        rb_raise(rb_eRuntimeError, "%s", SDL_GetError());
    
    rb_iv_set(self, "_func", MVAL2RB((mffi_value)hfunc));
    rb_iv_set(self, "_funcname", func);
    rb_iv_set(self, "_libname", libname);
    
    VALUE ary_imports = rb_ary_new();
    VALUE *entry;
    switch (TYPE(imports)) {
        case T_NIL:
            break;
        case T_ARRAY:
            entry = RARRAY_PTR(imports);
            for (int i = 0; i < RARRAY_LEN(imports); i++) {
                SafeStringValue(entry[i]);
                switch (*(char *)RSTRING_PTR(entry[i])) {
                    case 'N':
                    case 'n':
                    case 'L':
                    case 'l':
                        rb_ary_push(ary_imports, INT2FIX(_T_NUMBER));
                        break;
                        
                    case 'P':
                    case 'p':
                        rb_ary_push(ary_imports, INT2FIX(_T_POINTER));
                        break;
                        
                    case 'I':
                    case 'i':
                        rb_ary_push(ary_imports, INT2FIX(_T_INTEGER));
                        break;
                        
                    case 'B':
                    case 'b':
                        rb_ary_push(ary_imports, INT2FIX(_T_BOOL));
                        break;
                }
            }
            break;
        default:
            SafeStringValue(imports);
            const char *s = RSTRING_PTR(imports);
            for (int i = 0; i < RSTRING_LEN(imports); i++) {
                switch (*s++) {
                    case 'N':
                    case 'n':
                    case 'L':
                    case 'l':
                        rb_ary_push(ary_imports, INT2FIX(_T_NUMBER));
                        break;
                        
                    case 'P':
                    case 'p':
                        rb_ary_push(ary_imports, INT2FIX(_T_POINTER));
                        break;
                        
                    case 'I':
                    case 'i':
                        rb_ary_push(ary_imports, INT2FIX(_T_INTEGER));
                        break;
                        
                    case 'B':
                    case 'b':
                        rb_ary_push(ary_imports, INT2FIX(_T_BOOL));
                        break;
                }
            }
            break;
    }
    
    if (MINIFFI_MAX_ARGS < RARRAY_LEN(ary_imports))
        rb_raise(rb_eRuntimeError, "too many parameters: %ld/%ld\n",
                 RARRAY_LEN(ary_imports), MINIFFI_MAX_ARGS);
    
    rb_iv_set(self, "_imports", ary_imports);
    int ex;
    if (NIL_P(exports)) {
        ex = _T_VOID;
    } else {
        SafeStringValue(exports);
        switch (*RSTRING_PTR(exports)) {
            case 'V':
            case 'v':
                ex = _T_VOID;
                break;
                
            case 'N':
            case 'n':
            case 'L':
            case 'l':
                ex = _T_NUMBER;
                break;
                
            case 'P':
            case 'p':
                ex = _T_POINTER;
                break;
                
            case 'I':
            case 'i':
                ex = _T_INTEGER;
                break;
                
            case 'B':
            case 'b':
                ex = _T_BOOL;
                break;
        }
    }
    rb_iv_set(self, "_exports", INT2FIX(ex));
    if (rb_block_given_p())
        rb_yield(self);
    return Qnil;
}

RB_METHOD(MiniFFI_call) {
    MiniFFIFuncArgs param;
#define params param.params
    VALUE func = rb_iv_get(self, "_func");
    VALUE own_imports = rb_iv_get(self, "_imports");
    VALUE own_exports = rb_iv_get(self, "_exports");
    MINIFFI_FUNC ApiFunction = (MINIFFI_FUNC)RB2MVAL(func);
    VALUE args;
    int items = rb_scan_args(argc, argv, "0*", &args);
    int nimport = RARRAY_LEN(own_imports);
    if (items != nimport)
        rb_raise(rb_eRuntimeError,
                 "wrong number of parameters: expected %d, got %d", nimport, items);
    
    for (int i = 0; i < nimport; i++) {
        VALUE str = rb_ary_entry(args, i);
        mffi_value lParam = 0;
        switch (FIX2INT(rb_ary_entry(own_imports, i))) {
            case _T_POINTER:
                if (NIL_P(str)) {
                    lParam = 0;
                } else if (FIXNUM_P(str)) {
                    lParam = RB2MVAL(str);
                } else {
                    StringValue(str);
                    rb_str_modify(str);
                    lParam = (mffi_value)RSTRING_PTR(str);
                }
                break;
                
            case _T_BOOL:
                lParam = RTEST(rb_ary_entry(args, i));
                break;
                
            case _T_INTEGER:
#if INTPTR_MAX == INT64_MAX
                lParam = RB2MVAL(rb_ary_entry(args, i)) & UINT32_MAX;
                break;
#endif
            case _T_NUMBER:
            default:
                lParam = RB2MVAL(rb_ary_entry(args, i));
                break;
        }
        params[i] = lParam;
    }
    mffi_value ret = miniffi_call_intern(ApiFunction, &param, nimport);
    
    switch (FIX2INT(own_exports)) {
        case _T_NUMBER:
        case _T_INTEGER:
            return MVAL2RB(ret);
            
        case _T_POINTER:
            return rb_utf8_str_new_cstr((char *)ret);
            
        case _T_BOOL:
            return rb_bool_new(ret);
            
        case _T_VOID:
        default:
            return MVAL2RB(0);
    }
}

void MiniFFIBindingInit() {
    VALUE cMiniFFI = rb_define_class("MiniFFI", rb_cObject);
#if RAPI_FULL > 187
    rb_define_alloc_func(cMiniFFI, classAllocate<&MiniFFIType>);
#else
    rb_define_alloc_func(cMiniFFI, MiniFFIAllocate);
#endif
    _rb_define_method(cMiniFFI, "initialize", MiniFFI_initialize);
    _rb_define_method(cMiniFFI, "call", MiniFFI_call);
    rb_define_alias(cMiniFFI, "Call", "call");
    
    rb_define_const(rb_cObject, "Win32API", cMiniFFI);
}
