/*
** bitmap-binding.cpp
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

#include "bitmap.h"
#include "font.h"
#include "exception.h"
#include "sharedstate.h"
#include "disposable-binding.h"
#include "binding-util.h"
#include "binding-types.h"

DEF_TYPE(Bitmap);

MRB_METHOD(bitmapInitialize)
{
	Bitmap *b = 0;

	if (mrb->c->ci->argc == 1)
	{
		char *filename;
		mrb_get_args(mrb, "z", &filename);

		GUARD_EXC( b = new Bitmap(filename); )
	}
	else
	{
		mrb_int width, height;
		mrb_get_args(mrb, "ii", &width, &height);

		GUARD_EXC( b = new Bitmap(width, height); )
	}

	setPrivateData(self, b, BitmapType);

	/* Wrap properties */
	Font *font = new Font();
	b->setInitFont(font);
	font->initDynAttribs();

	mrb_value fontProp = wrapProperty(mrb, self, font, CSfont, FontType);

	wrapProperty(mrb, fontProp, &font->getColor(), CScolor, ColorType);

	if (rgssVer >= 3)
		wrapProperty(mrb, fontProp, &font->getOutColor(), CSout_color, ColorType);

	return self;
}

MRB_METHOD(bitmapWidth)
{
	Bitmap *b = getPrivateData<Bitmap>(mrb, self);

	mrb_int value = 0;
	GUARD_EXC( value = b->width(); )

	return mrb_fixnum_value(value);
}

MRB_METHOD(bitmapHeight)
{
	Bitmap *b = getPrivateData<Bitmap>(mrb, self);

	mrb_int value = 0;
	GUARD_EXC( value = b->height(); )

	return mrb_fixnum_value(value);
}

MRB_METHOD(bitmapRect)
{
	Bitmap *b = getPrivateData<Bitmap>(mrb, self);

	IntRect rect;
	GUARD_EXC( rect = b->rect(); )

	Rect *r = new Rect(rect);

	return wrapObject(mrb, r, RectType);
}

MRB_METHOD(bitmapBlt)
{
	Bitmap *b = getPrivateData<Bitmap>(mrb, self);

	mrb_int x, y;
	mrb_value srcObj;
	mrb_value srcRectObj;
	mrb_int opacity = 255;

	Bitmap *src;
	Rect *srcRect;

	mrb_get_args(mrb, "iioo|i", &x, &y, &srcObj, &srcRectObj, &opacity);

	src = getPrivateDataCheck<Bitmap>(mrb, srcObj, BitmapType);
	srcRect = getPrivateDataCheck<Rect>(mrb, srcRectObj, RectType);

	GUARD_EXC( b->blt(x, y, *src, srcRect->toIntRect(), opacity); )

	return mrb_nil_value();
}

MRB_METHOD(bitmapStretchBlt)
{
	Bitmap *b = getPrivateData<Bitmap>(mrb, self);

	mrb_value destRectObj;
	mrb_value srcObj;
	mrb_value srcRectObj;
	mrb_int opacity = 255;

	Bitmap *src;
	Rect *destRect, *srcRect;

	mrb_get_args(mrb, "ooo|i", &destRectObj, &srcObj, &srcRectObj);

	src = getPrivateDataCheck<Bitmap>(mrb, srcObj, BitmapType);
	destRect = getPrivateDataCheck<Rect>(mrb, destRectObj, RectType);
	srcRect = getPrivateDataCheck<Rect>(mrb, srcRectObj, RectType);

	GUARD_EXC( b->stretchBlt(destRect->toIntRect(), *src, srcRect->toIntRect(), opacity); )

	return mrb_nil_value();
}

MRB_METHOD(bitmapFillRect)
{
	Bitmap *b = getPrivateData<Bitmap>(mrb, self);

	mrb_value colorObj;
	Color *color;

	if (mrb->c->ci->argc == 2)
	{
		mrb_value rectObj;
		Rect *rect;

		mrb_get_args(mrb, "oo", &rectObj, &colorObj);

		rect = getPrivateDataCheck<Rect>(mrb, rectObj, RectType);
		color = getPrivateDataCheck<Color>(mrb, colorObj, ColorType);

		GUARD_EXC( b->fillRect(rect->toIntRect(), color->norm); )
	}
	else
	{
		mrb_int x, y, width, height;

		mrb_get_args(mrb, "iiiio", &x, &y, &width, &height, &colorObj);

		color = getPrivateDataCheck<Color>(mrb, colorObj, ColorType);

		GUARD_EXC( b->fillRect(x, y, width, height, color->norm); )
	}

	return mrb_nil_value();
}

MRB_METHOD(bitmapClear)
{
	Bitmap *b = getPrivateData<Bitmap>(mrb, self);

	GUARD_EXC( b->clear(); )

	return mrb_nil_value();
}

MRB_METHOD(bitmapGetPixel)
{
	Bitmap *b = getPrivateData<Bitmap>(mrb, self);

	mrb_int x, y;

	mrb_get_args(mrb, "ii", &x, &y);

	Color value;
	GUARD_EXC( value = b->getPixel(x, y); )

	Color *color = new Color(value);

	return wrapObject(mrb, color, ColorType);
}

