// Most of the MiniFFI class was taken from Ruby 1.8's Win32API.c,
// it's just as basic but should work fine for the moment

#include <ruby.h>
#include <SDL.h>
#if defined(__WIN32__) && defined(USE_ESSENTIALS_FIXES)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "sharedstate.h"
#endif

#define _T_VOID     0
#define _T_NUMBER   1
#define _T_POINTER  2
#define _T_INTEGER  3

#ifdef __WIN32__
typedef void* (__stdcall *MINIFFI_FUNC)(...);
#else
typedef void* (__cdecl *MINIFFI_FUNC)(...);
#endif

// MiniFFI class, also named Win32API on Windows
// Uses LoadLibrary/GetProcAddress on Windows, dlopen/dlsym everywhere else

static VALUE
MiniFFI_alloc(VALUE self)
{
    return Data_Wrap_Struct(self, 0, SDL_UnloadObject, 0);
}

static VALUE
MiniFFI_initialize(VALUE self, VALUE libname, VALUE func, VALUE imports, VALUE exports)
{
    SafeStringValue(libname);
    SafeStringValue(func);
    
    
    void *hlib = SDL_LoadObject(RSTRING_PTR(libname));
    if (!hlib)
    rb_raise(rb_eRuntimeError, SDL_GetError());
    DATA_PTR(self) = hlib;
    
    void *hfunc = SDL_LoadFunction(hlib, RSTRING_PTR(func));
#ifdef __WIN32__
    if (!hfunc)
    {
        VALUE func_a = rb_str_new3(func);
        func_a = rb_str_cat(func_a, "A", 1);
        hfunc = SDL_LoadFunction(hlib, RSTRING_PTR(func_a));
    }
#endif
    if (!hfunc)
    rb_raise(rb_eRuntimeError, SDL_GetError());
    
    
    rb_iv_set(self, "_func", OFFT2NUM((unsigned long)hfunc));
    rb_iv_set(self, "_funcname", func);
    rb_iv_set(self, "_libname", libname);
    
    VALUE ary_imports = rb_ary_new();
    VALUE *entry = RARRAY_PTR(imports);
    switch (TYPE(imports))
    {
            case T_NIL:
            break;
            case T_ARRAY:
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
            }
        }
            break;
    }
    
    if (16 < RARRAY_LEN(ary_imports))
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
        }
    }
    rb_iv_set(self, "_exports", INT2FIX(ex));
    return Qnil;
}

static VALUE
MiniFFI_call(int argc, VALUE *argv, VALUE self)
{
    struct {
        unsigned long params[16];
    } param;
#define params param.params
    VALUE func = rb_iv_get(self, "_func");
    VALUE own_imports = rb_iv_get(self, "_imports");
    VALUE own_exports = rb_iv_get(self, "_exports");
    MINIFFI_FUNC ApiFunction = (MINIFFI_FUNC)NUM2OFFT(func);
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
                lParam = NUM2OFFT(str);
            }
                else
            {
                StringValue(str);
                rb_str_modify(str);
                lParam = (unsigned long)RSTRING_PTR(str);
            }
                break;
                
                case _T_NUMBER: case _T_INTEGER: default:
                lParam = NUM2OFFT(rb_ary_entry(args, i));
                break;
        }
        params[i] = lParam;
    }
    
    unsigned long ret;
#if defined(__WIN32__) && defined(USE_ESSENTIALS_FIXES)
    // On Windows, if essentials fixes are enabled, function calls that
    // do not work with MKXP will be intercepted here so that the code
    // still has its desired effect
	
	// TODO: Move these to actual functions and just redirect the
	//       function pointers during the initialization stage
	//       so that we're not going through ugly if statements
	//       a million times a second in Essentials
    
    // GetCurrentThreadId, GetWindowThreadProcessId, FindWindowEx,
    // and GetForegroundWindow are used for determining whether to
    // handle input and for positioning
    
    // It's a super janky system, but I must abide by it
    
    char *fname = RSTRING_PTR(rb_iv_get(self, "_funcname"));
#define func_is(x) !strcmp(fname, x)
#define if_func_is(x) if (func_is(x))
    if (func_is("GetCurrentThreadId") || func_is("GetWindowThreadProcessId"))
    {
        ret = 571; // Dummy
    }
    else if_func_is("FindWindowEx")
    {
        ret = 571;
    }
    else if_func_is("GetForegroundWindow")
    {
        if (SDL_GetWindowFlags(shState->sdlWindow()) & SDL_WINDOW_INPUT_FOCUS)
        {
            ret = 571;
        }
        else
        ret = 0;
    }
    
    // Mouse support
	// FIXME: It worked before but I've broken it somehow,
	//        but on the plus side everything else works
    
    else if_func_is("GetCursorPos")
    {
        int *output = (int*)params[0];
        int x, y;
        SDL_GetGlobalMouseState(&x, &y);
        output[0] = x;
        output[1] = y;
        ret = true;
    }
    else if_func_is("ScreenToClient")
    {
        int *output = (int*)params[1];
        int x, y;
        SDL_GetMouseState(&x, &y);
        output[0] = x;
        output[1] = y;
        ret = true;
    }
	
	// Window stuff
	
    else if_func_is("SetWindowPos")
    {
		// Win32API.restoreScreen calls SetWindowPos with +6 to width
		// and +29 to height, I should be fine if I account for this
		// I think
        SDL_SetWindowSize(shState->sdlWindow(),params[4]-6,params[5]-29);
        SDL_SetWindowPosition(shState->sdlWindow(),params[2],params[3]);
        return true;
    }
	else if_func_is("RegisterHotKey") // Don't disable SDL's fullscreen,
	{								  // it works fine
		ret = 0;
	}
    else
    {
        ret = (unsigned long)ApiFunction(param);
    }
    
#else
    ret = (unsigned long)ApiFunction(param);
#endif
    switch (FIX2INT(own_exports))
    {
            case _T_NUMBER: case _T_INTEGER:
            return OFFT2NUM(ret);
            
            case _T_POINTER:
            return rb_str_new2((char*)ret);
            
            case _T_VOID: default:
            return OFFT2NUM(0);
    }
}


void
MiniFFIBindingInit()
{
    VALUE cMiniFFI = rb_define_class("MiniFFI", rb_cObject);
    rb_define_alloc_func(cMiniFFI, MiniFFI_alloc);
    rb_define_method(cMiniFFI, "initialize", RUBY_METHOD_FUNC(MiniFFI_initialize), 4);
    rb_define_method(cMiniFFI, "call", RUBY_METHOD_FUNC(MiniFFI_call), -1);
    rb_define_alias(cMiniFFI, "Call", "call");
    
#ifdef __WIN32__
    rb_define_const(rb_cObject, "Win32API", cMiniFFI);
#endif
}
