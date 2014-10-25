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

MRB_METHOD(viewportInitialize)
{
	Viewport *v;

	if (mrb->c->ci->argc == 1)
	{
		/* The rect arg is only used to init the viewport,
		 * and does NOT replace its 'rect' property */
		mrb_value rectObj;
		Rect *rect;

		mrb_get_args(mrb, "o", &rectObj);

		rect = getPrivateDataCheck<Rect>(mrb, rectObj, RectType);

		v = new Viewport(rect);
	}
	else
	{
		mrb_int x, y, width, height;

		mrb_get_args(mrb, "iiii", &x, &y, &width, &height);

		v = new Viewport(x, y, width, height);
	}

	setPrivateData(self, v, ViewportType);

	/* Wrap property objects */
	v->initDynAttribs();

	wrapProperty(mrb, self, &v->getRect(),  CSrect,  RectType);
	wrapProperty(mrb, self, &v->getColor(), CScolor, ColorType);
	wrapProperty(mrb, self, &v->getTone(),  CStone,  ToneType);

	return self;
}

DEF_PROP_OBJ_VAL(Viewport, Rect,  Rect,  CSrect)
DEF_PROP_OBJ_VAL(Viewport, Color, Color, CScolor)
DEF_PROP_OBJ_VAL(Viewport, Tone,  Tone,  CStone)

DEF_PROP_I(Viewport, OX)
DEF_PROP_I(Viewport, OY)


void
viewportBindingInit(mrb_state *mrb)
{
	RClass *klass = defineClass(mrb, "Viewport");

	disposableBindingInit  <Viewport>(mrb, klass);
	flashableBindingInit   <Viewport>(mrb, klass);
	sceneElementBindingInit<Viewport>(mrb, klass);

	mrb_define_method(mrb, klass, "initialize", viewportInitialize, MRB_ARGS_REQ(1) | MRB_ARGS_OPT(3));

	INIT_PROP_BIND( Viewport, Rect,  "rect"  );
	INIT_PROP_BIND( Viewport, OX,    "ox"    );
	INIT_PROP_BIND( Viewport, OY,    "oy"    );
	INIT_PROP_BIND( Viewport, Color, "color" );
	INIT_PROP_BIND( Viewport, Tone,  "tone"  );

	mrb_define_method(mrb, klass, "inspect", inspectObject, MRB_ARGS_NONE());
}
