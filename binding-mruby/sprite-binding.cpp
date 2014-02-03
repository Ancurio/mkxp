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

DEF_TYPE(Sprite);

MRB_METHOD(spriteInitialize)
{
	Sprite *s = viewportElementInitialize<Sprite>(mrb, self);

	setPrivateData(mrb, self, s, SpriteType);

	/* Wrap property objects */
	s->setSrcRect(new Rect);
	s->setColor(new Color);
	s->setTone(new Tone);

	wrapNilProperty(mrb, self, CSbitmap);
	wrapProperty(mrb, self, s->getSrcRect(), CSsrc_rect, RectType);
	wrapProperty(mrb, self, s->getColor(), CScolor, ColorType);
	wrapProperty(mrb, self, s->getTone(), CStone, ToneType);

	return self;
}

#define DISP_CLASS_NAME "sprite"

DEF_PROP_OBJ_NIL(Sprite, Bitmap, Bitmap,  CSbitmap)
DEF_PROP_OBJ(Sprite, Rect,   SrcRect, CSsrc_rect)
DEF_PROP_OBJ(Sprite, Color,  Color,   CScolor)
DEF_PROP_OBJ(Sprite, Tone,   Tone,    CStone)

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

#ifdef RGSS2

MRB_METHOD(spriteWidth)
{
	Sprite *s = getPrivateData<Sprite>(mrb, self);

	int value;
	GUARD_EXC( value = s->getWidth(); )

	return mrb_fixnum_value(value);
}

MRB_METHOD(spriteHeight)
{
	Sprite *s = getPrivateData<Sprite>(mrb, self);

	int value;
	GUARD_EXC( value = s->getHeight(); )

	return mrb_fixnum_value(value);
}

DEF_PROP_I(Sprite, WaveAmp)
DEF_PROP_I(Sprite, WaveLength)
DEF_PROP_I(Sprite, WaveSpeed)
DEF_PROP_F(Sprite, WavePhase)

#endif

void
spriteBindingInit(mrb_state *mrb)
{
	RClass *klass = mrb_define_class(mrb, "Sprite", 0);

	disposableBindingInit     <Sprite>(mrb, klass);
	flashableBindingInit      <Sprite>(mrb, klass);
	viewportElementBindingInit<Sprite>(mrb, klass);

	mrb_define_method(mrb, klass, "initialize", spriteInitialize, MRB_ARGS_OPT(1));

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

#ifdef RGSS2
	mrb_define_method(mrb, klass, "width",  spriteWidth,  MRB_ARGS_NONE());
	mrb_define_method(mrb, klass, "height", spriteHeight, MRB_ARGS_NONE());

	INIT_PROP_BIND( Sprite, WaveAmp,    "wave_amp"    );
	INIT_PROP_BIND( Sprite, WaveLength, "wave_length" );
	INIT_PROP_BIND( Sprite, WaveSpeed,  "wave_speed"  );
	INIT_PROP_BIND( Sprite, WavePhase,  "wave_phase"  );
#endif

	mrb_define_method(mrb, klass, "inspect", inspectObject, MRB_ARGS_NONE());
}
