/*
** sprite-binding.cpp
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

#include "sprite.h"
#include "disposable-binding.h"
#include "flashable-binding.h"
#include "sceneelement-binding.h"
#include "viewportelement-binding.h"
#include "binding-util.h"
#include "binding-types.h"

DEF_TYPE(Sprite);

RB_METHOD(spriteInitialize)
{
	Sprite *s = viewportElementInitialize<Sprite>(argc, argv, self);

	setPrivateData(self, s, SpriteType);

	/* Wrap property objects */
	s->setSrcRect(new Rect);
	s->setColor(new Color);
	s->setTone(new Tone);

	wrapNilProperty(self, "bitmap");
	wrapProperty(self, s->getSrcRect(), "src_rect", RectType);
	wrapProperty(self, s->getColor(), "color", ColorType);
	wrapProperty(self, s->getTone(), "tone", ToneType);

	return self;
}

#define DISP_CLASS_NAME "sprite"

DEF_PROP_OBJ_NIL(Sprite, Bitmap, Bitmap,  "bitmap")
DEF_PROP_OBJ(Sprite, Rect,   SrcRect, "src_rect")
DEF_PROP_OBJ(Sprite, Color,  Color,   "color")
DEF_PROP_OBJ(Sprite, Tone,   Tone,    "tone")

DEF_PROP_I(Sprite, X)
DEF_PROP_I(Sprite, Y)
DEF_PROP_I(Sprite, OX)
DEF_PROP_I(Sprite, OY)
DEF_PROP_I(Sprite, BushDepth)
DEF_PROP_I(Sprite, Opacity)
DEF_PROP_I(Sprite, BlendType)

DEF_PROP_F(Sprite, ZoomX)
DEF_PROP_F(Sprite, ZoomY)
DEF_PROP_F(Sprite, Angle)

DEF_PROP_B(Sprite, Mirror)

void
spriteBindingInit()
{
	INIT_TYPE(Sprite);

	VALUE klass = rb_define_class("Sprite", rb_cObject);

	disposableBindingInit     <Sprite>(klass);
	flashableBindingInit      <Sprite>(klass);
	viewportElementBindingInit<Sprite>(klass);

	_rb_define_method(klass, "initialize", spriteInitialize);

	INIT_PROP_BIND( Sprite, Bitmap,    "bitmap"     );
	INIT_PROP_BIND( Sprite, SrcRect,   "src_rect"   );
	INIT_PROP_BIND( Sprite, X,         "x"          );
	INIT_PROP_BIND( Sprite, Y,         "y"          );
	INIT_PROP_BIND( Sprite, OX,        "ox"         );
	INIT_PROP_BIND( Sprite, OY,        "oy"         );
	INIT_PROP_BIND( Sprite, ZoomX,     "zoom_x"     );
	INIT_PROP_BIND( Sprite, ZoomY,     "zoom_y"     );
	INIT_PROP_BIND( Sprite, Angle,     "angle"      );
	INIT_PROP_BIND( Sprite, Mirror,    "mirror"     );
	INIT_PROP_BIND( Sprite, BushDepth, "bush_depth" );
	INIT_PROP_BIND( Sprite, Opacity,   "opacity"    );
	INIT_PROP_BIND( Sprite, BlendType, "blend_type" );
	INIT_PROP_BIND( Sprite, Color,     "color"      );
	INIT_PROP_BIND( Sprite, Tone,      "tone"       );
}
