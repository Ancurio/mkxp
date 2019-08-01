/*
** binding-util.h
**
** This file is part of mkxp.
**
** Copyright (C) 2013 Jonas Kulla <Nyocurio@gmail.com>
**
** mkxp is free software: you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation, either version 2 of the License, or
** (at your option) any later version.
**
** mkxp is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with mkxp.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef BINDING_UTIL_H
#define BINDING_UTIL_H

#ifndef OLD_RUBY
#include <ruby.h>
#else
#include <ruby/ruby.h>
#endif

#include "exception.h"

enum RbException
{
	RGSS = 0,
	Reset,
	PHYSFS,
	SDL,
	MKXP,

	ErrnoENOENT,

	IOError,

	TypeError,
	ArgumentError,

	RbExceptionsMax
};

struct RbData
{
	VALUE exc[RbExceptionsMax];

	/* Input module (RGSS3) */
	VALUE buttoncodeHash;

	RbData();
	~RbData();
};

RbData *getRbData();

struct Exception;

void
raiseRbExc(const Exception &exc);

#ifndef OLD_RUBY
#define DECL_TYPE(Klass) \
	extern rb_data_type_t Klass##Type
#endif

/* 2.1 has added a new field (flags) to rb_data_type_t */
#include <ruby/version.h>
#if RUBY_API_VERSION_MAJOR >= 2 && RUBY_API_VERSION_MINOR >= 1
/* TODO: can mkxp use RUBY_TYPED_FREE_IMMEDIATELY here? */
#define DEF_TYPE_FLAGS 0
#else
#define DEF_TYPE_FLAGS
#endif

#ifndef OLD_RUBY
#define DEF_TYPE_CUSTOMNAME_AND_FREE(Klass, Name, Free) \
	rb_data_type_t Klass##Type = { \
		Name, { 0, Free, 0, { 0, 0 } }, 0, 0, DEF_TYPE_FLAGS \
	}

#define DEF_TYPE_CUSTOMFREE(Klass, Free) \
	DEF_TYPE_CUSTOMNAME_AND_FREE(Klass, #Klass, Free)

#define DEF_TYPE_CUSTOMNAME(Klass, Name) \
	DEF_TYPE_CUSTOMNAME_AND_FREE(Klass, Name, freeInstance<Klass>)

#define DEF_TYPE(Klass) DEF_TYPE_CUSTOMNAME(Klass, #Klass)
#endif

// Ruby 1.8 helper stuff
#ifdef OLD_RUBY

#define RUBY_T_FIXNUM T_FIXNUM
#define RUBY_T_TRUE T_TRUE
#define RUBY_T_FALSE T_FALSE
#define RUBY_T_NIL T_NIL
#define RUBY_T_UNDEF T_UNDEF
#define RUBY_T_SYMBOL T_SYMBOL
#define RUBY_T_FLOAT T_FLOAT
#define RUBY_T_STRING T_STRING
#define RUBY_T_ARRAY T_ARRAY

#define RUBY_Qtrue Qtrue
#define RUBY_Qfalse Qfalse
#define RUBY_Qnil Qnil
#define RUBY_Qundef Qundef

#define RB_FIXNUM_P(obj) FIXNUM_P(obj)
#define RB_SYMBOL_P(obj) SYMBOL_P(obj)

#define RB_TYPE_P(obj, type) ( \
((type) == RUBY_T_FIXNUM) ? RB_FIXNUM_P(obj) : \
((type) == RUBY_T_TRUE) ? ((obj) == RUBY_Qtrue) : \
((type) == RUBY_T_FALSE) ? ((obj) == RUBY_Qfalse) : \
((type) == RUBY_T_NIL) ? ((obj) == RUBY_Qnil) : \
((type) == RUBY_T_UNDEF) ? ((obj) == RUBY_Qundef) : \
((type) == RUBY_T_SYMBOL) ? RB_SYMBOL_P(obj) : \
(!SPECIAL_CONST_P(obj) && BUILTIN_TYPE(obj) == (type)))

#define OBJ_INIT_COPY(a,b) rb_obj_init_copy(a,b)

#define DEF_ALLOCFUNC_CUSTOMFREE(type,free) \
static VALUE type##Allocate(VALUE klass)\
{ \
    void *sval = 0; \
    return Data_Wrap_Struct(klass, 0, free, sval); \
}

