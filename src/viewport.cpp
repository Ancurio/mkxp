/*
** viewport.cpp
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

#include "viewport.h"

#include "sharedstate.h"
#include "etc.h"
#include "util.h"
#include "quad.h"
#include "glstate.h"
#include "graphics.h"

#include <SDL_rect.h>

#include <sigc++/connection.h>

struct ViewportPrivate
{
	/* Needed for geometry changes */
	Viewport *self;

	Rect *rect;
	sigc::connection rectCon;

	Color *color;
	Tone *tone;

	IntRect screenRect;
	int isOnScreen;

	EtcTemps tmp;

	ViewportPrivate(int x, int y, int width, int height, Viewport *self)
	    : self(self),
	      rect(&tmp.rect),
	      color(&tmp.color),
	      tone(&tmp.tone),
	      isOnScreen(false)
	{
		rect->set(x, y, width, height);
		updateRectCon();
	}

	~ViewportPrivate()
	{
		rectCon.disconnect();
	}

	void onRectChange()
	{
		self->geometry.rect = rect->toIntRect();
		self->notifyGeometryChange();
		recomputeOnScreen();
	}

	void updateRectCon()
	{
		rectCon.disconnect();
		rectCon = rect->valueChanged.connect
		        (sigc::mem_fun(this, &ViewportPrivate::onRectChange));
	}

	void recomputeOnScreen()
	{
		SDL_Rect r1 = { screenRect.x, screenRect.y,
		                screenRect.w, screenRect.h };

		SDL_Rect r2 = { rect->x,     rect->y,
		                rect->width, rect->height };

		SDL_Rect result;
		isOnScreen = SDL_IntersectRect(&r1, &r2, &result);
	}

	bool needsEffectRender(bool flashing)
	{
		bool rectEffective = !rect->isEmpty();
		bool colorToneEffective = color->hasEffect() || tone->hasEffect() || flashing;

		return (rectEffective && colorToneEffective && isOnScreen);
	}
};

Viewport::Viewport(int x, int y, int width, int height)
    : SceneElement(*shState->screen()),
      sceneLink(this)
{
	initViewport(x, y, width, height);
}

Viewport::Viewport(Rect *rect)
    : SceneElement(*shState->screen()),
      sceneLink(this)
{
	initViewport(rect->x, rect->y, rect->width, rect->height);
}

Viewport::Viewport()
    : SceneElement(*shState->screen()),
      sceneLink(this)
{
	const Graphics &graphics = shState->graphics();
	initViewport(0, 0, graphics.width(), graphics.height());
}

void Viewport::initViewport(int x, int y, int width, int height)
{
	p = new ViewportPrivate(x, y, width, height, this);

	/* Set our own geometry */
	geometry.rect = IntRect(x, y, width, height);

	/* Handle parent geometry */
	onGeometryChange(scene->getGeometry());
}

Viewport::~Viewport()
{
	unlink();

	delete p;
}

DEF_ATTR_RD_SIMPLE(Viewport, OX,   int,   geometry.xOrigin)
DEF_ATTR_RD_SIMPLE(Viewport, OY,   int,   geometry.yOrigin)
DEF_ATTR_RD_SIMPLE(Viewport, Rect, Rect*, p->rect)

DEF_ATTR_SIMPLE(Viewport, Color, Color*, p->color)
DEF_ATTR_SIMPLE(Viewport, Tone, Tone*, p->tone)

void Viewport::setOX(int value)
{
	if (geometry.xOrigin == value)
		return;

	geometry.xOrigin = value;
	notifyGeometryChange();
}

void Viewport::setOY(int value)
{
	if (geometry.yOrigin == value)
		return;

	geometry.yOrigin = value;
	notifyGeometryChange();
}

void Viewport::setRect(Rect *value)
{
	if (p->rect == value)
		return;

	p->rect = value;
	p->updateRectCon();
	p->onRectChange();
}

/* Scene */
void Viewport::composite()
{
	if (emptyFlashFlag)
		return;

	bool renderEffect = p->needsEffectRender(flashing);

	if (elements.getSize() == 0 && !renderEffect)
		return;

	/* Setup scissor */
	glState.scissorTest.pushSet(true);
	glState.scissorBox.pushSet(p->rect->toIntRect());

	Scene::composite();

	/* If any effects are visible, request parent Scene to
	 * render them. */
	if (renderEffect)
		scene->requestViewportRender
		        (p->color->norm, flashColor, p->tone->norm);

	glState.scissorBox.pop();
	glState.scissorTest.pop();
}

/* SceneElement */
void Viewport::draw()
{
	composite();
}

void Viewport::onGeometryChange(const Geometry &geo)
{
	p->screenRect = geo.rect;
	p->recomputeOnScreen();
}


ViewportElement::ViewportElement(Viewport *viewport, int z, bool isSprite)
    : SceneElement(viewport ? *viewport : *shState->screen(), z, isSprite),
      m_viewport(viewport)
{}

ViewportElement::ViewportElement(Viewport *viewport, int z, unsigned int cStamp)
    : SceneElement(viewport ? *viewport : *shState->screen(), z, cStamp)
{}

Viewport *ViewportElement::getViewport() const
{
	return m_viewport;
}

void ViewportElement::setViewport(Viewport *viewport)
{
	m_viewport = viewport;
	setScene(viewport ? *viewport : *shState->screen());
	onViewportChange();
	onGeometryChange(scene->getGeometry());
}
