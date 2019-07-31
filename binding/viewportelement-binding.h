/*
** viewportelement-binding.h
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

#ifndef VIEWPORTELEMENTBINDING_H
#define VIEWPORTELEMENTBINDING_H

#include "viewport.h"
#include "sharedstate.h"
#include "binding-util.h"
#include "binding-types.h"

#include "sceneelement-binding.h"
#include "disposable-binding.h"

template<class C>
RB_METHOD(viewportElementGetViewport)
{
	RB_UNUSED_PARAM;

	checkDisposed<C>(self);

	return rb_iv_get(self, "viewport");
}

template<class C>
RB_METHOD(viewportElementSetViewport)
{
	RB_UNUSED_PARAM;

	ViewportElement *ve = getPrivateData<C>(self);

	VALUE viewportObj = Qnil;
	Viewport *viewport = 0;

	rb_get_args(argc, argv, "o", &viewportObj RB_ARG_END);

	if (!NIL_P(viewportObj))
		viewport = getPrivateDataCheck<Viewport>(viewportObj, ViewportType);

	GUARD_EXC( ve->setViewport(viewport); );

	rb_iv_set(self, "viewport", viewportObj);

	return viewportObj;
}

template<class C>
static C *
viewportElementInitialize(int argc, VALUE *argv, VALUE self)
{
	/* Get parameters */
	VALUE viewportObj = Qnil;
	Viewport *viewport = 0;

	rb_get_args(argc, argv, "|o", &viewportObj RB_ARG_END);

	if (!NIL_P(viewportObj))
	{
		viewport = getPrivateDataCheck<Viewport>(viewportObj, ViewportType);

		if (rgssVer == 1)
			disposableAddChild(viewportObj, self);
	}

	/* Construct object */
	C *ve = new C(viewport);

	/* Set property objects */
	rb_iv_set(self, "viewport", viewportObj);

	return ve;
}

template<class C>
void
viewportElementBindingInit(VALUE klass)
{
	sceneElementBindingInit<C>(klass);

	_rb_define_method(klass, "viewport", viewportElementGetViewport<C>);

	if (rgssVer >= 2)
	{
	_rb_define_method(klass, "viewport=", viewportElementSetViewport<C>);
	}
}

#endif // VIEWPORTELEMENTBINDING_H
