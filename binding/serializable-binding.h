/*
** serializable-binding.h
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

#ifndef SERIALIZABLEBINDING_H
#define SERIALIZABLEBINDING_H

#include "serializable.h"
#include "binding-util.h"
#include "exception.h"

template<class C>
static VALUE
serializableDump(int, VALUE *, VALUE self)
{
	Serializable *s = getPrivateData<C>(self);

	int dataSize = s->serialSize();

	VALUE data = rb_str_new(0, dataSize);

	GUARD_EXC( s->serialize(RSTRING_PTR(data)); );

	return data;
}

template<class C>
void
serializableBindingInit(VALUE klass)
{
	_rb_define_method(klass, "_dump", serializableDump<C>);
}

#endif // SERIALIZABLEBINDING_H
