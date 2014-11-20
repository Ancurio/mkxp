/*
** etc.cpp
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

#include "etc.h"

#include "serial-util.h"
#include "exception.h"

#include <SDL_types.h>
#include <SDL_pixels.h>

Color::Color(double red, double green, double blue, double alpha)
	: red(red), green(green), blue(blue), alpha(alpha)
{
	updateInternal();
}

Color::Color(const Vec4 &norm)
	: norm(norm)
{
	updateExternal();
}

Color::Color(const Color &o)
    : red(o.red), green(o.green), blue(o.blue), alpha(o.alpha),
      norm(o.norm)
{}

bool Color::operator==(const Color &o) const
{
	return red   == o.red   &&
	       green == o.green &&
	       blue  == o.blue  &&
	       alpha == o.alpha;
}

const Color &Color::operator=(const Color &o)
{
	red   = o.red;
	green = o.green;
	blue  = o.blue;
	alpha = o.alpha;
	norm  = o.norm;

	return o;
}

void Color::set(double red, double green, double blue, double alpha)
{
	this->red   = red;
	this->green = green;
	this->blue  = blue;
	this->alpha = alpha;

	updateInternal();
}

void Color::setRed(double value)
{
	red = value;
	norm.x = clamp<double>(value, 0, 255) / 255;
}

void Color::setGreen(double value)
{
	green = value;
	norm.y = clamp<double>(value, 0, 255) / 255;
}

void Color::setBlue(double value)
{
	blue = value;
	norm.z = clamp<double>(value, 0, 255) / 255;
}

void Color::setAlpha(double value)
{
	alpha = value;
	norm.w = clamp<double>(value, 0, 255) / 255;
}

/* Serializable */
int Color::serialSize() const
{
	return 4 * 8;
}

void Color::serialize(char *buffer) const
{
	writeDouble(&buffer, red);
	writeDouble(&buffer, green);
	writeDouble(&buffer, blue);
	writeDouble(&buffer, alpha);
}

Color *Color::deserialize(const char *data, int len)
{
	if (len != 32)
		throw Exception(Exception::ArgumentError, "Color: Serialized data invalid");

	Color *c = new Color();

	c->red   = readDouble(&data);
	c->green = readDouble(&data);
	c->blue  = readDouble(&data);
	c->alpha = readDouble(&data);
	c->updateInternal();

	return c;
}

void Color::updateInternal()
{
	norm.x = red   / 255;
	norm.y = green / 255;
	norm.z = blue  / 255;
	norm.w = alpha / 255;
}

void Color::updateExternal()
{
	red   = norm.x * 255;
	green = norm.y * 255;
	blue  = norm.z * 255;
	alpha = norm.w * 255;
}

SDL_Color Color::toSDLColor() const
{
	SDL_Color c;
	c.r = clamp<double>(red, 0, 255);
	c.g = clamp<double>(green, 0, 255);
	c.b = clamp<double>(blue, 0, 255);
	c.a = clamp<double>(alpha, 0, 255);

	return c;
}


Tone::Tone(double red, double green, double blue, double gray)
	: red(red), green(green), blue(blue), gray(gray)
{
	updateInternal();
}

Tone::Tone(const Tone &o)
    : red(o.red), green(o.green), blue(o.blue), gray(o.gray),
      norm(o.norm)
{}

bool Tone::operator==(const Tone &o) const
{
	return red   == o.red   &&
	       green == o.green &&
	       blue  == o.blue  &&
	       gray  == o.gray;
}

void Tone::set(double red, double green, double blue, double gray)
{
	this->red   = red;
	this->green = green;
	this->blue  = blue;
	this->gray  = gray;

	updateInternal();
	valueChanged();
}

const Tone& Tone::operator=(const Tone &o)
{
	red   = o.red;
	green = o.green;
	blue  = o.blue;
	gray  = o.gray;
	norm  = o.norm;

	valueChanged();

	return o;
}

