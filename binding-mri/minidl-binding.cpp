// Most of this was taken from Ruby 1.8's Win32API.c,
// it's just as basic but should work fine for the moment

#include <ruby/ruby.h>

#ifdef __WIN32__

#include <windows.h>
#define LIBHANDLE HINSTANCE
#define FUNCHANDLE HANDLE

#else

#include <dlfcn.h>
#define LIBHANDLE void*
#define FUNCHANDLE void*

#endif

#define _T_VOID     0
#define _T_NUMBER   1
#define _T_POINTER  2
#define _T_INTEGER  3

typedef void* (*MINIDL_FUNC)(...);

static void
dl_freelib(LIBHANDLE lib)
{
#ifdef __WIN32__
    FreeLibrary(lib);
#else
    dlclose(lib);
#endif
}

static LIBHANDLE
dl_loadlib(const char *filename)
{
    LIBHANDLE ret;
#ifdef __WIN32__
    ret = LoadLibrary(filename);
#else
    ret = dlopen(filename, RTLD_NOW);
#endif
    return ret;
}

static FUNCHANDLE
dl_getfunc(LIBHANDLE lib, const char *filename)
{
    FUNCHANDLE ret;
#ifdef __WIN32__
    ret = (FUNCHANDLE)GetProcAddress(lib, filename);
#else
    ret = dlsym(lib, filename);
#endif
    return ret;
}

static VALUE
MiniDL_alloc(VALUE self)
{
    return Data_Wrap_Struct(self, 0, dl_freelib, 0);
}

static VALUE
MiniDL_initialize(VALUE self, VALUE libname, VALUE func, VALUE imports, VALUE exports)
{
    SafeStringValue(libname);
    SafeStringValue(func);


    LIBHANDLE hlib = dl_loadlib(RSTRING_PTR(libname));
    if (!hlib)
        rb_raise(rb_eRuntimeError, "Failed to load library %s", RSTRING_PTR(libname));
    DATA_PTR(self) = hlib;

    FUNCHANDLE hfunc = dl_getfunc(hlib, RSTRING_PTR(func));
#ifdef __WIN32__
    if (!hfunc)
        {
            VALUE func_a = rb_str_new3(func);
            func_a = rb_str_cat(func_a, "A", 1);
            hfunc = dl_getfunc(hlib, RSTRING_PTR(func_a));
        }
#endif
    if (!hfunc)
        rb_raise(rb_eRuntimeError, "Failed to find function %s within %s", RSTRING_PTR(func), RSTRING_PTR(libname));
    

    rb_iv_set(self, "_func", OFFT2NUM((unsigned long)hfunc));

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
MiniDL_call(int argc, VALUE *argv, VALUE self)
{
    struct {
        unsigned long params[16];
    } param;
#define params param.params
    VALUE func = rb_iv_get(self, "_func");
    VALUE own_imports = rb_iv_get(self, "_imports");
    VALUE own_exports = rb_iv_get(self, "_exports");
    MINIDL_FUNC ApiFunction = (MINIDL_FUNC)NUM2OFFT(func);
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
    
    unsigned long ret = (unsigned long)ApiFunction(param);
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
MiniDLBindingInit()
{
    VALUE cMiniDL = rb_define_class("MiniDL", rb_cObject);
    rb_define_alloc_func(cMiniDL, MiniDL_alloc);
    rb_define_method(cMiniDL, "initialize", RUBY_METHOD_FUNC(MiniDL_initialize), 4);
    rb_define_method(cMiniDL, "call", RUBY_METHOD_FUNC(MiniDL_call), -1);
    rb_define_alias(cMiniDL, "Call", "call");
#ifdef __WIN32__
    rb_define_const(rb_cObject, "Win32API", cMiniDL);
#endif
}
