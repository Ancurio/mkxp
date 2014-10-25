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

#ifndef BINDINGUTIL_H
#define BINDINGUTIL_H

#include "exception.h"

#include <mruby.h>
#include <mruby/data.h>
#include <mruby/variable.h>
#include <mruby/class.h>

#include <stdio.h>


enum CommonSymbol
{
	CSfont = 0,
	CSviewport,
	CSbitmap,
	CScolor,
	CSout_color,
	CStone,
	CSrect,
	CSsrc_rect,
	CStilemap,
	CStileset,
	CSautotiles,
	CSmap_data,
	CSflash_data,
	CSpriorities,
	CSwindowskin,
	CScontents,
	CScursor_rect,
	CSpath,
	CSarray,
	CSdefault_color,
	CSdefault_out_color,
	CSchildren,
	CS_mkxp_dispose_alias,

	CommonSymbolsMax
};

enum MrbException
{
	Shutdown = 0,
	RGSS,
	PHYSFS,
	SDL,
	MKXP,

	ErrnoE2BIG,
	ErrnoEACCES,
	ErrnoEAGAIN,
	ErrnoEBADF,
	ErrnoECHILD,
	ErrnoEDEADLOCK,
	ErrnoEDOM,
	ErrnoEEXIST,
	ErrnoEINVAL,
	ErrnoEMFILE,
	ErrnoENOENT,
	ErrnoENOEXEC,
	ErrnoENOMEM,
	ErrnoENOSPC,
	ErrnoERANGE,
	ErrnoEXDEV,

	IO,

	TypeError,
	ArgumentError,

	MrbExceptionsMax
};

struct MrbData
{
	RClass *exc[MrbExceptionsMax];
	/* I'll leave the usage of these syms to later,
	 * so I can measure how much of a speed difference they make */
	mrb_sym symbols[CommonSymbolsMax];

	mrb_value buttoncodeHash;

	MrbData(mrb_state *mrb);
};

struct Exception;

void
raiseMrbExc(mrb_state *mrb, const Exception &exc);

inline MrbData*
getMrbData(mrb_state *mrb)
{
	return static_cast<MrbData*>(mrb->ud);
}

inline RClass*
defineClass(mrb_state *mrb, const char *name)
{
	RClass *klass = mrb_define_class(mrb, name, mrb->object_class);
	MRB_SET_INSTANCE_TT(klass, MRB_TT_DATA);

	return klass;
}

#define GUARD_EXC(exp) \
{ try { exp } catch (Exception &exc) { raiseMrbExc(mrb, exc); } }

#define DECL_TYPE(_Type) \
	extern const mrb_data_type _Type##Type

#define DEF_TYPE(_Type) \
	extern const mrb_data_type _Type##Type = \
	{ \
		#_Type, \
		freeInstance<_Type> \
	}

#define MRB_METHOD_PUB(name) \
	mrb_value name(mrb_state *mrb, mrb_value self)

#define MRB_METHOD(name) static MRB_METHOD_PUB(name)

#define MRB_FUNCTION(name) \
	static mrb_value name(mrb_state *mrb, mrb_value)

#define MRB_UNUSED_PARAM \
	{ (void) mrb; (void) self; }

#define MRB_FUN_UNUSED_PARAM { (void) mrb; }

