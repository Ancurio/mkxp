/*
** viewport.h
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

#ifndef VIEWPORT_H
#define VIEWPORT_H

#include "scene.h"
#include "flashable.h"
#include "disposable.h"
#include "util.h"

struct ViewportPrivate;

class Viewport : public Scene, public SceneElement, public Flashable, public Disposable
{
public:
	Viewport(int x, int y, int width, int height);
	Viewport(Rect *rect);
	Viewport();
	~Viewport();

	void update();

	DECL_ATTR( Rect,  Rect&  )
	DECL_ATTR( OX,    int    )
	DECL_ATTR( OY,    int    )
	DECL_ATTR( Color, Color& )
	DECL_ATTR( Tone,  Tone&  )

	void initDynAttribs();

private:
	void initViewport(int x, int y, int width, int height);
	void geometryChanged();

	void composite();
	void draw();
	void onGeometryChange(const Geometry &);
	bool isEffectiveViewport(Rect *&, Color *&, Tone *&) const;

	void releaseResources();
	const char *klassName() const { return "viewport"; }

	ABOUT_TO_ACCESS_DISP

	ViewportPrivate *p;
	friend struct ViewportPrivate;

	IntruListLink<Scene> sceneLink;
};

class ViewportElement : public SceneElement
{
public:
	ViewportElement(Viewport *viewport = 0, int z = 0, int spriteY = 0);

	DECL_ATTR( Viewport,  Viewport* )

protected:
	virtual void onViewportChange() {}

private:
	Viewport *m_viewport;
};

#endif // VIEWPORT_H
