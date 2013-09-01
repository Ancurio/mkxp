/*
** flashable-binding.h
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

#ifndef FLASHABLEBINDING_H
#define FLASHABLEBINDING_H

#include "flashable.h"
#include "binding-util.h"
#include "binding-types.h"

template<class C>
MRB_METHOD(flashableFlash)
{
	Flashable *f = getPrivateData<C>(mrb, self);

	mrb_value colorObj;
	mrb_int duration;

	Color *color;

	mrb_get_args(mrb, "oi", &colorObj, &duration);

	if (mrb_nil_p(colorObj))
	{
		f->flash(0, duration);
		return mrb_nil_value();
	}

	color = getPrivateDataCheck<Color>(mrb, colorObj, ColorType);

	f->flash(&color->norm, duration);

	return mrb_nil_value();
}

template<class C>
MRB_METHOD(flashableUpdate)
{
	Flashable *f = getPrivateData<C>(mrb, self);

	f->update();

	return mrb_nil_value();
}

template<class C>
static void flashableBindingInit(mrb_state *mrb, RClass *klass)
{
	mrb_define_method(mrb, klass, "flash", flashableFlash<C>, MRB_ARGS_REQ(2));
	mrb_define_method(mrb, klass, "update", flashableUpdate<C>, MRB_ARGS_NONE());
}

#endif // FLASHABLEBINDING_H