void Tone::setRed(double value)
{
	red = value;
	norm.x = (float) clamp<double>(value, -255, 255) / 255;

	valueChanged();
}

void Tone::setGreen(double value)
{
	green = value;
	norm.y = (float) clamp<double>(value, -255, 255) / 255;

	valueChanged();
}

void Tone::setBlue(double value)
{
	blue = value;
	norm.z = (float) clamp<double>(value, -255, 255) / 255;

	valueChanged();
}

void Tone::setGray(double value)
{
	gray = value;
	norm.w = (float) clamp<double>(value, 0, 255) / 255;

	valueChanged();
}

/* Serializable */
int Tone::serialSize() const
{
	return 4 * 8;
}

void Tone::serialize(char *buffer) const
{
	writeDouble(&buffer, red);
	writeDouble(&buffer, green);
	writeDouble(&buffer, blue);
	writeDouble(&buffer, gray);
}

Tone *Tone::deserialize(const char *data, int len)
{
	if (len != 32)
		throw Exception(Exception::ArgumentError, "Tone: Serialized data invalid");

	Tone *t = new Tone();

	t->red   = readDouble(&data);
	t->green = readDouble(&data);
	t->blue  = readDouble(&data);
	t->gray  = readDouble(&data);
	t->updateInternal();

	return t;
}

void Tone::updateInternal()
{
	norm.x = (float) clamp<double>(red,   -255, 255) / 255;
	norm.y = (float) clamp<double>(green, -255, 255) / 255;
	norm.z = (float) clamp<double>(blue,  -255, 255) / 255;
	norm.w = (float) clamp<double>(gray,     0, 255) / 255;
}


Rect::Rect(int x, int y, int width, int height)
    : x(x), y(y), width(width), height(height)
{}

Rect::Rect(const Rect &o)
    : x(o.x), y(o.y),
      width(o.width), height(o.height)
{}

Rect::Rect(const IntRect &r)
    : x(r.x), y(r.y), width(r.w), height(r.h)
{}

bool Rect::operator==(const Rect &o) const
{
	return x      == o.x     &&
	       y      == o.y     &&
	       width  == o.width &&
	       height == o.height;
}

void Rect::operator=(const IntRect &rect)
{
	x = rect.x;
	y = rect.y;
	width = rect.w;
	height = rect.h;
}

void Rect::set(int x, int y, int w, int h)
{
	if (this->x == x &&
	    this->y == y &&
	    width   == w &&
	    height  == h)
	{
		return;
	}

	this->x = x;
	this->y = y;
	width = w;
	height = h;
	valueChanged();
}

const Rect &Rect::operator=(const Rect &o)
{
	x      = o.x;
	y      = o.y;
	width  = o.width;
	height = o.height;

	valueChanged();

	return o;
}

void Rect::empty()
{
	if (!(x || y || width || height))
		return;

	x = y = width = height = 0;
	valueChanged();
}

bool Rect::isEmpty() const
{
	return !(width && height);
}

void Rect::setX(int value)
{
	if (x == value)
		return;

	x = value;
	valueChanged();
}

void Rect::setY(int value)
{
	if (y == value)
		return;

	y = value;
	valueChanged();
}

void Rect::setWidth(int value)
{
	if (width == value)
		return;

	width = value;
	valueChanged();
}

void Rect::setHeight(int value)
{
	if (height == value)
		return;

	height = value;
	valueChanged();
}

int Rect::serialSize() const
{
	return 4 * 4;
}

void Rect::serialize(char *buffer) const
{
	writeInt32(&buffer, x);
	writeInt32(&buffer, y);
	writeInt32(&buffer, width);
	writeInt32(&buffer, height);
}

Rect *Rect::deserialize(const char *data, int len)
{
	if (len != 16)
		throw Exception(Exception::ArgumentError, "Rect: Serialized data invalid");

	Rect *r = new Rect();

	r->x      = readInt32(&data);
	r->y      = readInt32(&data);
	r->width  = readInt32(&data);
	r->height = readInt32(&data);

	return r;
}
