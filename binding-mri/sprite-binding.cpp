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
#include "sharedstate.h"
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

	setPrivateData(self, s);

	/* Wrap property objects */
	s->initDynAttribs();

	wrapProperty(self, &s->getSrcRect(), "src_rect", RectType);
	wrapProperty(self, &s->getColor(), "color", ColorType);
	wrapProperty(self, &s->getTone(), "tone", ToneType);

	return self;
}

DEF_PROP_OBJ_REF(Sprite, Bitmap, Bitmap,  "bitmap")
DEF_PROP_OBJ_VAL(Sprite, Rect,   SrcRect, "src_rect")
DEF_PROP_OBJ_VAL(Sprite, Color,  Color,   "color")
DEF_PROP_OBJ_VAL(Sprite, Tone,   Tone,    "tone")

DEF_PROP_I(Sprite, X)
DEF_PROP_I(Sprite, Y)
DEF_PROP_I(Sprite, OX)
DEF_PROP_I(Sprite, OY)
DEF_PROP_I(Sprite, BushDepth)
DEF_PROP_I(Sprite, BushOpacity)
DEF_PROP_I(Sprite, Opacity)
DEF_PROP_I(Sprite, BlendType)
DEF_PROP_I(Sprite, WaveAmp)
DEF_PROP_I(Sprite, WaveLength)
DEF_PROP_I(Sprite, WaveSpeed)

DEF_PROP_F(Sprite, ZoomX)
DEF_PROP_F(Sprite, ZoomY)
DEF_PROP_F(Sprite, Angle)
DEF_PROP_F(Sprite, WavePhase)

DEF_PROP_B(Sprite, Mirror)

RB_METHOD(spriteWidth)
{
	RB_UNUSED_PARAM;

	Sprite *s = getPrivateData<Sprite>(self);

	int value = 0;
	GUARD_EXC( value = s->getWidth(); )

	return rb_fix_new(value);
}

RB_METHOD(spriteHeight)
{
	RB_UNUSED_PARAM;

	Sprite *s = getPrivateData<Sprite>(self);

	int value = 0;
	GUARD_EXC( value = s->getHeight(); )

	return rb_fix_new(value);
}

void
spriteBindingInit()
{
	VALUE klass = rb_define_class("Sprite", rb_cObject);
	rb_define_alloc_func(klass, classAllocate<&SpriteType>);

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

	if (rgssVer >= 2)
	{
	_rb_define_method(klass, "width", spriteWidth);
	_rb_define_method(klass, "height", spriteHeight);

	INIT_PROP_BIND( Sprite, BushOpacity, "bush_opacity" );

	INIT_PROP_BIND( Sprite, WaveAmp,    "wave_amp"    );
	INIT_PROP_BIND( Sprite, WaveLength, "wave_length" );
	INIT_PROP_BIND( Sprite, WaveSpeed,  "wave_speed"  );
	INIT_PROP_BIND( Sprite, WavePhase,  "wave_phase"  );
	}
}