#define DEF_ALLOCFUNC(type) DEF_ALLOCFUNC_CUSTOMFREE(type, freeInstance<type>)

#define rb_str_new_cstr rb_str_new2
#define PRIsVALUE "s"

#endif
// end

#ifndef OLD_RUBY
template<rb_data_type_t *rbType>
static VALUE classAllocate(VALUE klass)
{
/* 2.3 has changed the name of this function */
#if RUBY_API_VERSION_MAJOR >= 2 && RUBY_API_VERSION_MINOR >= 3
	return rb_data_typed_object_wrap(klass, 0, rbType);
#else
	return rb_data_typed_object_alloc(klass, 0, rbType);
#endif
}
#endif

template<class C>
static void freeInstance(void *inst)
{
	delete static_cast<C*>(inst);
}

void
raiseDisposedAccess(VALUE self);

template<class C>
inline C *
getPrivateData(VALUE self)
{
#ifndef OLD_RUBY
	C *c = static_cast<C*>(RTYPEDDATA_DATA(self));
#else
    C *c = static_cast<C*>(DATA_PTR(self));
#endif
	return c;
}

template<class C>
static inline C *
#ifndef OLD_RUBY
getPrivateDataCheck(VALUE self, const rb_data_type_t &type)
#else
getPrivateDataCheck(VALUE self, const char *type)
#endif
{
#ifndef OLD_RUBY
    void *obj = Check_TypedStruct(self, &type);
#else // RGSS1 works in a more permissive way than the above,
	  // See the function at ram:10012AB0 in RGSS104E for an example
	  // This isn't an exact replica, but should have pretty much the same result
    rb_check_type(self, T_DATA);
    VALUE otherObj = rb_const_get(rb_cObject, rb_intern(type));
	const char *ownname, *othername;
	if (!rb_obj_is_kind_of(self, otherObj))
	{
		ownname = rb_obj_classname(self);
		othername = rb_obj_classname(otherObj);
		rb_raise(rb_eTypeError, "Can't convert %s into %s", othername, ownname);
	}
    void *obj = DATA_PTR(self);
        
#endif
	return static_cast<C*>(obj);
}

static inline void
setPrivateData(VALUE self, void *p)
{
#ifndef OLD_RUBY
	RTYPEDDATA_DATA(self) = p;
#else
    DATA_PTR(self) = p;
#endif
}

inline VALUE
#ifndef OLD_RUBY
wrapObject(void *p, const rb_data_type_t &type,
           VALUE underKlass = rb_cObject)
#else
wrapObject(void *p, const char *type, VALUE underKlass = rb_cObject)
#endif
{
#ifndef OLD_RUBY
	VALUE klass = rb_const_get(underKlass, rb_intern(type.wrap_struct_name));
#else
    VALUE klass = rb_const_get(underKlass, rb_intern(type));
#endif
	VALUE obj = rb_obj_alloc(klass);

	setPrivateData(obj, p);

	return obj;
}

inline VALUE
wrapProperty(VALUE self, void *prop, const char *iv,
#ifndef OLD_RUBY
             const rb_data_type_t &type,
#else
             const char *type,
#endif
             VALUE underKlass = rb_cObject)
{
	VALUE propObj = wrapObject(prop, type, underKlass);

	rb_iv_set(self, iv, propObj);

	return propObj;
}

/* Implemented: oSszfibn| */
int
rb_get_args(int argc, VALUE *argv, const char *format, ...);

/* Always terminate 'rb_get_args' with this */
#ifndef NDEBUG
#  define RB_ARG_END_VAL ((void*) -1)
#  define RB_ARG_END ,RB_ARG_END_VAL
#else
#  define RB_ARG_END
#endif

typedef VALUE (*RubyMethod)(int argc, VALUE *argv, VALUE self);

static inline void
_rb_define_method(VALUE klass, const char *name, RubyMethod func)
{
	rb_define_method(klass, name, RUBY_METHOD_FUNC(func), -1);
}

static inline void
rb_define_class_method(VALUE klass, const char *name, RubyMethod func)
{
	rb_define_singleton_method(klass, name, RUBY_METHOD_FUNC(func), -1);
}

static inline void
_rb_define_module_function(VALUE module, const char *name, RubyMethod func)
{
	rb_define_module_function(module, name, RUBY_METHOD_FUNC(func), -1);
}

#define GUARD_EXC(exp) \
{ try { exp } catch (const Exception &exc) { raiseRbExc(exc); } }