MRB_METHOD(bitmapSetPixel)
{
	Bitmap *b = getPrivateData<Bitmap>(mrb, self);

	mrb_int x, y;
	mrb_value colorObj;

	Color *color;

	mrb_get_args(mrb, "iio", &x, &y, &colorObj);

	color = getPrivateDataCheck<Color>(mrb, colorObj, ColorType);

	GUARD_EXC( b->setPixel(x, y, *color); )

	return mrb_nil_value();
}

MRB_METHOD(bitmapHueChange)
{
	Bitmap *b = getPrivateData<Bitmap>(mrb, self);

	mrb_int hue;

	mrb_get_args(mrb, "i", &hue);

	GUARD_EXC( b->hueChange(hue); )

	return mrb_nil_value();
}

MRB_METHOD(bitmapDrawText)
{
	Bitmap *b = getPrivateData<Bitmap>(mrb, self);

	char *str;
	mrb_int align = Bitmap::Left;

	int argc = mrb->c->ci->argc;
	if (argc == 2 || argc == 3)
	{
		mrb_value rectObj;
		Rect *rect;

		mrb_get_args(mrb, "oz|i", &rectObj, &str, &align);

		rect = getPrivateDataCheck<Rect>(mrb, rectObj, RectType);

		GUARD_EXC( b->drawText(rect->toIntRect(), str, align); )
	}
	else
	{
		mrb_int x, y, width, height;

		mrb_get_args(mrb, "iiiiz|i", &x, &y, &width, &height, &str, &align);

		GUARD_EXC( b->drawText(x, y, width, height, str, align); )
	}

	return mrb_nil_value();
}

MRB_METHOD(bitmapTextSize)
{
	Bitmap *b = getPrivateData<Bitmap>(mrb, self);

	char *str;

	mrb_get_args(mrb, "z", &str);

	IntRect value;
	GUARD_EXC( value = b->textSize(str); )

	Rect *rect = new Rect(value);

	return wrapObject(mrb, rect, RectType);
}

MRB_METHOD(bitmapGetFont)
{
	checkDisposed<Bitmap>(mrb, self);

	return getProperty(mrb, self, CSfont);
}

MRB_METHOD(bitmapSetFont)
{
	Bitmap *b = getPrivateData<Bitmap>(mrb, self);

	mrb_value fontObj;
	Font *font;

	mrb_get_args(mrb, "o", &fontObj);

	font = getPrivateDataCheck<Font>(mrb, fontObj, FontType);

	GUARD_EXC( b->setFont(*font); )

	return mrb_nil_value();
}

INITCOPY_FUN(Bitmap)


void
bitmapBindingInit(mrb_state *mrb)
{
	RClass *klass = defineClass(mrb, "Bitmap");

	disposableBindingInit<Bitmap>(mrb, klass);

	mrb_define_method(mrb, klass, "initialize",      bitmapInitialize,     MRB_ARGS_REQ(1) | MRB_ARGS_OPT(1));
	mrb_define_method(mrb, klass, "initialize_copy", BitmapInitializeCopy, MRB_ARGS_REQ(1));

	mrb_define_method(mrb, klass, "width",       bitmapWidth,      MRB_ARGS_NONE());
	mrb_define_method(mrb, klass, "height",      bitmapHeight,     MRB_ARGS_NONE());
	mrb_define_method(mrb, klass, "rect",        bitmapRect,       MRB_ARGS_NONE());
	mrb_define_method(mrb, klass, "blt",         bitmapBlt,        MRB_ARGS_REQ(4) | MRB_ARGS_OPT(1));
	mrb_define_method(mrb, klass, "stretch_blt", bitmapStretchBlt, MRB_ARGS_REQ(3) | MRB_ARGS_OPT(1));
	mrb_define_method(mrb, klass, "fill_rect",   bitmapFillRect,   MRB_ARGS_REQ(2) | MRB_ARGS_OPT(2));
	mrb_define_method(mrb, klass, "clear",       bitmapClear,      MRB_ARGS_NONE());
	mrb_define_method(mrb, klass, "get_pixel",   bitmapGetPixel,   MRB_ARGS_REQ(2));
	mrb_define_method(mrb, klass, "set_pixel",   bitmapSetPixel,   MRB_ARGS_REQ(3));
	mrb_define_method(mrb, klass, "hue_change",  bitmapHueChange,  MRB_ARGS_REQ(1));
	mrb_define_method(mrb, klass, "draw_text",   bitmapDrawText,   MRB_ARGS_REQ(2) | MRB_ARGS_OPT(4));
	mrb_define_method(mrb, klass, "text_size",   bitmapTextSize,   MRB_ARGS_REQ(1));

	mrb_define_method(mrb, klass, "font",        bitmapGetFont,    MRB_ARGS_NONE());
	mrb_define_method(mrb, klass, "font=",       bitmapSetFont,    MRB_ARGS_REQ(1));

	mrb_define_method(mrb, klass, "inspect",     inspectObject,    MRB_ARGS_NONE());
}
