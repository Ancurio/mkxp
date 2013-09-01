/*
** table-binding.cpp
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

#include "table.h"
#include "binding-util.h"
#include "serializable-binding.h"

DEF_TYPE(Table);

RB_METHOD(tableInitialize)
{
	int x, y, z;
	x = y = z = 1;

	rb_get_args(argc, argv, "i|ii", &x, &y, &z);

	Table *t = new Table(x, y, z);

	setPrivateData(self, t, TableType);

	return self;
}

RB_METHOD(tableResize)
{
	Table *t = getPrivateData<Table>(self);

	switch (argc)
	{
	default:
	case 1:
		t->resize(rb_fix2int(argv[0]));
		return Qnil;

	case 2:
		t->resize(rb_fix2int(argv[0]),
		          rb_fix2int(argv[1]));
		return Qnil;

	case 3:
		t->resize(rb_fix2int(argv[0]),
		          rb_fix2int(argv[1]),
		          rb_fix2int(argv[2]));
		return Qnil;
	}
}

#define TABLE_SIZE(d, D) \
	RB_METHOD(table##D##Size) \
	{ \
		RB_UNUSED_PARAM \
		Table *t = getPrivateData<Table>(self); \
		return rb_int2inum(t->d##Size()); \
	}

TABLE_SIZE(x, X)
TABLE_SIZE(y, Y)
TABLE_SIZE(z, Z)

RB_METHOD(tableGetAt)
{
	Table *t = getPrivateData<Table>(self);

	int x, y, z;
	x = y = z = 0;

	x = rb_num2int(argv[0]);
	if (argc > 1)
		y = rb_num2int(argv[1]);
	if (argc > 2)
		z = rb_num2int(argv[2]);

	if (argc > 3)
		rb_raise(rb_eArgError, "wrong number of arguments");

	if (x < 0 || x >= t->xSize()
	||  y < 0 || y >= t->ySize()
	||  z < 0 || z >= t->zSize())
	{
		return Qnil;
	}

	int result = t->get(x, y, z);

	return rb_int2inum(result);
}

RB_METHOD(tableSetAt)
{
	Table *t = getPrivateData<Table>(self);

	int x, y, z, value;
	x = y = z = 0;

	if (argc < 2)
		rb_raise(rb_eArgError, "wrong number of arguments");

	switch (argc)
	{
	default:
	case 2 :
		x = rb_fix2int(argv[0]);
		value = rb_fix2int(argv[1]);
		break;
	case 3 :
		x = rb_fix2int(argv[0]);
		y = rb_fix2int(argv[1]);
		value = rb_fix2int(argv[2]);
		break;
	case 4 :
		x = rb_fix2int(argv[0]);
		y = rb_fix2int(argv[1]);
		z = rb_fix2int(argv[2]);
		value = rb_fix2int(argv[3]);
		break;
	}

	t->set(value, x, y, z);

	return rb_int2inum(value);
}

MARSH_LOAD_FUN(Table)

void
tableBindingInit()
{
	INIT_TYPE(Table);

	VALUE klass = rb_define_class("Table", rb_cObject);

	serializableBindingInit<Table>(klass);

	rb_define_class_method(klass, "_load", TableLoad);

	_rb_define_method(klass, "initialize", tableInitialize);
	_rb_define_method(klass, "resize", tableResize);
	_rb_define_method(klass, "xsize", tableXSize);
	_rb_define_method(klass, "ysize", tableYSize);
	_rb_define_method(klass, "zsize", tableZSize);
	_rb_define_method(klass, "[]", tableGetAt);
	_rb_define_method(klass, "[]=", tableSetAt);

}
