/*
** window-vx.h
**
** This file is part of mkxp.
**
** Copyright (C) 2021 Amaryllis Kulla <amaryllis.kulla@protonmail.com>
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

#ifndef WINDOWVX_H
#define WINDOWVX_H

#include "viewport.h"
#include "disposable.h"

#include "util.h"

class Bitmap;
struct Rect;
struct Tone;

struct WindowVXPrivate;

class WindowVX : public ViewportElement, public Disposable
{
public:
	WindowVX(Viewport *viewport = 0);
	WindowVX(int x, int y, int width, int height);
	~WindowVX();

	void update();

	void move(int x, int y, int width, int height);
	bool isOpen() const;
	bool isClosed() const;

	DECL_ATTR( Windowskin,      Bitmap* )
	DECL_ATTR( Contents,        Bitmap* )
	DECL_ATTR( CursorRect,      Rect&   )
	DECL_ATTR( Active,          bool    )
	DECL_ATTR( ArrowsVisible,   bool    )
	DECL_ATTR( Pause,           bool    )
	DECL_ATTR( X,               int     )
	DECL_ATTR( Y,               int     )
	DECL_ATTR( Width,           int     )
	DECL_ATTR( Height,          int     )
	DECL_ATTR( OX,              int     )
	DECL_ATTR( OY,              int     )
	DECL_ATTR( Padding,         int     )
	DECL_ATTR( PaddingBottom,   int     )
	DECL_ATTR( Opacity,         int     )
	DECL_ATTR( BackOpacity,     int     )
	DECL_ATTR( ContentsOpacity, int     )
	DECL_ATTR( Openness,        int     )
	DECL_ATTR( Tone,            Tone&   )

	void initDynAttribs();

private:
	WindowVXPrivate *p;

	void draw();
	void onGeometryChange(const Scene::Geometry &);

	void releaseResources();
	const char *klassName() const { return "window"; }

	ABOUT_TO_ACCESS_DISP
};

#endif // WINDOWVX_H
