// Most of the MiniFFI class was taken from Ruby 1.8's Win32API.c,
// it's just as basic but should work fine for the moment

#include <SDL.h>
#ifdef USE_FAKEAPI
#include "fake-api.h"
#endif

#include "binding-util.h"

#define _T_VOID     0
#define _T_NUMBER   1
#define _T_POINTER  2
#define _T_INTEGER  3
#define _T_BOOL     4


// Might need to let MiniFFI.initialize set calling convention
// as an optional arg, this won't work with everything
// Maybe libffi would help out with this

// Only using 8 max args instead of 16 to reduce the time taken
// to set all those variables up and ease the eyesore, and I don't
// think there are many functions one would need to use that require
// that many arguments anyway

// stdcall is easy, everything in a struct gets pushed to the stack
#ifdef __WIN32__
typedef void* (__stdcall *MINIFFI_FUNC)(...);
#else
// L O N G, but variables won't get set up correctly otherwise
// should allow for __fastcalls (macOS likes these) and whatever else
typedef void* (*MINIFFI_FUNC)(unsigned long,unsigned long,unsigned long,unsigned long,unsigned long,unsigned long,unsigned long,unsigned long);
#endif

// MiniFFI class, also named Win32API on Windows
// Uses LoadLibrary/GetProcAddress on Windows, dlopen/dlsym everywhere else

static VALUE
MiniFFI_alloc(VALUE self)
{
    return Data_Wrap_Struct(self, 0, SDL_UnloadObject, 0);
}

static void*
MiniFFI_GetFunctionHandle(void *libhandle, const char *func)
{
#ifdef USE_FAKEAPI
#define CAPTURE(n) if (!strcmp(#n,func)) return (void*)&MKXP_##n
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
    CAPTURE(RegisterHotKey);
    CAPTURE(SetWindowLong);
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
#endif
#endif
    if (!libhandle) return 0;
    return SDL_LoadFunction(libhandle, func);
}

// MiniFFI.new(library, function[, imports[, exports]])
// Yields itself in blocks

RB_METHOD(MiniFFI_initialize)
{
    VALUE libname, func, imports, exports;
    rb_scan_args(argc, argv, "22", &libname, &func, &imports, &exports);
    SafeStringValue(libname);
    SafeStringValue(func);
    void *hlib = SDL_LoadObject(RSTRING_PTR(libname));
    DATA_PTR(self) = hlib;
    void *hfunc = MiniFFI_GetFunctionHandle(hlib, RSTRING_PTR(func));
#ifdef __WIN32__
    if (hlib && !hfunc)
    {
        VALUE func_a = rb_str_new3(func);
        func_a = rb_str_cat(func_a, "A", 1);
        hfunc = SDL_LoadFunction(hlib, RSTRING_PTR(func_a));
    }
#endif
    if (!hfunc)
        rb_raise(rb_eRuntimeError, SDL_GetError());
    
    
    rb_iv_set(self, "_func", ULONG2NUM((unsigned long)hfunc));
    rb_iv_set(self, "_funcname", func);
    rb_iv_set(self, "_libname", libname);
    
    VALUE ary_imports = rb_ary_new();
    VALUE *entry;
    switch (TYPE(imports))
    {
        case T_NIL:
            break;
        case T_ARRAY:
            entry = RARRAY_PTR(imports);
            for (int i = 0; i < RARRAY_LEN(imports); i++)
            {
                SafeStringValue(entry[i]);
                switch (*(char*)RSTRING_PTR(entry[i]))
                {
                    case 'N': case 'n': case 'L': case 'l':
                        rb_ary_push(ary_imports, INT2FIX(_T_NUMBER));
                        break;
                        
                    case 'P': case 'p':
                        rb_ary_push(ary_imports, INT2FIX(_T_POINTER));
                        break;
                        
                    case 'I': case 'i':
                        rb_ary_push(ary_imports, INT2FIX(_T_INTEGER));
                        break;
                    
                    case 'B': case 'b':
                        rb_ary_push(ary_imports, INT2FIX(_T_BOOL));
                        break;
                }
            }
            break;
        default:
            SafeStringValue(imports);
            const char *s = RSTRING_PTR(imports);
            for (int i = 0; i < RSTRING_LEN(imports); i++)
            {
                switch (*s++)
                {
                    case 'N': case 'n': case 'L': case 'l':
                        rb_ary_push(ary_imports, INT2FIX(_T_NUMBER));
                        break;
                        
                    case 'P': case 'p':
                        rb_ary_push(ary_imports, INT2FIX(_T_POINTER));
                        break;
                        
                    case 'I': case 'i':
                        rb_ary_push(ary_imports, INT2FIX(_T_INTEGER));
                        break;
                    
                    case 'B': case 'b':
                        rb_ary_push(ary_imports, INT2FIX(_T_BOOL));
                        break;
                }
            }
            break;
    }
    
    if (8 < RARRAY_LEN(ary_imports))
        rb_raise(rb_eRuntimeError, "too many parameters: %ld\n", RARRAY_LEN(ary_imports));
    
    rb_iv_set(self, "_imports", ary_imports);
    int ex;
    if (NIL_P(exports))
    {
        ex = _T_VOID;
    }
    else
    {
        SafeStringValue(exports);
        switch(*RSTRING_PTR(exports))
        {
            case 'V': case 'v':
                ex = _T_VOID;
                break;
                
            case 'N': case 'n': case 'L': case 'l':
                ex = _T_NUMBER;
                break;
                
            case 'P': case 'p':
                ex = _T_POINTER;
                break;
                
            case 'I': case 'i':
                ex = _T_INTEGER;
                break;
            
            case 'B': case 'b':
                ex = _T_BOOL;
                break;
        }
    }
    rb_iv_set(self, "_exports", INT2FIX(ex));
    if (rb_block_given_p()) rb_yield(self);
    return Qnil;
}

