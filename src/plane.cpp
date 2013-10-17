/*
** plane.cpp
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

#include "plane.h"

#include "sharedstate.h"
#include "bitmap.h"
#include "etc.h"
#include "util.h"

#include "gl-util.h"
#include "quad.h"
#include "transform.h"
#include "etc-internal.h"
#include "shader.h"
#include "glstate.h"

#include <sigc++/connection.h>

struct PlanePrivate
{
	Bitmap *bitmap;
	NormValue opacity;
	BlendType blendType;
	Color *color;
	Tone *tone;

	int ox, oy;
	float zoomX, zoomY;

	Scene::Geometry sceneGeo;

	bool quadSourceDirty;

	Quad quad;

	EtcTemps tmp;

	sigc::connection prepareCon;

	PlanePrivate()
	    : bitmap(0),
	      opacity(255),
	      blendType(BlendNormal),
	      color(&tmp.color),
	      tone(&tmp.tone),
	      ox(0), oy(0),
	      zoomX(1), zoomY(1),
	      quadSourceDirty(false)
	{
		prepareCon = shState->prepareDraw.connect
		        (sigc::mem_fun(this, &PlanePrivate::prepare));
	}

	~PlanePrivate()
	{
		prepareCon.disconnect();
	}

	void updateQuadSource()
	{
		FloatRect srcRect;
		srcRect.x = (sceneGeo.yOrigin + ox) / zoomX;
		srcRect.y = (sceneGeo.xOrigin + oy) / zoomY;
		srcRect.w = sceneGeo.rect.w / zoomX;
		srcRect.h = sceneGeo.rect.h / zoomY;

		quad.setTexRect(srcRect);
	}

	void prepare()
	{
		if (quadSourceDirty)
		{
			updateQuadSource();
			quadSourceDirty = false;
		}

		if (bitmap)
			bitmap->flush();
	}
};

Plane::Plane(Viewport *viewport)
    : ViewportElement(viewport)
{
	p = new PlanePrivate();

	onGeometryChange(scene->getGeometry());
}

#define DISP_CLASS_NAME "plane"

DEF_ATTR_RD_SIMPLE(Plane, Bitmap,    Bitmap*, p->bitmap)
DEF_ATTR_RD_SIMPLE(Plane, OX,        int,     p->ox)
DEF_ATTR_RD_SIMPLE(Plane, OY,        int,     p->oy)
DEF_ATTR_RD_SIMPLE(Plane, ZoomX,     float,   p->zoomX)
DEF_ATTR_RD_SIMPLE(Plane, ZoomY,     float,   p->zoomY)
DEF_ATTR_RD_SIMPLE(Plane, BlendType, int,     p->blendType)

DEF_ATTR_SIMPLE(Plane, Opacity, int,     p->opacity)
DEF_ATTR_SIMPLE(Plane, Color,   Color*,  p->color)
DEF_ATTR_SIMPLE(Plane, Tone,    Tone*,   p->tone)

Plane::~Plane()
{
	dispose();
}

void Plane::setBitmap(Bitmap *value)
{
	GUARD_DISPOSED;

	p->bitmap = value;

	if (!value)
		return;

	value->ensureNonMega();
}

void Plane::setOX(int value)
{
	GUARD_DISPOSED

	p->ox = value;
	p->quadSourceDirty = true;
}

void Plane::setOY(int value)
{
	GUARD_DISPOSED

	p->oy = value;
	p->quadSourceDirty = true;
}

void Plane::setZoomX(float value)
{
	GUARD_DISPOSED

	p->zoomX = value;
	p->quadSourceDirty = true;
}

void Plane::setZoomY(float value)
{
	GUARD_DISPOSED

	p->zoomY = value;
	p->quadSourceDirty = true;
}

void Plane::setBlendType(int value)
{
	GUARD_DISPOSED

	switch (value)
	{
	default :
	case BlendNormal :
		p->blendType = BlendNormal;
		return;
	case BlendAddition :
		p->blendType = BlendAddition;
		return;
	case BlendSubstraction :
		p->blendType = BlendSubstraction;
		return;
	}
}


void Plane::draw()
{
	if (!p->bitmap)
		return;

	if (p->bitmap->isDisposed())
		return;

	if (!p->opacity)
		return;

	ShaderBase *base;

	if (p->color->hasEffect() || p->tone->hasEffect() || p->opacity != 255)
	{
		PlaneShader &shader = shState->planeShader();

		shader.bind();
		shader.applyViewportProj();
		shader.setTone(p->tone->norm);
		shader.setColor(p->color->norm);
		shader.setFlash(Vec4());
		shader.setOpacity(p->opacity.norm);

		base = &shader;
	}
	else
	{
		SimpleShader &shader = shState->simpleShader();

		shader.bind();
		shader.applyViewportProj();
		shader.setTranslation(Vec2i());

		base = &shader;
	}

	glState.blendMode.pushSet(p->blendType);

	p->bitmap->bindTex(*base);
	TEX::setRepeat(true);

	p->quad.draw();

	TEX::setRepeat(false);

	glState.blendMode.pop();
}

void Plane::onGeometryChange(const Scene::Geometry &geo)
{
	p->quad.setPosRect(FloatRect(geo.rect));

	p->sceneGeo = geo;
	p->quadSourceDirty = true;
}

void Plane::aboutToAccess() const
{
	GUARD_DISPOSED
}


void Plane::releaseResources()
{
	unlink();

	delete p;
}
