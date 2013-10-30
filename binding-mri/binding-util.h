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

#include "./ruby/ruby.h"

enum RbException
{
	RGSS = 0,
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

	RbData();
	~RbData();
};

RbData *getRbData();

struct Exception;

void
raiseRbExc(const Exception &exc);

#define DECL_TYPE(Klass) \
	extern rb_data_type_struct Klass##Type

#define DEF_TYPE(Klass) \
	rb_data_type_struct Klass##Type

void initType(rb_data_type_struct &type,
              const char *name,
              void (*freeInst)(void*));

template<rb_data_type_t *rbType>
static VALUE classAllocate(VALUE klass)
{
	return rb_data_typed_object_alloc(klass, 0, rbType);
}

template<class C>
static void freeInstance(void *inst)
{
	delete static_cast<C*>(inst);
}

#define INIT_TYPE(Klass) initType(Klass##Type, #Klass, freeInstance<Klass>)

template<class C>
static inline C *
getPrivateData(VALUE self)
{
	return static_cast<C*>(RTYPEDDATA_DATA(self));
}

template<class C>
static inline C *
getPrivateDataCheck(VALUE self, const rb_data_type_struct &type)
{
	void *obj = rb_check_typeddata(self, &type);
	return static_cast<C*>(obj);
}

static inline void
setPrivateData(VALUE self, void *p)
{
	RTYPEDDATA_DATA(self) = p;
}

inline VALUE
wrapObject(void *p, const rb_data_type_struct &type)
{
	VALUE klass = rb_const_get(rb_cObject, rb_intern(type.wrap_struct_name));
	VALUE obj = rb_obj_alloc(klass);

	setPrivateData(obj, p);

	return obj;
}

inline VALUE
wrapProperty(VALUE self, void *prop, const char *iv,
             const rb_data_type_struct &type)
{
	VALUE propObj = wrapObject(prop, type);

	rb_iv_set(self, iv, propObj);

	return propObj;
}

inline void
wrapNilProperty(VALUE self, const char *iv)
{
	rb_iv_set(self, iv, Qnil);
}

/* Implemented: oSszfib| */
int
rb_get_args(int argc, VALUE *argv, const char *format, ...);

/* Always terminate 'rb_get_args' with this */
#define RB_ARG_END ((void*) -1)

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
	rb_get_args(argc, argv, "s", &data, &dataLen, RB_ARG_END);

	VALUE obj = rb_obj_alloc(self);

	C *c;

	GUARD_EXC( c = C::deserialize(data, dataLen); );

	setPrivateData(obj, c);

	return obj;
}

static inline VALUE
rb_bool_new(bool value)
{
	return value ? Qtrue : Qfalse;
}

#define RB_METHOD(name) \
	static VALUE name(int argc, VALUE *argv, VALUE self)

#define RB_UNUSED_PARAM \
	{ (void) argc; (void) argv; (void) self; }

#define MARSH_LOAD_FUN(Typ) \
	RB_METHOD(Typ##Load) \
	{ \
		return objectLoad<Typ>(argc, argv, self); \
	}

#define CLONE_FUNC(Klass) \
	static mrb_value \
	Klass##Clone(mrb_state *mrb, mrb_value self) \
	{ \
		Klass *k = getPrivateData<Klass>(mrb, self); \
		mrb_value dupObj = mrb_obj_clone(mrb, self); \
		Klass *dupK = new Klass(*k); \
		setPrivateData(mrb, dupObj, dupK, Klass##Type); \
		return dupObj; \
	}

#define CLONE_FUN(Klass) \
	RB_METHOD(Klass##Clone) \
	{ \
		RB_UNUSED_PARAM \
		Klass *k = getPrivateData<Klass>(self); \
		VALUE dupObj = rb_obj_clone(self); \
		Klass *dupK = new Klass(*k); \
		setPrivateData(dupObj, dupK); \
		return dupObj; \
	}

/* If we're not binding a disposable class,
 * we want to #undef DEF_PROP_CHK_DISP */
#define DEF_PROP_CHK_DISP \
	checkDisposed(k, DISP_CLASS_NAME);

#define DEF_PROP_OBJ(Klass, PropKlass, PropName, prop_iv) \
	RB_METHOD(Klass##Get##PropName) \
	{ \
		RB_UNUSED_PARAM; \
		Klass *k = getPrivateData<Klass>(self); (void) k; \
		DEF_PROP_CHK_DISP \
		return rb_iv_get(self, prop_iv); \
	} \
	RB_METHOD(Klass##Set##PropName) \
	{ \
		Klass *k = getPrivateData<Klass>(self); \
		VALUE propObj; \
		PropKlass *prop; \
		rb_get_args(argc, argv, "o", &propObj, RB_ARG_END); \
		prop = getPrivateDataCheck<PropKlass>(propObj, PropKlass##Type); \
		GUARD_EXC( k->set##PropName(prop); ) \
		rb_iv_set(self, prop_iv, propObj); \
		return propObj; \
	}

/* Object property with allowed NIL */
#define DEF_PROP_OBJ_NIL(Klass, PropKlass, PropName, prop_iv) \
	RB_METHOD(Klass##Get##PropName) \
	{ \
		RB_UNUSED_PARAM; \
		Klass *k = getPrivateData<Klass>(self); (void) k; \
		DEF_PROP_CHK_DISP \
		return rb_iv_get(self, prop_iv); \
	} \
	RB_METHOD(Klass##Set##PropName) \
	{ \
		RB_UNUSED_PARAM; \
		Klass *k = getPrivateData<Klass>(self); \
		VALUE propObj; \
		PropKlass *prop; \
		rb_get_args(argc, argv, "o", &propObj, RB_ARG_END); \
		if (rb_type(propObj) == RUBY_T_NIL) \
			prop = 0; \
		else \
			prop = getPrivateDataCheck<PropKlass>(propObj, PropKlass##Type); \
		GUARD_EXC( k->set##PropName(prop); ) \
		rb_iv_set(self, prop_iv, propObj); \
		return propObj; \
	}

#define DEF_PROP(Klass, type, PropName, param_t_s, value_fun) \
	RB_METHOD(Klass##Get##PropName) \
	{ \
		RB_UNUSED_PARAM; \
		Klass *k = getPrivateData<Klass>(self); \
		DEF_PROP_CHK_DISP \
		return value_fun(k->get##PropName()); \
	} \
	RB_METHOD(Klass##Set##PropName) \
	{ \
		Klass *k = getPrivateData<Klass>(self); \
		type value; \
		rb_get_args(argc, argv, param_t_s, &value, RB_ARG_END); \
		GUARD_EXC( k->set##PropName(value); ) \
		return value_fun(value); \
	}

#define DEF_PROP_I(Klass, PropName) \
	DEF_PROP(Klass, int, PropName, "i", rb_fix_new)

#define DEF_PROP_F(Klass, PropName) \
	DEF_PROP(Klass, double, PropName, "f", rb_float_new)

#define DEF_PROP_B(Klass, PropName) \
	DEF_PROP(Klass, bool, PropName, "b", rb_bool_new)

#define INIT_PROP_BIND(Klass, PropName, prop_name_s) \
{ \
	_rb_define_method(klass, prop_name_s, Klass##Get##PropName); \
	_rb_define_method(klass, prop_name_s "=", Klass##Set##PropName); \
}


#endif // BINDING_UTIL_H