RB_METHOD(MiniFFI_call)
{
    struct {
        unsigned long params[8];
    } param;
#define params param.params
    VALUE func = rb_iv_get(self, "_func");
    VALUE own_imports = rb_iv_get(self, "_imports");
    VALUE own_exports = rb_iv_get(self, "_exports");
    MINIFFI_FUNC ApiFunction = (MINIFFI_FUNC)NUM2ULONG(func);
    VALUE args;
    int items = rb_scan_args(argc, argv, "0*", &args);
    int nimport = RARRAY_LEN(own_imports);
    if (items != nimport)
        rb_raise(rb_eRuntimeError, "wrong number of parameters: expected %d, got %d",
                 nimport, items);
    
    for (int i = 0; i < nimport; i++)
    {
        VALUE str = rb_ary_entry(args, i);
        unsigned long lParam = 0;
        switch(FIX2INT(rb_ary_entry(own_imports, i)))
        {
            case _T_POINTER:
                if (NIL_P(str))
                {
                    lParam = 0;
                }
                else if (FIXNUM_P(str))
                {
                    lParam = NUM2ULONG(str);
                }
                else
                {
                    StringValue(str);
                    rb_str_modify(str);
                    lParam = (unsigned long)RSTRING_PTR(str);
                }
                break;
            
            case _T_BOOL:
                lParam = (rb_ary_entry(args, i) == Qtrue);
                break;
                
            case _T_NUMBER: case _T_INTEGER: default:
                lParam = NUM2ULONG(rb_ary_entry(args, i));
                break;
        }
        params[i] = lParam;
    }
#ifdef __WIN32__
    unsigned long ret = (unsigned long)ApiFunction(param);
#else
    unsigned long ret = (unsigned long)ApiFunction(params[0],params[1],params[2],params[3],params[4],params[5],params[6],params[7]);
#endif
    
    switch (FIX2INT(own_exports))
    {
        case _T_NUMBER: case _T_INTEGER:
            return ULONG2NUM(ret);
            
        case _T_POINTER:
            return rb_str_new2((char*)ret);
        
        case _T_BOOL:
            return rb_bool_new(ret);
            
        case _T_VOID: default:
            return ULONG2NUM(0);
    }
}


void
MiniFFIBindingInit()
{
    VALUE cMiniFFI = rb_define_class("MiniFFI", rb_cObject);
    rb_define_alloc_func(cMiniFFI, MiniFFI_alloc);
    _rb_define_method(cMiniFFI, "initialize", MiniFFI_initialize);
    _rb_define_method(cMiniFFI, "call", MiniFFI_call);
    rb_define_alias(cMiniFFI, "Call", "call");

    // Preferably use MiniFFI, the name Win32API makes no sense
    // on things that aren't Windows but I'm leaving it here
    // for compatibility
    rb_define_const(rb_cObject, "Win32API", cMiniFFI);
}
