/*
** disposable-binding.h
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

#ifndef DISPOSABLEBINDING_H
#define DISPOSABLEBINDING_H

#include "disposable.h"
#include "binding-util.h"

template<class C>
RB_METHOD(disposableDispose)
{
	RB_UNUSED_PARAM;

	Disposable *d = getPrivateData<C>(self);

	d->dispose();

	return Qnil;
}

template<class C>
RB_METHOD(disposableIsDisposed)
{
	RB_UNUSED_PARAM;

	Disposable *d = getPrivateData<C>(self);

	return rb_bool_new(d->isDisposed());
}

template<class C>
static void disposableBindingInit(VALUE klass)
{
	_rb_define_method(klass, "dispose", disposableDispose<C>);
	_rb_define_method(klass, "disposed?", disposableIsDisposed<C>);
}

inline void checkDisposed(Disposable *d, const char *klassName)
{
	RbData *data = getRbData(); (void) data;

	if (d->isDisposed())
		rb_raise(getRbData()->exc[RGSS], "disposed %s", klassName);
}

#endif // DISPOSABLEBINDING_H
