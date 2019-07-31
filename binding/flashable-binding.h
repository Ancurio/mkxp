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
RB_METHOD(flashableFlash)
{
	Flashable *f = getPrivateData<C>(self);

	VALUE colorObj;
	int duration;

	Color *color;

	rb_get_args(argc, argv, "oi", &colorObj, &duration RB_ARG_END);

	if (NIL_P(colorObj))
	{
		f->flash(0, duration);
		return Qnil;
	}

	color = getPrivateDataCheck<Color>(colorObj, ColorType);

	f->flash(&color->norm, duration);

	return Qnil;
}

template<class C>
RB_METHOD(flashableUpdate)
{
	RB_UNUSED_PARAM;

	Flashable *f = getPrivateData<C>(self);

	f->update();

	return Qnil;
}

template<class C>
static void flashableBindingInit(VALUE klass)
{
	_rb_define_method(klass, "flash", flashableFlash<C>);
	_rb_define_method(klass, "update", flashableUpdate<C>);
}

#endif // FLASHABLEBINDING_H
