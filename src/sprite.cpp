/*
** sprite.cpp
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

#include "sprite.h"

#include "globalstate.h"
#include "bitmap.h"
#include "etc.h"
#include "etc-internal.h"
#include "util.h"

#include "gl-util.h"
#include "quad.h"
#include "transform.h"
#include "shader.h"
#include "glstate.h"

#include "sigc++/connection.h"

#include <QDebug>

struct SpritePrivate
{
	Bitmap *bitmap;

	Quad quad;
	Transform trans;

	Rect *srcRect;
	sigc::connection srcRectCon;

	bool mirrored;
	int bushDepth;
	float efBushDepth;
	NormValue bushOpacity;
	NormValue opacity;
	BlendType blendType;

	Color *color;
	Tone *tone;

	EtcTemps tmp;

	SpritePrivate()
	    : bitmap(0),
	      srcRect(&tmp.rect),
	      mirrored(false),
	      bushDepth(0),
	      efBushDepth(0),
	      bushOpacity(128),
	      opacity(255),
	      blendType(BlendNormal),
	      color(&tmp.color),
	      tone(&tmp.tone)

	{
		updateSrcRectCon();
	}

	~SpritePrivate()
	{
		srcRectCon.disconnect();
	}

	void recomputeBushDepth()
	{
		/* Calculate effective (normalized) bush depth */
		float texBushDepth = (bushDepth / trans.getScale().y) -
		                     (srcRect->y + srcRect->height) +
		                     bitmap->height();

		efBushDepth = 1.0 - texBushDepth / bitmap->height();
	}

	void onSrcRectChange()
	{
		if (mirrored)
			quad.setTexRect(srcRect->toFloatRect().hFlipped());
		else
			quad.setTexRect(srcRect->toFloatRect());

		quad.setPosRect(IntRect(0, 0, srcRect->width, srcRect->height));
		recomputeBushDepth();
	}

	void updateSrcRectCon()
	{
		/* Cut old connection */
		srcRectCon.disconnect();
		/* Create new one */
		srcRectCon = srcRect->valueChanged.connect
				(sigc::mem_fun(this, &SpritePrivate::onSrcRectChange));
	}
};

Sprite::Sprite(Viewport *viewport)
	: ViewportElement(viewport)
{
	p = new SpritePrivate;
	onGeometryChange(scene->getGeometry());
}

Sprite::~Sprite()
{
	dispose();
}

#define DISP_CLASS_NAME "sprite"

DEF_ATTR_RD_SIMPLE(Sprite, Bitmap,    Bitmap*, p->bitmap)
DEF_ATTR_RD_SIMPLE(Sprite, SrcRect,   Rect*,   p->srcRect)
DEF_ATTR_RD_SIMPLE(Sprite, X,         int,     p->trans.getPosition().x)
DEF_ATTR_RD_SIMPLE(Sprite, Y,         int,     p->trans.getPosition().y)
DEF_ATTR_RD_SIMPLE(Sprite, OX,        int,     p->trans.getOrigin().x)
DEF_ATTR_RD_SIMPLE(Sprite, OY,        int,     p->trans.getOrigin().y)
DEF_ATTR_RD_SIMPLE(Sprite, ZoomX,     float,   p->trans.getScale().x)
DEF_ATTR_RD_SIMPLE(Sprite, ZoomY,     float,   p->trans.getScale().y)
DEF_ATTR_RD_SIMPLE(Sprite, Angle,     float,   p->trans.getRotation())
DEF_ATTR_RD_SIMPLE(Sprite, Mirror,    bool,    p->mirrored)
DEF_ATTR_RD_SIMPLE(Sprite, BushDepth, int,     p->bushDepth)
DEF_ATTR_RD_SIMPLE(Sprite, BlendType, int,     p->blendType)
DEF_ATTR_RD_SIMPLE(Sprite, Width,     int,     p->srcRect->width)
DEF_ATTR_RD_SIMPLE(Sprite, Height,    int,     p->srcRect->height)

DEF_ATTR_SIMPLE(Sprite, BushOpacity, int,    p->bushOpacity)
DEF_ATTR_SIMPLE(Sprite, Opacity,     int,    p->opacity)
DEF_ATTR_SIMPLE(Sprite, Color,       Color*, p->color)
DEF_ATTR_SIMPLE(Sprite, Tone,        Tone*,  p->tone)

