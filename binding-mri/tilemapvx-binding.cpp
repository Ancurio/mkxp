/*
** tilemapvx-binding.cpp
**
** This file is part of mkxp.
**
** Copyright (C) 2014 Jonas Kulla <Nyocurio@gmail.com>
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

#include "tilemapvx.h"
#include "viewport.h"
#include "bitmap.h"
#include "table.h"
#include "sharedstate.h"

#include "disposable-binding.h"
#include "binding-util.h"
#include "binding-types.h"

#ifndef OLD_RUBY
DEF_TYPE_CUSTOMNAME(TilemapVX, "Tilemap");

DEF_TYPE_CUSTOMFREE(BitmapArray, RUBY_TYPED_NEVER_FREE);
#else
DEF_ALLOCFUNC(TilemapVX);
#define BitmapArrayType "BitmapArray"
#endif

RB_METHOD(tilemapVXInitialize)
{
	TilemapVX *t;

	/* Get parameters */
	VALUE viewportObj = Qnil;
	Viewport *viewport = 0;

	rb_get_args(argc, argv, "|o", &viewportObj RB_ARG_END);

	if (!NIL_P(viewportObj))
		viewport = getPrivateDataCheck<Viewport>(viewportObj, ViewportType);

	/* Construct object */
	t = new TilemapVX(viewport);

	setPrivateData(self, t);

	rb_iv_set(self, "viewport", viewportObj);

	wrapProperty(self, &t->getBitmapArray(), "bitmap_array", BitmapArrayType,
	             rb_const_get(rb_cObject, rb_intern("Tilemap")));

	VALUE autotilesObj = rb_iv_get(self, "bitmap_array");

	VALUE ary = rb_ary_new2(9);
	for (int i = 0; i < 9; ++i)
		rb_ary_push(ary, Qnil);

	rb_iv_set(autotilesObj, "array", ary);

	/* Circular reference so both objects are always
	 * alive at the same time */
	rb_iv_set(autotilesObj, "tilemap", self);

	return self;
}

RB_METHOD(tilemapVXGetBitmapArray)
{
	RB_UNUSED_PARAM;

	checkDisposed<TilemapVX>(self);

	return rb_iv_get(self, "bitmap_array");
}

RB_METHOD(tilemapVXUpdate)
{
	RB_UNUSED_PARAM;

	TilemapVX *t = getPrivateData<TilemapVX>(self);

	t->update();

	return Qnil;
}

DEF_PROP_OBJ_REF(TilemapVX, Viewport, Viewport,  "viewport")
DEF_PROP_OBJ_REF(TilemapVX, Table,    MapData,   "map_data")
DEF_PROP_OBJ_REF(TilemapVX, Table,    FlashData, "flash_data")
DEF_PROP_OBJ_REF(TilemapVX, Table,    Flags,     "flags")

DEF_PROP_B(TilemapVX, Visible)

DEF_PROP_I(TilemapVX, OX)
DEF_PROP_I(TilemapVX, OY)

RB_METHOD(tilemapVXBitmapsSet)
{
	TilemapVX::BitmapArray *a = getPrivateData<TilemapVX::BitmapArray>(self);

	int i;
	VALUE bitmapObj;

	rb_get_args(argc, argv, "io", &i, &bitmapObj RB_ARG_END);

	Bitmap *bitmap = getPrivateDataCheck<Bitmap>(bitmapObj, BitmapType);

	a->set(i, bitmap);

	VALUE ary = rb_iv_get(self, "array");
	rb_ary_store(ary, i, bitmapObj);

	return self;
}

RB_METHOD(tilemapVXBitmapsGet)
{
	int i;
	rb_get_args (argc, argv, "i", &i RB_ARG_END);

	if (i < 0 || i > 8)
		return Qnil;

	VALUE ary = rb_iv_get(self, "array");

	return rb_ary_entry(ary, i);
}

void
tilemapVXBindingInit()
{
	VALUE klass = rb_define_class("Tilemap", rb_cObject);
#ifndef OLD_RUBY
	rb_define_alloc_func(klass, classAllocate<&TilemapVXType>);
#else
    rb_define_alloc_func(klass, TilemapVXAllocate);
#endif

	disposableBindingInit<TilemapVX>(klass);

	_rb_define_method(klass, "initialize", tilemapVXInitialize);
	_rb_define_method(klass, "bitmaps", tilemapVXGetBitmapArray);
	_rb_define_method(klass, "update", tilemapVXUpdate);

	INIT_PROP_BIND( TilemapVX, Viewport,  "viewport"   );
	INIT_PROP_BIND( TilemapVX, MapData,   "map_data"   );
	INIT_PROP_BIND( TilemapVX, FlashData, "flash_data" );
	INIT_PROP_BIND( TilemapVX, Visible,   "visible"    );
	INIT_PROP_BIND( TilemapVX, OX,        "ox"         );
	INIT_PROP_BIND( TilemapVX, OY,        "oy"         );

	if (rgssVer == 3)
	{
	INIT_PROP_BIND( TilemapVX, Flags,     "flags"      );
	}
	else
	{
	INIT_PROP_BIND( TilemapVX, Flags,     "passages"   );
	}

	klass = rb_define_class_under(klass, "BitmapArray", rb_cObject);
#ifndef OLD_RUBY
	rb_define_alloc_func(klass, classAllocate<&BitmapArrayType>);
#endif

	_rb_define_method(klass, "[]=", tilemapVXBitmapsSet);
	_rb_define_method(klass, "[]", tilemapVXBitmapsGet);
}
