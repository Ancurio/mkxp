/*
** plane.h
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

#ifndef PLANE_H
#define PLANE_H

#include "disposable.h"
#include "viewport.h"

class Bitmap;
struct Color;
struct Tone;

struct PlanePrivate;

class Plane : public ViewportElement, public Disposable
{
public:
	Plane(Viewport *viewport = 0);
	~Plane();

	DECL_ATTR( Bitmap,    Bitmap* )
	DECL_ATTR( OX,        int     )
	DECL_ATTR( OY,        int     )
	DECL_ATTR( ZoomX,     float   )
	DECL_ATTR( ZoomY,     float   )
	DECL_ATTR( Opacity,   int     )
	DECL_ATTR( BlendType, int     )
	DECL_ATTR( Color,     Color&  )
	DECL_ATTR( Tone,      Tone&   )

	void initDynAttribs();

private:
	PlanePrivate *p;

	void draw();
	void onGeometryChange(const Scene::Geometry &);

	void releaseResources();
	const char *klassName() const { return "plane"; }

	ABOUT_TO_ACCESS_DISP
};

#endif // PLANE_H
