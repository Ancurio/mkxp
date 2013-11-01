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
#include "disposable-binding.h"
#include "binding-util.h"
#include "binding-types.h"

#include <QDebug>

#define DISP_CLASS_NAME "bitmap"

DEF_TYPE(Bitmap);

RB_METHOD(bitmapInitialize)
{
	Bitmap *b = 0;

	if (argc == 1)
	{
		char *filename;
		rb_get_args(argc, argv, "z", &filename, RB_ARG_END);

		GUARD_EXC( b = new Bitmap(filename); )
	}
	else
	{
		int width, height;
		rb_get_args(argc, argv, "ii", &width, &height, RB_ARG_END);

		GUARD_EXC( b = new Bitmap(width, height); )
	}

	setPrivateData(self, b);

	/* Wrap properties */
	Font *font = new Font();
	b->setFont(font);
	font->setColor(new Color(*font->getColor()));

	VALUE fontProp = wrapProperty(self, font, "font", FontType);
	wrapProperty(fontProp, font->getColor(), "color", ColorType);

	return self;
}

RB_METHOD(bitmapWidth)
{
	RB_UNUSED_PARAM;

	Bitmap *b = getPrivateData<Bitmap>(self);

	int value = 0;
	GUARD_EXC( value = b->width(); );

	return INT2FIX(value);
}

RB_METHOD(bitmapHeight)
{
	RB_UNUSED_PARAM;

	Bitmap *b = getPrivateData<Bitmap>(self);

	int value = 0;
	GUARD_EXC( value = b->height(); );

	return INT2FIX(value);
}

RB_METHOD(bitmapRect)
{
	RB_UNUSED_PARAM;

	Bitmap *b = getPrivateData<Bitmap>(self);

	IntRect rect;
	GUARD_EXC( rect = b->rect(); );

	Rect *r = new Rect(rect);

	return wrapObject(r, RectType);
}

RB_METHOD(bitmapBlt)
{
	Bitmap *b = getPrivateData<Bitmap>(self);

	int x, y;
	VALUE srcObj;
	VALUE srcRectObj;
	int opacity = 255;

	Bitmap *src;
	Rect *srcRect;

	rb_get_args(argc, argv, "iioo|i", &x, &y, &srcObj, &srcRectObj, &opacity, RB_ARG_END);

	src = getPrivateDataCheck<Bitmap>(srcObj, BitmapType);
	srcRect = getPrivateDataCheck<Rect>(srcRectObj, RectType);

	GUARD_EXC( b->blt(x, y, *src, srcRect->toIntRect(), opacity); );

	return self;
}

RB_METHOD(bitmapStretchBlt)
{
	Bitmap *b = getPrivateData<Bitmap>(self);

	VALUE destRectObj;
	VALUE srcObj;
	VALUE srcRectObj;
	int opacity = 255;

	Bitmap *src;
	Rect *destRect, *srcRect;

	rb_get_args(argc, argv, "ooo|i", &destRectObj, &srcObj, &srcRectObj, &opacity, RB_ARG_END);

	src = getPrivateDataCheck<Bitmap>(srcObj, BitmapType);
	destRect = getPrivateDataCheck<Rect>(destRectObj, RectType);
	srcRect = getPrivateDataCheck<Rect>(srcRectObj, RectType);

	GUARD_EXC( b->stretchBlt(destRect->toIntRect(), *src, srcRect->toIntRect(), opacity); );

	return self;
}

RB_METHOD(bitmapFillRect)
{
	Bitmap *b = getPrivateData<Bitmap>(self);

	VALUE colorObj;
	Color *color;

	if (argc == 2)
	{
		VALUE rectObj;
		Rect *rect;

		rb_get_args(argc, argv, "oo", &rectObj, &colorObj, RB_ARG_END);

		rect = getPrivateDataCheck<Rect>(rectObj, RectType);
		color = getPrivateDataCheck<Color>(colorObj, ColorType);

		GUARD_EXC( b->fillRect(rect->toIntRect(), color->norm); );
	}
	else
	{
		int x, y, width, height;

		rb_get_args(argc, argv, "iiiio", &x, &y, &width, &height, &colorObj, RB_ARG_END);

		color = getPrivateDataCheck<Color>(colorObj, ColorType);

		GUARD_EXC( b->fillRect(x, y, width, height, color->norm); );
	}

	return self;
}

