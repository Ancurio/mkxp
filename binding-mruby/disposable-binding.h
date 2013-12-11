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

#include <string.h>

template<class C>
MRB_METHOD(disposableDispose)
{
	Disposable *d = getPrivateData<C>(mrb, self);

	d->dispose();

	return mrb_nil_value();
}

template<class C>
MRB_METHOD(disposableDisposed)
{
	Disposable *d = getPrivateData<C>(mrb, self);

	return mrb_bool_value(d->isDisposed());
}

template<class C>
static void disposableBindingInit(mrb_state *mrb, RClass *klass)
{
	mrb_define_method(mrb, klass, "dispose", disposableDispose<C>, MRB_ARGS_NONE());
	mrb_define_method(mrb, klass, "disposed?", disposableDisposed<C>, MRB_ARGS_NONE());
}

inline void checkDisposed(mrb_state *mrb, Disposable *d, const char *klassName)
{
	MrbData *data = getMrbData(mrb);

	if (d->isDisposed())
		mrb_raisef(mrb, data->exc[RGSS], "disposed %S",
		           mrb_str_new_static(mrb, klassName, strlen(klassName)));
}

#endif // DISPOSABLEBINDING_H
