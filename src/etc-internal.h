/*
** etc-internal.h
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

#ifndef ETC_TYPES_H
#define ETC_TYPES_H

#include "util.h"

struct Vec2
{
	float x, y;

	Vec2()
	    : x(0), y(0)
	{}

	Vec2(float x, float y)
	    : x(x), y(y)
	{}

	bool operator==(const Vec2 &other) const
	{
		return (x == other.x && y == other.y);
	}
};

struct Vec4
{
	float x, y, z, w;

	Vec4()
	    : x(0), y(0), z(0), w(0)
	{}

	Vec4(float x, float y, float z, float w)
	    : x(x), y(y), z(z), w(w)
	{}

	bool operator==(const Vec4 &other) const
	{
		return (x == other.x && y == other.y && z == other.z && w == other.w);
	}
};

struct Vec2i
{
	int x, y;

	Vec2i()
	    : x(0), y(0)
	{}

	Vec2i(int x, int y)
	    : x(x), y(y)
	{}

	bool operator==(const Vec2i &other)
	{
		return x == other.x && y == other.y;
	}
};

/* Simple Vertex */
struct SVertex
{
	Vec2 pos;
	Vec2 texPos;

	static const void *posOffset()
	{
		return (const void*) 0;
	}

	static const void *texPosOffset()
	{
		return (const void*) sizeof(Vec2);
	}
};

/* Color Vertex */
struct CVertex
{
	Vec2 pos;
	Vec4 color;

	CVertex()
	    : color(1, 1, 1, 1)
	{}

	static const void *posOffset()
	{
		return (const void*) 0;
	}

	static const void *colorOffset()
	{
		return (const void*) sizeof(pos);
	}
};

struct Vertex
{
	Vec2 pos;
	Vec2 texPos;
	Vec4 color;

	Vertex()
	    : color(1, 1, 1, 1)
	{}

	static const void *posOffset()
	{
		return (const void*) 0;
	}

	static const void *texPosOffset()
	{
		return (const void*) sizeof(Vec2);
	}

	static const void *colorOffset()
	{
		return (const void*) sizeof(SVertex);
	}
};

struct IntRect
{
	int x, y, w, h;

	IntRect()
	    : x(0), y(0), w(0), h(0)
	{}

	IntRect(int x, int y, int w, int h)
	    : x(x), y(y), w(w), h(h)
	{}

	bool operator==(const IntRect &other) const
	{
		return (x == other.x && y == other.y &&
		        w == other.w && h == other.h);
	}
};

struct StaticRect { float x, y, w, h; };

struct FloatRect
{
	float x, y, w, h;

	FloatRect()
	    : x(0), y(0), w(0), h(0)
	{}

	FloatRect(float x, float y, float w, float h)
	    : x(x), y(y), w(w), h(h)
	{}

	FloatRect(const StaticRect &d)
	    : x(d.x), y(d.y), w(d.w), h(d.h)
	{}

	FloatRect(const IntRect &r)
	    : x(r.x), y(r.y), w(r.w), h(r.h)
	{}

	operator IntRect() const
	{
		return IntRect(x, y, w, h);
	}

	Vec2 topLeft() const { return Vec2(x, y); }
	Vec2 bottomLeft() const { return Vec2(x, y+h); }
	Vec2 topRight() const { return Vec2(x+w, y); }
	Vec2 bottomRight() const { return Vec2(x+w, y+h); }

	void shrinkHalf()
	{
		x += 0.5;
		y += 0.5;
		w -= 1.0;
		h -= 1.0;
	}

	FloatRect vFlipped() const
	{
		return FloatRect(x, y+h, w, -h);
	}

	FloatRect hFlipped() const
	{
		return FloatRect(x+w, y, -w, h);
	}

	Vec2 corner(int i) const
	{
		switch (i)
		{
		case 0 : return topLeft();
		case 1 : return topRight();
		case 2 : return bottomRight();
		case 3 : return bottomLeft();
		default : return Vec2();
		}
	}
};

/* Value between 0 and 255 with internal
 * normalized representation */
struct NormValue
{
	int unNorm;
	float norm;

	NormValue()
	    : unNorm(0),
	      norm(0)
	{}

	NormValue(int unNorm)
	    : unNorm(unNorm),
	      norm(unNorm / 255.0)
	{}

	void operator =(int value)
	{
		unNorm = bound(value, 0, 255);
		norm = unNorm / 255.0;
	}

	operator int() const
	{
		return unNorm;
	}
};

#endif // ETC_TYPES_H