template<class C>
static inline VALUE
objectLoad(int argc, VALUE *argv, VALUE self)
{
	const char *data;
	int dataLen;
	rb_get_args(argc, argv, "s", &data, &dataLen RB_ARG_END);

	VALUE obj = rb_obj_alloc(self);

	C *c = 0;

	GUARD_EXC( c = C::deserialize(data, dataLen); );

	setPrivateData(obj, c);

	return obj;
}

static inline VALUE
rb_bool_new(bool value)
{
	return value ? Qtrue : Qfalse;
}

inline void
rb_float_arg(VALUE arg, double *out, int argPos = 0)
{
	switch (rb_type(arg))
	{
	case RUBY_T_FLOAT :
		*out = RFLOAT_VALUE(arg);
		break;

	case RUBY_T_FIXNUM :
		*out = FIX2INT(arg);
		break;

	default:
		rb_raise(rb_eTypeError, "Argument %d: Expected float", argPos);
	}
}

inline void
rb_int_arg(VALUE arg, int *out, int argPos = 0)
{
	switch (rb_type(arg))
	{
	case RUBY_T_FLOAT :
		// FIXME check int range?
		*out = NUM2LONG(arg);
		break;

	case RUBY_T_FIXNUM :
		*out = FIX2INT(arg);
		break;

	default:
		rb_raise(rb_eTypeError, "Argument %d: Expected fixnum", argPos);
	}
}

inline void
rb_bool_arg(VALUE arg, bool *out, int argPos = 0)
{
	switch (rb_type(arg))
	{
	case RUBY_T_TRUE :
		*out = true;
		break;

	case RUBY_T_FALSE :
	case RUBY_T_NIL :
		*out = false;
		break;

	default:
		rb_raise(rb_eTypeError, "Argument %d: Expected bool", argPos);
	}
}

inline void
rb_check_argc(int actual, int expected)
{
	if (actual != expected)
		rb_raise(rb_eArgError, "wrong number of arguments (%d for %d)",
		         actual, expected);
}

#ifdef OLD_RUBY
static inline void
rb_error_arity(int argc, int min, int max)
{
    if (argc > max || argc < min)
        rb_raise(rb_eArgError, "Finish me! rb_error_arity()"); //TODO
}

static inline VALUE
rb_sprintf(const char *fmt, ...)
{
    return rb_str_new2("Finish me! rb_sprintf()"); //TODO
}

static inline VALUE
rb_str_catf(VALUE obj, const char *fmt, ...)
{
    return rb_str_new2("Finish me! rb_str_catf()"); //TODO
}

static inline VALUE
rb_file_open_str(VALUE filename, const char *mode)
{
    VALUE fileobj = rb_const_get(rb_cObject, rb_intern("File"));
    return rb_funcall(fileobj, rb_intern("open"), 2, filename, rb_str_new2(mode));
}
#endif

#define RB_METHOD(name) \
	static VALUE name(int argc, VALUE *argv, VALUE self)

#define RB_UNUSED_PARAM \
	{ (void) argc; (void) argv; (void) self; }

