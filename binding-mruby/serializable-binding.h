/*
** serializable-binding.h
**
** This file is part of mkxp.
**
** Copyright (C) 2021 Amaryllis Kulla <amaryllis.kulla@protonmail.com>
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

#include <mruby/string.h>

template<class C>
MRB_METHOD(serializableDump)
{
	Serializable *s = getPrivateData<C>(mrb, self);

	int dataSize = s->serialSize();

	mrb_value data = mrb_str_new(mrb, 0, dataSize);

	s->serialize(RSTRING_PTR(data));

	return data;
}

template<class C>
void
serializableBindingInit(mrb_state *mrb, RClass *klass)
{
	mrb_define_method(mrb, klass, "_dump", serializableDump<C>, MRB_ARGS_NONE());
}

#endif // SERIALIZABLEBINDING_H
