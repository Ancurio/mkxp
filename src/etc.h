/*
** etc.h
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

#ifndef ETC_H
#define ETC_H

#include <sigc++/signal.h>

#include "serializable.h"
#include "etc-internal.h"

struct SDL_Color;

enum BlendType
{
	BlendKeepDestAlpha = -1,

	BlendNormal = 0,
	BlendAddition = 1,
	BlendSubstraction = 2
};

struct Color : public Serializable
{
	Color()
	    : red(0), green(0), blue(0), alpha(0)
	{}

	Color(double red, double green, double blue, double alpha = 255);
	Color(const Vec4 &norm);
	Color(const Color &o);

	virtual ~Color() {}

	const Color &operator=(const Color &o);
	void set(double red, double green, double blue, double alpha);

	bool operator==(const Color &o) const;

	void setRed(double value);
	void setGreen(double value);
	void setBlue(double value);
	void setAlpha(double value);

	double getRed()   const { return red;   }
	double getGreen() const { return green; }
	double getBlue()  const { return blue;  }
	double getAlpha() const { return alpha; }

	/* Serializable */
	int serialSize() const;
	void serialize(char *buffer) const;
	static Color *deserialize(const char *data, int len);

	/* Internal */
	void updateInternal();
	void updateExternal();

	bool hasEffect() const
	{
		return (alpha != 0);
	}

	SDL_Color toSDLColor() const;

	/* Range (0.0 ~ 255.0) */
	double red;
	double green;
	double blue;
	double alpha;

	/* Normalized (0.0 ~ 1.0) */
	Vec4 norm;
};

struct Tone : public Serializable
{
	Tone()
	    : red(0), green(0), blue(0), gray(0)
	{}

	Tone(double red, double green, double blue, double gray = 0);
	Tone(const Tone &o);

	virtual ~Tone() {}

	bool operator==(const Tone &o) const;

	void set(double red, double green, double blue, double gray);
	const Tone &operator=(const Tone &o);

	void setRed(double value);
	void setGreen(double value);
	void setBlue(double value);
	void setGray(double value);

	double getRed()   const { return red;   }
	double getGreen() const { return green; }
	double getBlue()  const { return blue;  }
	double getGray()  const { return gray;  }

	/* Serializable */
	int serialSize() const;
	void serialize(char *buffer) const;
	static Tone *deserialize(const char *data, int len);

	/* Internal */
	void updateInternal();

	bool hasEffect() const
	{
		return ((int)red   != 0 ||
				(int)green != 0 ||
				(int)blue  != 0 ||
				(int)gray  != 0);
	}

	/* Range (-255.0 ~ 255.0) */
	double red;
	double green;
	double blue;
	/* Range (0.0 ~ 255.0) */
	double gray;

	/* Normalized (-1.0 ~ 1.0) */
	Vec4 norm;

	sigc::signal<void> valueChanged;
};

struct Rect : public Serializable
{
	Rect()
	    : x(0), y(0), width(0), height(0)
	{}

	virtual ~Rect() {}

	Rect(int x, int y, int width, int height);
	Rect(const Rect &o);
	Rect(const IntRect &r);

	bool operator==(const Rect &o) const;
	void operator=(const IntRect &rect);
	void set(int x, int y, int w, int h);
	const Rect &operator=(const Rect &o);

	void empty();
	bool isEmpty() const;

	void setX(int value);
	void setY(int value);
	void setWidth(int value);
	void setHeight(int value);

	int getX() const { return x; }
	int getY() const { return y; }
	int getWidth() const { return width; }
	int getHeight() const { return height; }

	/* Serializable */
	int serialSize() const;
	void serialize(char *buffer) const;
	static Rect *deserialize(const char *data, int len);

	/* Internal */
	FloatRect toFloatRect() const
	{
		return FloatRect(x, y, width, height);
	}

	IntRect toIntRect()
	{
		return IntRect(x, y, width, height);
	}

	int x;
	int y;
	int width;
	int height;

	sigc::signal<void> valueChanged;
};

/* For internal use.
 * All drawable classes have properties of one or more of the above
 * types, which in an interpreted environment act as independent
 * objects, and rely on the GC to clean them up. When a drawable
 * class is constructed, these properties must have default objects
 * that are constructed with the class. C++ however has no GC, so
 * there is no way to clean them up when used directly with it.
 * Therefore the default objects are first created embedded in the
 * drawable class (so they get destroyed automatically from within
 * C++ if no pointers were changed), and the binding then takes
 * care of properly allocating new, independent objects and replacing
 * the defaults. Thus both C++ and the interpreted API can be used
 * without memory leakage.
 * This can be removed at a later point when no testing directly
 * from C++ is needed anymore. */
struct EtcTemps
{
	Color color;
	Tone tone;
	Rect rect;
};

#endif // ETC_H
