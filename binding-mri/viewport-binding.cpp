/*
** viewport-binding.cpp
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

#include "viewport.h"
#include "disposable-binding.h"
#include "flashable-binding.h"
#include "sceneelement-binding.h"
#include "binding-util.h"
#include "binding-types.h"

DEF_TYPE(Viewport);

RB_METHOD(viewportInitialize)
{
	Viewport *v;

#ifdef RGSS3
	if (argc == 0)
	{
		v = new Viewport();
	}
	else
#endif
	if (argc == 1)
	{
		/* The rect arg is only used to init the viewport,
		 * and does NOT replace its 'rect' property */
		VALUE rectObj;
		Rect *rect;

		rb_get_args(argc, argv, "o", &rectObj RB_ARG_END);

		rect = getPrivateDataCheck<Rect>(rectObj, RectType);

		v = new Viewport(rect);
	}
	else
	{
		int x, y, width, height;

		rb_get_args(argc, argv, "iiii", &x, &y, &width, &height RB_ARG_END);

		v = new Viewport(x, y, width, height);
	}

	setPrivateData(self, v);

	/* Wrap property objects */
	v->setRect(new Rect(*v->getRect()));
	v->setColor(new Color);
	v->setTone(new Tone);

	wrapProperty(self, v->getRect(),  "rect",  RectType);
	wrapProperty(self, v->getColor(), "color", ColorType);
	wrapProperty(self, v->getTone(),  "tone",  ToneType);

	/* 'elements' holds all SceneElements that become children
	 * of this viewport, so we can dispose them when the viewport
	 * is disposed */
	rb_iv_set(self, "elements", rb_ary_new());

	return self;
}

DEF_PROP_OBJ(Viewport, Rect, Rect, "rect")
DEF_PROP_OBJ(Viewport, Color, Color, "color")
DEF_PROP_OBJ(Viewport, Tone, Tone, "tone")

DEF_PROP_I(Viewport, OX)
DEF_PROP_I(Viewport, OY)


void
viewportBindingInit()
{
	INIT_TYPE(Viewport);

	VALUE klass = rb_define_class("Viewport", rb_cObject);
	rb_define_alloc_func(klass, classAllocate<&ViewportType>);

	disposableBindingInit  <Viewport>(klass);
	flashableBindingInit   <Viewport>(klass);
	sceneElementBindingInit<Viewport>(klass);

	_rb_define_method(klass, "initialize", viewportInitialize);

	INIT_PROP_BIND( Viewport, Rect,  "rect"  );
	INIT_PROP_BIND( Viewport, OX,    "ox"    );
	INIT_PROP_BIND( Viewport, OY,    "oy"    );
	INIT_PROP_BIND( Viewport, Color, "color" );
	INIT_PROP_BIND( Viewport, Tone,  "tone"  );
}