/* Object property which is copied by value, not reference */
#define DEF_PROP_OBJ_VAL(Klass, PropKlass, PropName, prop_iv) \
	MRB_METHOD(Klass##Get##PropName) \
	{ \
		checkDisposed<Klass>(mrb, self); \
		return getProperty(mrb, self, prop_iv); \
	} \
	MRB_METHOD(Klass##Set##PropName) \
	{ \
		Klass *k = getPrivateData<Klass>(mrb, self); \
		mrb_value propObj; \
		PropKlass *prop; \
		mrb_get_args(mrb, "o", &propObj); \
		prop = getPrivateDataCheck<PropKlass>(mrb, propObj, PropKlass##Type); \
		GUARD_EXC( k->set##PropName(*prop); ) \
		return propObj; \
	}

/* Object property which is copied by reference, with allowed NIL */
#define DEF_PROP_OBJ_REF(Klass, PropKlass, PropName, prop_iv) \
	MRB_METHOD(Klass##Get##PropName) \
	{ \
		return getProperty(mrb, self, prop_iv); \
	} \
	MRB_METHOD(Klass##Set##PropName) \
	{ \
		Klass *k = getPrivateData<Klass>(mrb, self); \
		mrb_value propObj; \
		PropKlass *prop; \
		mrb_get_args(mrb, "o", &propObj); \
		if (mrb_nil_p(propObj)) \
			prop = 0; \
		else \
			prop = getPrivateDataCheck<PropKlass>(mrb, propObj, PropKlass##Type); \
		GUARD_EXC( k->set##PropName(prop); ) \
		setProperty(mrb, self, prop_iv, propObj); \
		return mrb_nil_value(); \
	}

#define DEF_PROP(Klass, mrb_type, PropName, arg_type, conv_t) \
	MRB_METHOD(Klass##Get##PropName) \
	{ \
		Klass *k = getPrivateData<Klass>(mrb, self); \
		mrb_type value = 0; \
		GUARD_EXC( value = k->get##PropName(); ) \
		return mrb_##conv_t##_value(value); \
	} \
	MRB_METHOD(Klass##Set##PropName) \
	{ \
		Klass *k = getPrivateData<Klass>(mrb, self); \
		mrb_type value; \
		mrb_get_args(mrb, arg_type, &value); \
		GUARD_EXC( k->set##PropName(value); ) \
		return mrb_##conv_t##_value(value); \
	}

#define DEF_PROP_I(Klass, PropName) \
	DEF_PROP(Klass, mrb_int, PropName, "i", fixnum)

#define DEF_PROP_F(Klass, PropName) \
	DEF_PROP(Klass, mrb_float, PropName, "f", _float)

#define DEF_PROP_B(Klass, PropName) \
	DEF_PROP(Klass, mrb_bool, PropName, "b", bool)

#define INITCOPY_FUN(Klass) \
	MRB_METHOD(Klass##InitializeCopy) \
	{ \
		mrb_value origObj; \
		mrb_get_args(mrb, "o", &origObj); \
		Klass *orig = getPrivateData<Klass>(mrb, origObj); \
		Klass *k = 0; \
		GUARD_EXC( k = new Klass(*orig); ) \
		setPrivateData(self, k, Klass##Type); \
		return self; \
	}

#define MARSH_LOAD_FUN(Klass) \
	MRB_METHOD(Klass##Load) \
	{ \
		return objectLoad<Klass>(mrb, self, Klass##Type); \
	}

#define INIT_PROP_BIND(Klass, PropName, prop_name_s) \
{ \
	mrb_define_method(mrb, klass, prop_name_s, Klass##Get##PropName, MRB_ARGS_NONE()); \
	mrb_define_method(mrb, klass, prop_name_s "=", Klass##Set##PropName, MRB_ARGS_REQ(1)); \
}

static inline mrb_value
mrb__float_value(mrb_float f)
{
	mrb_value v;

	SET_FLOAT_VALUE(0, v, f);
	return v;
}

inline mrb_sym
getSym(mrb_state *mrb, CommonSymbol sym)
{
	return getMrbData(mrb)->symbols[sym];
}

void
raiseDisposedAccess(mrb_state *mrb, mrb_value self);

template<class C>
inline C *
getPrivateData(mrb_state *, mrb_value self)
{
	C *c = static_cast<C*>(DATA_PTR(self));

	return c;
}

template<typename T>
inline T *
getPrivateDataCheck(mrb_state *mrb, mrb_value obj, const mrb_data_type &type)
{
	void *ptr = mrb_check_datatype(mrb, obj, &type);
	return static_cast<T*>(ptr);
}

inline void
setPrivateData(mrb_value self, void *p, const mrb_data_type &type)
{
	DATA_PTR(self) = p;
	DATA_TYPE(self) = &type;
}


inline mrb_value
wrapObject(mrb_state *mrb, void *p, const mrb_data_type &type)
{
	RClass *klass = mrb_class_get(mrb, type.struct_name);
	RData *data = mrb_data_object_alloc(mrb, klass, p, &type);
	mrb_value obj = mrb_obj_value(data);

	setPrivateData(obj, p, type);

	return obj;
}

inline mrb_value
wrapProperty(mrb_state *mrb, mrb_value self,
             void *prop, CommonSymbol iv, const mrb_data_type &type)
{
	mrb_value propObj = wrapObject(mrb, prop, type);

	mrb_obj_iv_set(mrb,
	               mrb_obj_ptr(self),
	               getSym(mrb, iv),
	               propObj);

	return propObj;
}

inline mrb_value
getProperty(mrb_state *mrb, mrb_value self, CommonSymbol iv)
{
	return mrb_obj_iv_get(mrb,
	                      mrb_obj_ptr(self),
	                      getSym(mrb, iv));
}

inline void
setProperty(mrb_state *mrb, mrb_value self,
            CommonSymbol iv, mrb_value propObject)
{
	mrb_obj_iv_set(mrb,
	               mrb_obj_ptr(self),
	               getSym(mrb, iv),
	               propObject);
}

template<typename T>
void
freeInstance(mrb_state *, void *instance)
{
	delete static_cast<T*>(instance);
}

inline mrb_value
mrb_bool_value(bool value)
{
	return value ? mrb_true_value() : mrb_false_value();
}

inline bool
_mrb_bool(mrb_value o)
{
	return mrb_test(o);
}

template<class C>
inline mrb_value
objectLoad(mrb_state *mrb, mrb_value self, const mrb_data_type &type)
{
	RClass *klass = mrb_class_ptr(self);
	char *data;
	int data_len;
	mrb_get_args(mrb, "s", &data, &data_len);

	C *c = C::deserialize(data, data_len);

	RData *obj = mrb_data_object_alloc(mrb, klass, c, &type);
	mrb_value obj_value = mrb_obj_value(obj);

	return obj_value;
}

MRB_METHOD_PUB(inspectObject);

#endif // BINDINGUTIL_H