RB_METHOD(bitmapClear)
{
	RB_UNUSED_PARAM;

	Bitmap *b = getPrivateData<Bitmap>(self);

	GUARD_EXC( b->clear(); )

	return self;
}

RB_METHOD(bitmapGetPixel)
{
	Bitmap *b = getPrivateData<Bitmap>(self);

	int x, y;

	rb_get_args(argc, argv, "ii", &x, &y, RB_ARG_END);

	GUARD_EXC(
		if (x < 0 || y < 0 || x >= b->width() || y >= b->height())
	            return Qnil;
	         )

	Vec4 value;
	GUARD_EXC( value = b->getPixel(x, y); );

	Color *color = new Color(value);

	return wrapObject(color, ColorType);
}

RB_METHOD(bitmapSetPixel)
{
	Bitmap *b = getPrivateData<Bitmap>(self);

	int x, y;
	VALUE colorObj;

	Color *color;

	rb_get_args(argc, argv, "iio", &x, &y, &colorObj, RB_ARG_END);

	color = getPrivateDataCheck<Color>(colorObj, ColorType);

	GUARD_EXC( b->setPixel(x, y, color->norm); );

	return self;
}

RB_METHOD(bitmapHueChange)
{
	Bitmap *b = getPrivateData<Bitmap>(self);

	int hue;

	rb_get_args(argc, argv, "i", &hue, RB_ARG_END);

	GUARD_EXC( b->hueChange(hue); );

	return self;
}

RB_METHOD(bitmapDrawText)
{
	Bitmap *b = getPrivateData<Bitmap>(self);

	const char *str;
	int align = Bitmap::Left;

	if (argc == 2 || argc == 3)
	{
		VALUE rectObj;
		Rect *rect;

		rb_get_args(argc, argv, "oz|i", &rectObj, &str, &align, RB_ARG_END);

		rect = getPrivateDataCheck<Rect>(rectObj, RectType);

		GUARD_EXC( b->drawText(rect->toIntRect(), str, align); );
	}
	else
	{
		int x, y, width, height;

		rb_get_args(argc, argv, "iiiiz|i", &x, &y, &width, &height, &str, &align, RB_ARG_END);

		GUARD_EXC( b->drawText(x, y, width, height, str, align); );
	}

	return self;
}

RB_METHOD(bitmapTextSize)
{
	Bitmap *b = getPrivateData<Bitmap>(self);

	const char *str;

	rb_get_args(argc, argv, "z", &str, RB_ARG_END);

	IntRect value;
	GUARD_EXC( value = b->textSize(str); );

	Rect *rect = new Rect(value);

	return wrapObject(rect, RectType);
}

DEF_PROP_OBJ(Bitmap, Font, Font, "font")

// FIXME: This isn't entire correct as the cloned bitmap
// does not get a cloned version of the original bitmap's 'font'
// attribute (the internal font attrb is the default one, whereas
// the stored iv visible to ruby would still be the same as the original)
// Not sure if this needs fixing though
INITCOPY_FUN(Bitmap)


void
bitmapBindingInit()
{
	INIT_TYPE(Bitmap);

	VALUE klass = rb_define_class("Bitmap", rb_cObject);
	rb_define_alloc_func(klass, classAllocate<&BitmapType>);

	disposableBindingInit<Bitmap>(klass);

	_rb_define_method(klass, "initialize",      bitmapInitialize);
	_rb_define_method(klass, "initialize_copy", BitmapInitializeCopy);

	_rb_define_method(klass, "width",       bitmapWidth);
	_rb_define_method(klass, "height",      bitmapHeight);
	_rb_define_method(klass, "rect",        bitmapRect);
	_rb_define_method(klass, "blt",         bitmapBlt);
	_rb_define_method(klass, "stretch_blt", bitmapStretchBlt);
	_rb_define_method(klass, "fill_rect",   bitmapFillRect);
	_rb_define_method(klass, "clear",       bitmapClear);
	_rb_define_method(klass, "get_pixel",   bitmapGetPixel);
	_rb_define_method(klass, "set_pixel",   bitmapSetPixel);
	_rb_define_method(klass, "hue_change",  bitmapHueChange);
	_rb_define_method(klass, "draw_text",   bitmapDrawText);
	_rb_define_method(klass, "text_size",   bitmapTextSize);

	INIT_PROP_BIND(Bitmap, Font, "font");
}
