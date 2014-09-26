/*
** tilemap-binding.cpp
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

#include "tilemap.h"
#include "viewport.h"
#include "bitmap.h"
#include "table.h"

#include "disposable-binding.h"
#include "binding-util.h"
#include "binding-types.h"

#include <mruby/array.h>

static const mrb_data_type TilemapAutotilesType =
{
    "TilemapAutotiles",
	0
};

MRB_METHOD(tilemapAutotilesSet)
{
	Tilemap::Autotiles *a = getPrivateData<Tilemap::Autotiles>(mrb, self);

	mrb_int i;
	mrb_value bitmapObj;

	mrb_get_args(mrb, "io", &i, &bitmapObj);

	Bitmap *bitmap = getPrivateDataCheck<Bitmap>(mrb, bitmapObj, BitmapType);

	a->set(i, bitmap);

	mrb_value ary = mrb_iv_get(mrb, self, getMrbData(mrb)->symbols[CSarray]);
	mrb_ary_set(mrb, ary, i, bitmapObj);

	return self;
}

MRB_METHOD(tilemapAutotilesGet)
{
	mrb_int i;

	mrb_get_args (mrb, "i", &i);

	if (i < 0 || i > 6)
		return mrb_nil_value();

	mrb_value ary = mrb_iv_get(mrb, self, getMrbData(mrb)->symbols[CSarray]);

	return mrb_ary_entry(ary, i);
}

DEF_TYPE(Tilemap);

MRB_METHOD(tilemapInitialize)
{
	Tilemap *t;

	/* Get parameters */
	mrb_value viewportObj = mrb_nil_value();
	Viewport *viewport = 0;

	mrb_get_args(mrb, "|o", &viewportObj);

	if (!mrb_nil_p(viewportObj))
		viewport = getPrivateDataCheck<Viewport>(mrb, viewportObj, ViewportType);

	/* Construct object */
	t = new Tilemap(viewport);

	setPrivateData(self, t, TilemapType);

	setProperty(mrb, self, CSviewport, viewportObj);

	wrapProperty(mrb, self, &t->getAutotiles(), CSautotiles, TilemapAutotilesType);

	MrbData &mrbData = *getMrbData(mrb);
	mrb_value autotilesObj = mrb_iv_get(mrb, self, mrbData.symbols[CSautotiles]);

	mrb_value ary = mrb_ary_new_capa(mrb, 7);
	for (int i = 0; i < 7; ++i)
		mrb_ary_push(mrb, ary, mrb_nil_value());

	mrb_iv_set(mrb, autotilesObj, mrbData.symbols[CSarray], ary);

	/* Circular reference so both objects are always
	 * alive at the same time */
	mrb_iv_set(mrb, autotilesObj, mrbData.symbols[CStilemap], self);

	return self;
}

MRB_METHOD(tilemapGetAutotiles)
{
	checkDisposed<Tilemap>(mrb, self);

	return getProperty(mrb, self, CSautotiles);
}

MRB_METHOD(tilemapUpdate)
{
	Tilemap *t = getPrivateData<Tilemap>(mrb, self);

	t->update();

	return mrb_nil_value();
}

MRB_METHOD(tilemapGetViewport)
{
	checkDisposed<Tilemap>(mrb, self);

	return getProperty(mrb, self, CSviewport);
}

DEF_PROP_OBJ_REF(Tilemap, Bitmap,   Tileset,    CStileset)
DEF_PROP_OBJ_REF(Tilemap, Table,    MapData,    CSmap_data)
DEF_PROP_OBJ_REF(Tilemap, Table,    FlashData,  CSflash_data)
DEF_PROP_OBJ_REF(Tilemap, Table,    Priorities, CSpriorities)

DEF_PROP_B(Tilemap, Visible)

DEF_PROP_I(Tilemap, OX)
DEF_PROP_I(Tilemap, OY)

void
tilemapBindingInit(mrb_state *mrb)
{
	RClass *klass = defineClass(mrb, "TilemapAutotiles");

	mrb_define_method(mrb, klass, "[]=", tilemapAutotilesSet, MRB_ARGS_REQ(2));
	mrb_define_method(mrb, klass, "[]", tilemapAutotilesGet, MRB_ARGS_REQ(1));
	mrb_define_method(mrb, klass, "inspect", inspectObject, MRB_ARGS_NONE());

	klass = defineClass(mrb, "Tilemap");

	disposableBindingInit<Tilemap>(mrb, klass);

	mrb_define_method(mrb, klass, "initialize", tilemapInitialize, MRB_ARGS_OPT(1));
	mrb_define_method(mrb, klass, "autotiles", tilemapGetAutotiles, MRB_ARGS_NONE());
	mrb_define_method(mrb, klass, "update", tilemapUpdate, MRB_ARGS_NONE());

	mrb_define_method(mrb, klass, "viewport", tilemapGetViewport, MRB_ARGS_NONE());

	INIT_PROP_BIND( Tilemap, Tileset,    "tileset"    );
	INIT_PROP_BIND( Tilemap, MapData,    "map_data"   );
	INIT_PROP_BIND( Tilemap, FlashData,  "flash_data" );
	INIT_PROP_BIND( Tilemap, Priorities, "priorities" );
	INIT_PROP_BIND( Tilemap, Visible,    "visible"    );
	INIT_PROP_BIND( Tilemap, OX,         "ox"         );
	INIT_PROP_BIND( Tilemap, OY,         "oy"         );

	mrb_define_method(mrb, klass, "inspect", inspectObject, MRB_ARGS_NONE());
}
