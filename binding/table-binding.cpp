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

#include "binding-util.h"
#include "serializable-binding.h"
#include "table.h"
#include <algorithm>

static int num2TableSize(VALUE v) {
  int i = NUM2INT(v);
  return std::max(0, i);
}

static void parseArgsTableSizes(int argc, VALUE *argv, int *x, int *y, int *z) {
  *y = *z = 1;

  switch (argc) {
  case 3:
    *z = num2TableSize(argv[2]);
    /* fall through */
  case 2:
    *y = num2TableSize(argv[1]);
    /* fall through */
  case 1:
    *x = num2TableSize(argv[0]);
    break;
  default:
    rb_error_arity(argc, 1, 3);
  }
}
#if RAPI_FULL > 187
DEF_TYPE(Table);
#else
DEF_ALLOCFUNC(Table);
#endif

RB_METHOD(tableInitialize) {
  int x, y, z;

  parseArgsTableSizes(argc, argv, &x, &y, &z);

  Table *t = new Table(x, y, z);

  setPrivateData(self, t);

  return self;
}

RB_METHOD(tableResize) {
  Table *t = getPrivateData<Table>(self);

  int x, y, z;
  parseArgsTableSizes(argc, argv, &x, &y, &z);

  t->resize(x, y, z);

  return Qnil;
}

#define TABLE_SIZE(d, D)                                                       \
  RB_METHOD(table##D##Size) {                                                  \
    RB_UNUSED_PARAM                                                            \
    Table *t = getPrivateData<Table>(self);                                    \
    return INT2NUM(t->d##Size());                                              \
  }

TABLE_SIZE(x, X)
TABLE_SIZE(y, Y)
TABLE_SIZE(z, Z)

RB_METHOD(tableGetAt) {
  Table *t = getPrivateData<Table>(self);

  int x, y, z;
  x = y = z = 0;

  x = NUM2INT(argv[0]);
  if (argc > 1)
    y = NUM2INT(argv[1]);
  if (argc > 2)
    z = NUM2INT(argv[2]);

  if (argc > 3)
    rb_raise(rb_eArgError, "wrong number of arguments");

  if (x < 0 || x >= t->xSize() || y < 0 || y >= t->ySize() || z < 0 ||
      z >= t->zSize()) {
    return Qnil;
  }

  short result = t->get(x, y, z);

  return INT2FIX(result); /* short always fits in a Fixnum */
}

RB_METHOD(tableSetAt) {
  Table *t = getPrivateData<Table>(self);

  int x, y, z, value;
  x = y = z = 0;

  if (argc < 2)
    rb_raise(rb_eArgError, "wrong number of arguments");

  switch (argc) {
  default:
  case 2:
    x = NUM2INT(argv[0]);
    value = NUM2INT(argv[1]);

    break;
  case 3:
    x = NUM2INT(argv[0]);
    y = NUM2INT(argv[1]);
    value = NUM2INT(argv[2]);

    break;
  case 4:
    x = NUM2INT(argv[0]);
    y = NUM2INT(argv[1]);
    z = NUM2INT(argv[2]);
    value = NUM2INT(argv[3]);

    break;
  }

  t->set(value, x, y, z);

  return argv[argc - 1];
}

MARSH_LOAD_FUN(Table)
INITCOPY_FUN(Table)

void tableBindingInit() {
  VALUE klass = rb_define_class("Table", rb_cObject);
#if RAPI_FULL > 187
  rb_define_alloc_func(klass, classAllocate<&TableType>);
#else
  rb_define_alloc_func(klass, TableAllocate);
#endif

  serializableBindingInit<Table>(klass);

  rb_define_class_method(klass, "_load", TableLoad);

  _rb_define_method(klass, "initialize", tableInitialize);
  _rb_define_method(klass, "initialize_copy", TableInitializeCopy);
  _rb_define_method(klass, "resize", tableResize);
  _rb_define_method(klass, "xsize", tableXSize);
  _rb_define_method(klass, "ysize", tableYSize);
  _rb_define_method(klass, "zsize", tableZSize);
  _rb_define_method(klass, "[]", tableGetAt);
  _rb_define_method(klass, "[]=", tableSetAt);
}