void Sprite::setBitmap(Bitmap *bitmap)
{
	GUARD_DISPOSED

	if (p->bitmap == bitmap)
		return;

	bitmap->ensureNonMega();

	p->bitmap = bitmap;
	*p->srcRect = bitmap->rect();
	p->onSrcRectChange();
	p->quad.setPosRect(p->srcRect->toFloatRect());
}

void Sprite::setSrcRect(Rect *rect)
{
	GUARD_DISPOSED

	if (p->srcRect == rect)
		return;

	p->srcRect = rect;
	p->updateSrcRectCon();

	if (p->bitmap)
		p->onSrcRectChange();
}

void Sprite::setX(int value)
{
	GUARD_DISPOSED

	if (p->trans.getPosition().x == value)
		return;

	p->trans.setPosition(Vec2(value, getY()));
}

void Sprite::setY(int value)
{
	GUARD_DISPOSED

	if (p->trans.getPosition().y == value)
		return;

	p->trans.setPosition(Vec2(getX(), value));
}

void Sprite::setOX(int value)
{
	GUARD_DISPOSED

	if (p->trans.getOrigin().x == value)
		return;

	p->trans.setOrigin(Vec2(value, getOY()));
}

void Sprite::setOY(int value)
{
	GUARD_DISPOSED

	if (p->trans.getOrigin().y == value)
		return;

	p->trans.setOrigin(Vec2(getOX(), value));
}

void Sprite::setZoomX(float value)
{
	GUARD_DISPOSED

	if (p->trans.getScale().x == value)
		return;

	p->trans.setScale(Vec2(value, getZoomY()));
}

void Sprite::setZoomY(float value)
{
	GUARD_DISPOSED

	if (p->trans.getScale().y == value)
		return;

	p->trans.setScale(Vec2(getZoomX(), value));
	p->recomputeBushDepth();
}

void Sprite::setAngle(float value)
{
	GUARD_DISPOSED

	if (p->trans.getRotation() == value)
		return;

	p->trans.setRotation(value);
}

void Sprite::setMirror(bool mirrored)
{
	GUARD_DISPOSED

	if (p->mirrored == mirrored)
		return;

	p->mirrored = mirrored;
	p->onSrcRectChange();
}

void Sprite::setBushDepth(int value)
{
	GUARD_DISPOSED

	if (p->bushDepth == value)
		return;

	p->bushDepth = value;
	p->recomputeBushDepth();
}

void Sprite::setBlendType(int type)
{
	GUARD_DISPOSED

	switch (type)
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

/* Disposable */
void Sprite::releaseResources()
{
	unlink();

	delete p;
}

/* SceneElement */
void Sprite::draw()
{
	if (!p->bitmap)
		return;

	if (p->bitmap->isDisposed())
		return;

	if (emptyFlashFlag)
		return;

	p->bitmap->flush();

	ShaderBase *base;

	bool renderEffect = p->color->hasEffect() ||
	                    p->tone->hasEffect()  ||
	                    p->opacity != 255     ||
	                    flashing              ||
	                    p->bushDepth != 0;

	if (renderEffect)
	{
		SpriteShader &shader = gState->spriteShader();

		shader.bind();
		shader.applyViewportProj();
		shader.setSpriteMat(p->trans.getMatrix());

		shader.setTone(p->tone->norm);
		shader.setOpacity(p->opacity.norm);
		shader.setBushDepth(p->efBushDepth);
		shader.setBushOpacity(p->bushOpacity.norm);

		/* When both flashing and effective color are set,
		 * the one with higher alpha will be blended */
		const Vec4 *blend = (flashing && flashColor.w > p->color->norm.w) ?
			                 &flashColor : &p->color->norm;

		shader.setColor(*blend);

		base = &shader;
	}
	else
	{
		SimpleSpriteShader &shader = gState->simpleSpriteShader();
		shader.bind();

		shader.setSpriteMat(p->trans.getMatrix());
		shader.applyViewportProj();
		base = &shader;
	}

	glState.blendMode.pushSet(p->blendType);

	p->bitmap->bindTex(*base);

	p->quad.draw();

	glState.blendMode.pop();
}

void Sprite::onGeometryChange(const Scene::Geometry &geo)
{
	/* Offset at which the sprite will be drawn
	 * relative to screen origin */
	int xOffset = geo.rect.x - geo.xOrigin;
	int yOffset = geo.rect.y - geo.yOrigin;

	p->trans.setGlobalOffset(xOffset, yOffset);
}
