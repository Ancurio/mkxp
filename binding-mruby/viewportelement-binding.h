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
#include "binding-util.h"
#include "binding-types.h"

#include "sceneelement-binding.h"
#include "disposable-binding.h"

template<class C>
MRB_METHOD(viewportElementGetViewport)
{
	checkDisposed<C>(mrb, self);

	return getProperty(mrb, self, CSviewport);
}

template<class C>
static C *
viewportElementInitialize(mrb_state *mrb, mrb_value self)
{
	/* Get parameters */
	mrb_value viewportObj = mrb_nil_value();
	Viewport *viewport = 0;

	mrb_get_args(mrb, "|o", &viewportObj);

	if (!mrb_nil_p(viewportObj))
	{
		viewport = getPrivateDataCheck<Viewport>(mrb, viewportObj, ViewportType);

		if (rgssVer == 1)
			disposableAddChild(mrb, viewportObj, self);
	}

	/* Construct object */
	C *ve = new C(viewport);

	/* Set property objects */
	setProperty(mrb, self, CSviewport, viewportObj);

	return ve;
}

template<class C>
void
viewportElementBindingInit(mrb_state *mrb, RClass *klass)
{
	sceneElementBindingInit<C>(mrb, klass);

	mrb_define_method(mrb, klass, "viewport", viewportElementGetViewport<C>, MRB_ARGS_NONE());
}

#endif // VIEWPORTELEMENTBINDING_H