#define MARSH_LOAD_FUN(Typ) \
	RB_METHOD(Typ##Load) \
	{ \
		return objectLoad<Typ>(argc, argv, self); \
	}

#define INITCOPY_FUN(Klass) \
	RB_METHOD(Klass##InitializeCopy) \
	{ \
		VALUE origObj; \
		rb_get_args(argc, argv, "o", &origObj RB_ARG_END); \
		if (!OBJ_INIT_COPY(self, origObj)) /* When would this fail??*/\
			return self; \
		Klass *orig = getPrivateData<Klass>(origObj); \
		Klass *k = 0; \
		GUARD_EXC( k = new Klass(*orig); ) \
		setPrivateData(self, k); \
		return self; \
	}

/* Object property which is copied by reference, with allowed NIL
 * FIXME: Getter assumes prop is disposable,
 * because self.disposed? is not checked in this case.
 * Should make this more clear */
#ifndef OLD_RUBY
#define DEF_PROP_OBJ_REF(Klass, PropKlass, PropName, prop_iv) \
	RB_METHOD(Klass##Get##PropName) \
	{ \
		RB_UNUSED_PARAM; \
		return rb_iv_get(self, prop_iv); \
	} \
	RB_METHOD(Klass##Set##PropName) \
	{ \
		RB_UNUSED_PARAM; \
		rb_check_argc(argc, 1); \
		Klass *k = getPrivateData<Klass>(self); \
		VALUE propObj = *argv; \
		PropKlass *prop; \
		if (NIL_P(propObj)) \
			prop = 0; \
		else \
			prop = getPrivateDataCheck<PropKlass>(propObj, PropKlass##Type); \
		GUARD_EXC( k->set##PropName(prop); ) \
		rb_iv_set(self, prop_iv, propObj); \
		return propObj; \
	}
#else
#define DEF_PROP_OBJ_REF(Klass, PropKlass, PropName, prop_iv) \
    RB_METHOD(Klass##Get##PropName) \
    { \
        RB_UNUSED_PARAM; \
        return rb_iv_get(self, prop_iv); \
    } \
    RB_METHOD(Klass##Set##PropName) \
    { \
        RB_UNUSED_PARAM; \
        rb_check_argc(argc, 1); \
        Klass *k = getPrivateData<Klass>(self); \
        VALUE propObj = *argv; \
        PropKlass *prop; \
        if (NIL_P(propObj)) \
            prop = 0; \
        else \
prop = getPrivateDataCheck<PropKlass>(propObj, #PropKlass); \
        GUARD_EXC( k->set##PropName(prop); ) \
        rb_iv_set(self, prop_iv, propObj); \
        return propObj; \
    }
#endif

/* Object property which is copied by value, not reference */
#ifndef OLD_RUBY
#define DEF_PROP_OBJ_VAL(Klass, PropKlass, PropName, prop_iv) \
	RB_METHOD(Klass##Get##PropName) \
	{ \
		RB_UNUSED_PARAM; \
		checkDisposed<Klass>(self); \
		return rb_iv_get(self, prop_iv); \
	} \
	RB_METHOD(Klass##Set##PropName) \
	{ \
		rb_check_argc(argc, 1); \
		Klass *k = getPrivateData<Klass>(self); \
		VALUE propObj = *argv; \
		PropKlass *prop; \
		prop = getPrivateDataCheck<PropKlass>(propObj, PropKlass##Type); \
		GUARD_EXC( k->set##PropName(*prop); ) \
		return propObj; \
	}
#else
#define DEF_PROP_OBJ_VAL(Klass, PropKlass, PropName, prop_iv) \
    RB_METHOD(Klass##Get##PropName) \
    { \
        RB_UNUSED_PARAM; \
        checkDisposed<Klass>(self); \
        return rb_iv_get(self, prop_iv); \
    } \
    RB_METHOD(Klass##Set##PropName) \
    { \
        rb_check_argc(argc, 1); \
        Klass *k = getPrivateData<Klass>(self); \
        VALUE propObj = *argv; \
        PropKlass *prop; \
        prop = getPrivateDataCheck<PropKlass>(propObj, #PropKlass); \
        GUARD_EXC( k->set##PropName(*prop); ) \
        return propObj; \
    }
#endif

#define DEF_PROP(Klass, type, PropName, arg_fun, value_fun) \
	RB_METHOD(Klass##Get##PropName) \
	{ \
		RB_UNUSED_PARAM; \
		Klass *k = getPrivateData<Klass>(self); \
		type value = 0; \
		GUARD_EXC( value = k->get##PropName(); ) \
		return value_fun(value); \
	} \
	RB_METHOD(Klass##Set##PropName) \
	{ \
		rb_check_argc(argc, 1); \
		Klass *k = getPrivateData<Klass>(self); \
		type value; \
		rb_##arg_fun##_arg(*argv, &value); \
		GUARD_EXC( k->set##PropName(value); ) \
		return *argv; \
	}

#define DEF_PROP_I(Klass, PropName) \
	DEF_PROP(Klass, int, PropName, int, rb_fix_new)

#define DEF_PROP_F(Klass, PropName) \
	DEF_PROP(Klass, double, PropName, float, rb_float_new)

#define DEF_PROP_B(Klass, PropName) \
	DEF_PROP(Klass, bool, PropName, bool, rb_bool_new)

#define INIT_PROP_BIND(Klass, PropName, prop_name_s) \
{ \
	_rb_define_method(klass, prop_name_s, Klass##Get##PropName); \
	_rb_define_method(klass, prop_name_s "=", Klass##Set##PropName); \
}


#endif // BINDING_UTIL_H
