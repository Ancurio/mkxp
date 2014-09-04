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
#include "serializable-binding.h"
#include "sharedstate.h"

DEF_TYPE(Color);
DEF_TYPE(Tone);
DEF_TYPE(Rect);

#define ATTR_RW(Klass, Attr, arg_type, arg_t_s, value_fun) \
	RB_METHOD(Klass##Get##Attr) \
	{ \
		RB_UNUSED_PARAM \
		Klass *p = getPrivateData<Klass>(self); \
		return value_fun(p->get##Attr()); \
	} \
	RB_METHOD(Klass##Set##Attr) \
	{ \
		Klass *p = getPrivateData<Klass>(self); \
		arg_type arg; \
		rb_get_args(argc, argv, arg_t_s, &arg RB_ARG_END); \
		p->set##Attr(arg); \
		return *argv; \
	}

#define ATTR_DOUBLE_RW(Klass, Attr) ATTR_RW(Klass, Attr, double, "f", rb_float_new)
#define ATTR_INT_RW(Klass, Attr)   ATTR_RW(Klass, Attr, int, "i", rb_fix_new)

ATTR_DOUBLE_RW(Color, Red)
ATTR_DOUBLE_RW(Color, Green)
ATTR_DOUBLE_RW(Color, Blue)
ATTR_DOUBLE_RW(Color, Alpha)

ATTR_DOUBLE_RW(Tone, Red)
ATTR_DOUBLE_RW(Tone, Green)
ATTR_DOUBLE_RW(Tone, Blue)
ATTR_DOUBLE_RW(Tone, Gray)

ATTR_INT_RW(Rect, X)
ATTR_INT_RW(Rect, Y)
ATTR_INT_RW(Rect, Width)
ATTR_INT_RW(Rect, Height)

#define EQUAL_FUN(Klass) \
	RB_METHOD(Klass##Equal) \
	{ \
		Klass *p = getPrivateData<Klass>(self); \
		VALUE otherObj; \
		Klass *other; \
		rb_get_args(argc, argv, "o", &otherObj RB_ARG_END); \
		if (rgssVer >= 3) \
			if (!rb_typeddata_is_kind_of(otherObj, &Klass##Type)) \
				return Qfalse; \
		other = getPrivateDataCheck<Klass>(otherObj, Klass##Type); \
		return rb_bool_new(*p == *other); \
	}

EQUAL_FUN(Color)
EQUAL_FUN(Tone)
EQUAL_FUN(Rect)

#define INIT_FUN(Klass, param_type, param_t_s, last_param_def) \
	RB_METHOD(Klass##Initialize) \
	{ \
		Klass *k; \
		if (argc == 0) \
		{ \
			k = new Klass(); \
		} \
		else \
		{ \
			param_type p1, p2, p3, p4 = last_param_def; \
			rb_get_args(argc, argv, param_t_s, &p1, &p2, &p3, &p4 RB_ARG_END); \
			k = new Klass(p1, p2, p3, p4); \
		} \
		setPrivateData(self, k); \
		return self; \
	}

INIT_FUN(Color, double, "fff|f", 255)
INIT_FUN(Tone, double, "fff|f", 0)
INIT_FUN(Rect, int, "iiii", 0)

#define SET_FUN(Klass, param_type, param_t_s, last_param_def) \
	RB_METHOD(Klass##Set) \
	{ \
		Klass *k = getPrivateData<Klass>(self); \
		if (argc == 1) \
		{ \
			VALUE otherObj = argv[0]; \
			Klass *other = getPrivateDataCheck<Klass>(otherObj, Klass##Type); \
			*k = *other; \
		} \
		else \
		{ \
			param_type p1, p2, p3, p4 = last_param_def; \
			rb_get_args(argc, argv, param_t_s, &p1, &p2, &p3, &p4 RB_ARG_END); \
			k->set(p1, p2, p3, p4); \
		} \
		return self; \
	}

SET_FUN(Color, double, "fff|f", 255)
SET_FUN(Tone, double, "fff|f", 0)
SET_FUN(Rect, int, "iiii", 0)

RB_METHOD(rectEmpty)
{
	RB_UNUSED_PARAM;
	Rect *r = getPrivateData<Rect>(self);
	r->empty();
	return self;
}

RB_METHOD(ColorStringify)
{
	RB_UNUSED_PARAM;

	Color *c = getPrivateData<Color>(self);

	return rb_sprintf("(%f, %f, %f, %f)",
	                  c->red, c->green, c->blue, c->alpha);
}

RB_METHOD(ToneStringify)
{
	RB_UNUSED_PARAM;

	Tone *t = getPrivateData<Tone>(self);

	return rb_sprintf("(%f, %f, %f, %f)",
	                  t->red, t->green, t->blue, t->gray);
}

RB_METHOD(RectStringify)
{
	RB_UNUSED_PARAM;

	Rect *r = getPrivateData<Rect>(self);

	return rb_sprintf("(%d, %d, %d, %d)",
	                  r->x, r->y, r->width, r->height);
}

MARSH_LOAD_FUN(Color)
MARSH_LOAD_FUN(Tone)
MARSH_LOAD_FUN(Rect)

INITCOPY_FUN(Tone)
INITCOPY_FUN(Color)
INITCOPY_FUN(Rect)

#define INIT_BIND(Klass) \
{ \
	klass = rb_define_class(#Klass, rb_cObject); \
	rb_define_alloc_func(klass, classAllocate<&Klass##Type>); \
	rb_define_class_method(klass, "_load", Klass##Load); \
	serializableBindingInit<Klass>(klass); \
	_rb_define_method(klass, "initialize", Klass##Initialize); \
	_rb_define_method(klass, "initialize_copy", Klass##InitializeCopy); \
	_rb_define_method(klass, "set", Klass##Set); \
	_rb_define_method(klass, "==", Klass##Equal); \
	_rb_define_method(klass, "===", Klass##Equal); \
	_rb_define_method(klass, "eql?", Klass##Equal); \
	_rb_define_method(klass, "to_s", Klass##Stringify); \
	_rb_define_method(klass, "inspect", Klass##Stringify); \
}

#define MRB_ATTR_R(Class, attr) mrb_define_method(mrb, klass, #attr, Class##Get_##attr, MRB_ARGS_NONE())
#define MRB_ATTR_W(Class, attr) mrb_define_method(mrb, klass, #attr "=", Class##Set_##attr, MRB_ARGS_REQ(1))
#define MRB_ATTR_RW(Class, attr) { MRB_ATTR_R(Class, attr); MRB_ATTR_W(Class, attr); }

#define RB_ATTR_R(Klass, Attr, attr) _rb_define_method(klass, #attr, Klass##Get##Attr)
#define RB_ATTR_W(Klass, Attr, attr) _rb_define_method(klass, #attr "=", Klass##Set##Attr)
#define RB_ATTR_RW(Klass, Attr, attr) \
	{ RB_ATTR_R(Klass, Attr, attr); RB_ATTR_W(Klass, Attr, attr); }

void
etcBindingInit()
{
	VALUE klass;

	INIT_BIND(Color);

	RB_ATTR_RW(Color, Red, red);
	RB_ATTR_RW(Color, Green, green);
	RB_ATTR_RW(Color, Blue, blue);
	RB_ATTR_RW(Color, Alpha, alpha);

	INIT_BIND(Tone);

	RB_ATTR_RW(Tone, Red, red);
	RB_ATTR_RW(Tone, Green, green);
	RB_ATTR_RW(Tone, Blue, blue);
	RB_ATTR_RW(Tone, Gray, gray);

	INIT_BIND(Rect);

	RB_ATTR_RW(Rect, X, x);
	RB_ATTR_RW(Rect, Y, y);
	RB_ATTR_RW(Rect, Width, width);
	RB_ATTR_RW(Rect, Height, height);
	_rb_define_method(klass, "empty", rectEmpty);
}
