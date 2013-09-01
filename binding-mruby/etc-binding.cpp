/*
** etc-binding.cpp
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

#include "etc.h"
#include "binding-util.h"
#include "binding-types.h"
#include "serializable-binding.h"
#include "mruby/class.h"

#include <QDebug>

#define ATTR_RW(Type, attr, arg_type, mrb_val, arg_t_s) \
	MRB_METHOD(Type##Get_##attr) \
	{ \
		Type *p = getPrivateData<Type>(mrb, self); \
	 \
		return mrb_##mrb_val##_value(p->attr); \
	} \
\
	MRB_METHOD(Type##Set_##attr) \
	{ \
		Type *p = getPrivateData<Type>(mrb, self); \
	 \
		arg_type arg; \
		mrb_get_args(mrb, arg_t_s, &arg); \
	 \
		p->attr = arg; \
		UPDATE_F \
	 \
		return mrb_##mrb_val##_value(arg); \
	}

#define EQUAL_FUN(Typ) \
	MRB_METHOD(Typ##Equal) \
	{ \
		Typ *p = getPrivateData<Typ>(mrb, self); \
		mrb_value otherObj; \
		Typ *other; \
		mrb_get_args(mrb, "o", &otherObj); \
		RClass *klass = mrb_obj_class(mrb, self); \
		RClass *otherKlass = mrb_obj_class(mrb, otherObj); \
		if (klass != otherKlass) \
			return mrb_false_value(); \
		other = getPrivateDataCheck<Typ>(mrb, otherObj, Typ##Type); \
		return mrb_bool_value(*p == *other); \
	}

#define ATTR_FLOAT_RW(Type, attr) ATTR_RW(Type, attr, mrb_float, _float, "f")
#define ATTR_INT_RW(Type, attr)   ATTR_RW(Type, attr, mrb_int, fixnum, "i")

#define UPDATE_F p->updateInternal();
ATTR_FLOAT_RW(Color, red)
ATTR_FLOAT_RW(Color, green)
ATTR_FLOAT_RW(Color, blue)
ATTR_FLOAT_RW(Color, alpha)

ATTR_FLOAT_RW(Tone, red)
ATTR_FLOAT_RW(Tone, green)
ATTR_FLOAT_RW(Tone, blue)
ATTR_FLOAT_RW(Tone, gray)

#undef UPDATE_F
#define UPDATE_F
ATTR_INT_RW(Rect, x)
ATTR_INT_RW(Rect, y)
ATTR_INT_RW(Rect, width)
ATTR_INT_RW(Rect, height)

EQUAL_FUN(Color)
EQUAL_FUN(Tone)
EQUAL_FUN(Rect)

DEF_TYPE(Color);
DEF_TYPE(Tone);
DEF_TYPE(Rect);

#define INIT_FUN(Klass, param_type, param_t_s, last_param_def) \
	MRB_METHOD(Klass##Initialize) \
	{ \
		param_type p1, p2, p3, p4 = last_param_def; \
		mrb_get_args(mrb, param_t_s, &p1, &p2, &p3, &p4); \
		Klass *k = new Klass(p1, p2, p3, p4); \
		setPrivateData(mrb, self, k, Klass##Type); \
		return self; \
	}

INIT_FUN(Color, mrb_float, "fff|f", 255)
INIT_FUN(Tone, mrb_float, "fff|f", 0)
INIT_FUN(Rect, mrb_int, "iiii", 0)

#define SET_FUN(Klass, param_type, param_t_s, last_param_def) \
	MRB_METHOD(Klass##Set) \
	{ \
		param_type p1, p2, p3, p4 = last_param_def; \
		mrb_get_args(mrb, param_t_s, &p1, &p2, &p3, &p4); \
		Klass *k = getPrivateData<Klass>(mrb, self); \
		k->set(p1, p2, p3, p4); \
		return self; \
	}

SET_FUN(Color, mrb_float, "fff|f", 255)
SET_FUN(Tone, mrb_float, "fff|f", 0)
SET_FUN(Rect, mrb_int, "iiii", 0)

MRB_METHOD(RectEmpty)
{
	Rect *r = getPrivateData<Rect>(mrb, self);

	r->empty();

	return mrb_nil_value();
}

static char buffer[64];

MRB_METHOD(ColorStringify)
{
	Color *c = getPrivateData<Color>(mrb, self);

	sprintf(buffer, "(%f, %f, %f, %f)", c->red, c->green, c->blue, c->alpha);

	return mrb_str_new_cstr(mrb, buffer);
}

MRB_METHOD(ToneStringify)
{
	Tone *t = getPrivateData<Tone>(mrb, self);

	sprintf(buffer, "(%f, %f, %f, %f)", t->red, t->green, t->blue, t->gray);

	return mrb_str_new_cstr(mrb, buffer);
}

MRB_METHOD(RectStringify)
{
	Rect *r = getPrivateData<Rect>(mrb, self);

	sprintf(buffer, "(%d, %d, %d, %d)", r->x, r->y, r->width, r->height);

	return mrb_str_new_cstr(mrb, buffer);
}

MARSH_LOAD_FUN(Color)
MARSH_LOAD_FUN(Tone)
MARSH_LOAD_FUN(Rect)

CLONE_FUN(Tone)
CLONE_FUN(Color)
CLONE_FUN(Rect)

#define MRB_ATTR_R(Class, attr) mrb_define_method(mrb, klass, #attr, Class##Get_##attr, MRB_ARGS_NONE())
#define MRB_ATTR_W(Class, attr) mrb_define_method(mrb, klass, #attr "=", Class##Set_##attr, MRB_ARGS_REQ(1))
#define MRB_ATTR_RW(Class, attr) { MRB_ATTR_R(Class, attr); MRB_ATTR_W(Class, attr); }

#define MRB_ATTR_RW_A(Class, attr, alias) \
{ \
	mrb_define_method(mrb, klass, #alias, Class##Get_##attr, MRB_ARGS_NONE()); \
	mrb_define_method(mrb, klass, #alias "=", Class##Set_##attr, MRB_ARGS_REQ(1)); \
}

#define INIT_BIND(Klass) \
{ \
	klass = mrb_define_class(mrb, #Klass, 0); \
	mrb_define_class_method(mrb, klass, "_load", Klass##Load, MRB_ARGS_REQ(1)); \
	serializableBindingInit<Klass>(mrb, klass); \
	mrb_define_method(mrb, klass, "initialize", Klass##Initialize, MRB_ARGS_REQ(3) | MRB_ARGS_OPT(1)); \
	mrb_define_method(mrb, klass, "set", Klass##Set, MRB_ARGS_REQ(3) | MRB_ARGS_OPT(1)); \
	mrb_define_method(mrb, klass, "clone", Klass##Clone, MRB_ARGS_NONE()); \
	mrb_define_method(mrb, klass, "==", Klass##Equal, MRB_ARGS_REQ(1)); \
	mrb_define_method(mrb, klass, "to_s", Klass##Stringify, MRB_ARGS_NONE()); \
	mrb_define_method(mrb, klass, "inspect", Klass##Stringify, MRB_ARGS_NONE()); \
}

void etcBindingInit(mrb_state *mrb)
{
	RClass *klass;

	INIT_BIND(Color);
	MRB_ATTR_RW(Color, red);
	MRB_ATTR_RW(Color, green);
	MRB_ATTR_RW(Color, blue);
	MRB_ATTR_RW(Color, alpha);

	INIT_BIND(Tone);
	MRB_ATTR_RW(Tone, red);
	MRB_ATTR_RW(Tone, green);
	MRB_ATTR_RW(Tone, blue);
	MRB_ATTR_RW(Tone, gray);

	INIT_BIND(Rect);
	MRB_ATTR_RW(Rect, x);
	MRB_ATTR_RW(Rect, y);
	MRB_ATTR_RW(Rect, width);
	MRB_ATTR_RW(Rect, height);
	mrb_define_method(mrb, klass, "empty", RectEmpty, MRB_ARGS_NONE());
}
