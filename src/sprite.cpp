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

#include "sharedstate.h"
#include "bitmap.h"
#include "etc.h"
#include "etc-internal.h"
#include "util.h"

#include "gl-util.h"
#include "quad.h"
#include "transform.h"
#include "shader.h"
#include "glstate.h"
#include "quadarray.h"

#include <math.h>
#ifndef M_PI
# define M_PI 3.14159265358979323846
#endif

#include <SDL_rect.h>

#include <sigc++/connection.h>

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

	IntRect sceneRect;
	Vec2i sceneOrig;

	/* Would this sprite be visible on
	 * the screen if drawn? */
	bool isVisible;

	Color *color;
	Tone *tone;

	struct
	{
		int amp;
		int length;
		int speed;
		float phase;

		/* Wave effect is active (amp != 0) */
		bool active;
		/* qArray needs updating */
		bool dirty;
		SimpleQuadArray qArray;
	} wave;

	EtcTemps tmp;

	sigc::connection prepareCon;

	SpritePrivate()
	    : bitmap(0),
	      srcRect(&tmp.rect),
	      mirrored(false),
	      bushDepth(0),
	      efBushDepth(0),
	      bushOpacity(128),
	      opacity(255),
	      blendType(BlendNormal),
	      isVisible(false),
	      color(&tmp.color),
	      tone(&tmp.tone)

	{
		sceneRect.x = sceneRect.y = 0;

		updateSrcRectCon();

		prepareCon = shState->prepareDraw.connect
		        (sigc::mem_fun(this, &SpritePrivate::prepare));

		wave.amp = 0;
		wave.length = 180;
		wave.speed = 360;
		wave.phase = 0.0f;
		wave.dirty = false;
	}

	~SpritePrivate()
	{
		srcRectCon.disconnect();
		prepareCon.disconnect();
	}

	void recomputeBushDepth()
	{
		if (nullOrDisposed(bitmap))
			return;

		/* Calculate effective (normalized) bush depth */
		float texBushDepth = (bushDepth / trans.getScale().y) -
		                     (srcRect->y + srcRect->height) +
		                     bitmap->height();

		efBushDepth = 1.0f - texBushDepth / bitmap->height();
	}

	void onSrcRectChange()
	{
		FloatRect rect = srcRect->toFloatRect();
		Vec2i bmSize;

		if (!nullOrDisposed(bitmap))
			bmSize = Vec2i(bitmap->width(), bitmap->height());

		/* Clamp the rectangle so it doesn't reach outside
		 * the bitmap bounds */
		rect.w = clamp<int>(rect.w, 0, bmSize.x-rect.x);
		rect.h = clamp<int>(rect.h, 0, bmSize.y-rect.y);

		quad.setTexRect(mirrored ? rect.hFlipped() : rect);

		quad.setPosRect(FloatRect(0, 0, rect.w, rect.h));
		recomputeBushDepth();

		wave.dirty = true;
	}

	void updateSrcRectCon()
	{
		/* Cut old connection */
		srcRectCon.disconnect();
		/* Create new one */
		srcRectCon = srcRect->valueChanged.connect
				(sigc::mem_fun(this, &SpritePrivate::onSrcRectChange));
	}

	void updateVisibility()
	{
		isVisible = false;

		if (nullOrDisposed(bitmap))
			return;

		if (!opacity)
			return;

		if (wave.active)
		{
			/* Don't do expensive wave bounding box
			 * calculations */
			isVisible = true;
			return;
		}

		/* Compare sprite bounding box against the scene */

		/* If sprite is zoomed/rotated, just opt out for now
		 * for simplicity's sake */
		const Vec2 &scale = trans.getScale();
		if (scale.x != 1 || scale.y != 1 || trans.getRotation() != 0)
		{
			isVisible = true;
			return;
		}

		IntRect self;
		self.setPos(trans.getPositionI() - (trans.getOriginI() + sceneOrig));
		self.w = bitmap->width();
		self.h = bitmap->height();

		isVisible = SDL_HasIntersection(&self, &sceneRect);
	}

	void emitWaveChunk(SVertex *&vert, float phase, int width,
	                   float zoomY, int chunkY, int chunkLength)
	{
		float wavePos = phase + (chunkY / (float) wave.length) * (float) (M_PI * 2);
		float chunkX = sin(wavePos) * wave.amp;

		FloatRect tex(0, chunkY / zoomY, width, chunkLength / zoomY);
		FloatRect pos = tex;
		pos.x = chunkX;

		Quad::setTexPosRect(vert, mirrored ? tex.hFlipped() : tex, pos);
		vert += 4;
	}

	void updateWave()
	{
		if (nullOrDisposed(bitmap))
			return;

		if (wave.amp == 0)
		{
			wave.active = false;
			return;
		}

		wave.active = true;

		int width = srcRect->width;
		int height = srcRect->height;
		float zoomY = trans.getScale().y;

		if (wave.amp < -(width / 2))
		{
			wave.qArray.resize(0);
			wave.qArray.commit();

			return;
		}

		/* RMVX does this, and I have no fucking clue why */
		if (wave.amp < 0)
		{
			wave.qArray.resize(1);

			int x = -wave.amp;
			int w = width - x * 2;

			FloatRect tex(x, srcRect->y, w, srcRect->height);

			Quad::setTexPosRect(&wave.qArray.vertices[0], tex, tex);
			wave.qArray.commit();

			return;
		}

		/* The length of the sprite as it appears on screen */
		int visibleLength = height * zoomY;

		/* First chunk length (aligned to 8 pixel boundary */
		int firstLength = ((int) trans.getPosition().y) % 8;

		/* Amount of full 8 pixel chunks in the middle */
		int chunks = (visibleLength - firstLength) / 8;

		/* Final chunk length */
		int lastLength = (visibleLength - firstLength) % 8;

		wave.qArray.resize(!!firstLength + chunks + !!lastLength);
		SVertex *vert = &wave.qArray.vertices[0];

		float phase = (wave.phase * (float) M_PI) / 180.0f;

		if (firstLength > 0)
			emitWaveChunk(vert, phase, width, zoomY, 0, firstLength);

		for (int i = 0; i < chunks; ++i)
			emitWaveChunk(vert, phase, width, zoomY, firstLength + i * 8, 8);

		if (lastLength > 0)
			emitWaveChunk(vert, phase, width, zoomY, firstLength + chunks * 8, lastLength);

		wave.qArray.commit();
	}

	void prepare()
	{
		if (wave.dirty)
		{
			updateWave();
			wave.dirty = false;
		}

		updateVisibility();
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

DEF_ATTR_RD_SIMPLE(Sprite, Bitmap,     Bitmap*, p->bitmap)
DEF_ATTR_RD_SIMPLE(Sprite, X,          int,     p->trans.getPosition().x)
DEF_ATTR_RD_SIMPLE(Sprite, Y,          int,     p->trans.getPosition().y)
DEF_ATTR_RD_SIMPLE(Sprite, OX,         int,     p->trans.getOrigin().x)
DEF_ATTR_RD_SIMPLE(Sprite, OY,         int,     p->trans.getOrigin().y)
DEF_ATTR_RD_SIMPLE(Sprite, ZoomX,      float,   p->trans.getScale().x)
DEF_ATTR_RD_SIMPLE(Sprite, ZoomY,      float,   p->trans.getScale().y)
DEF_ATTR_RD_SIMPLE(Sprite, Angle,      float,   p->trans.getRotation())
DEF_ATTR_RD_SIMPLE(Sprite, Mirror,     bool,    p->mirrored)
DEF_ATTR_RD_SIMPLE(Sprite, BushDepth,  int,     p->bushDepth)
DEF_ATTR_RD_SIMPLE(Sprite, BlendType,  int,     p->blendType)
DEF_ATTR_RD_SIMPLE(Sprite, Width,      int,     p->srcRect->width)
DEF_ATTR_RD_SIMPLE(Sprite, Height,     int,     p->srcRect->height)
DEF_ATTR_RD_SIMPLE(Sprite, WaveAmp,    int,     p->wave.amp)
DEF_ATTR_RD_SIMPLE(Sprite, WaveLength, int,     p->wave.length)
DEF_ATTR_RD_SIMPLE(Sprite, WaveSpeed,  int,     p->wave.speed)
DEF_ATTR_RD_SIMPLE(Sprite, WavePhase,  float,   p->wave.phase)

DEF_ATTR_SIMPLE(Sprite, BushOpacity, int,     p->bushOpacity)
DEF_ATTR_SIMPLE(Sprite, Opacity,     int,     p->opacity)
DEF_ATTR_SIMPLE(Sprite, SrcRect,     Rect&,  *p->srcRect)
DEF_ATTR_SIMPLE(Sprite, Color,       Color&, *p->color)
DEF_ATTR_SIMPLE(Sprite, Tone,        Tone&,  *p->tone)

void Sprite::setBitmap(Bitmap *bitmap)
{
	guardDisposed();

	if (p->bitmap == bitmap)
		return;

	p->bitmap = bitmap;

	if (nullOrDisposed(bitmap))
		return;

	bitmap->ensureNonMega();

	*p->srcRect = bitmap->rect();
	p->onSrcRectChange();
	p->quad.setPosRect(p->srcRect->toFloatRect());

	p->wave.dirty = true;
}

void Sprite::setX(int value)
{
	guardDisposed();

	if (p->trans.getPosition().x == value)
		return;

	p->trans.setPosition(Vec2(value, getY()));
}

void Sprite::setY(int value)
{
	guardDisposed();

	if (p->trans.getPosition().y == value)
		return;

	p->trans.setPosition(Vec2(getX(), value));

	if (rgssVer >= 2)
	{
		p->wave.dirty = true;
		setSpriteY(value);
	}
}

void Sprite::setOX(int value)
{
	guardDisposed();

	if (p->trans.getOrigin().x == value)
		return;

	p->trans.setOrigin(Vec2(value, getOY()));
}

void Sprite::setOY(int value)
{
	guardDisposed();

	if (p->trans.getOrigin().y == value)
		return;

	p->trans.setOrigin(Vec2(getOX(), value));
}

void Sprite::setZoomX(float value)
{
	guardDisposed();

	if (p->trans.getScale().x == value)
		return;

	p->trans.setScale(Vec2(value, getZoomY()));
}

void Sprite::setZoomY(float value)
{
	guardDisposed();

	if (p->trans.getScale().y == value)
		return;

	p->trans.setScale(Vec2(getZoomX(), value));
	p->recomputeBushDepth();

	if (rgssVer >= 2)
		p->wave.dirty = true;
}

void Sprite::setAngle(float value)
{
	guardDisposed();

	if (p->trans.getRotation() == value)
		return;

	p->trans.setRotation(value);
}

void Sprite::setMirror(bool mirrored)
{
	guardDisposed();

	if (p->mirrored == mirrored)
		return;

	p->mirrored = mirrored;
	p->onSrcRectChange();
}

void Sprite::setBushDepth(int value)
{
	guardDisposed();

	if (p->bushDepth == value)
		return;

	p->bushDepth = value;
	p->recomputeBushDepth();
}

void Sprite::setBlendType(int type)
{
	guardDisposed();

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

#define DEF_WAVE_SETTER(Name, name, type) \
	void Sprite::setWave##Name(type value) \
	{ \
		guardDisposed(); \
		if (p->wave.name == value) \
			return; \
		p->wave.name = value; \
		p->wave.dirty = true; \
	}

DEF_WAVE_SETTER(Amp,    amp,    int)
DEF_WAVE_SETTER(Length, length, int)
DEF_WAVE_SETTER(Speed,  speed,  int)
DEF_WAVE_SETTER(Phase,  phase,  float)

#undef DEF_WAVE_SETTER

void Sprite::initDynAttribs()
{
	p->srcRect = new Rect;
	p->color = new Color;
	p->tone = new Tone;

	p->updateSrcRectCon();
}

/* Flashable */
void Sprite::update()
{
	guardDisposed();

	Flashable::update();

	p->wave.phase += p->wave.speed / 180;
	p->wave.dirty = true;
}

/* SceneElement */
void Sprite::draw()
{
	if (!p->isVisible)
		return;

	if (emptyFlashFlag)
		return;

	ShaderBase *base;

	bool renderEffect = p->color->hasEffect() ||
	                    p->tone->hasEffect()  ||
	                    flashing              ||
	                    p->bushDepth != 0;

	if (renderEffect)
	{
		SpriteShader &shader = shState->shaders().sprite;

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
	else if (p->opacity != 255)
	{
		AlphaSpriteShader &shader = shState->shaders().alphaSprite;
		shader.bind();

		shader.setSpriteMat(p->trans.getMatrix());
		shader.setAlpha(p->opacity.norm);
		shader.applyViewportProj();
		base = &shader;
	}
	else
	{
		SimpleSpriteShader &shader = shState->shaders().simpleSprite;
		shader.bind();

		shader.setSpriteMat(p->trans.getMatrix());
		shader.applyViewportProj();
		base = &shader;
	}

	glState.blendMode.pushSet(p->blendType);

	p->bitmap->bindTex(*base);

	if (p->wave.active)
		p->wave.qArray.draw();
	else
		p->quad.draw();

	glState.blendMode.pop();
}

void Sprite::onGeometryChange(const Scene::Geometry &geo)
{
	/* Offset at which the sprite will be drawn
	 * relative to screen origin */
	p->trans.setGlobalOffset(geo.offset());

	p->sceneRect.setSize(geo.rect.size());
	p->sceneOrig = geo.orig;
}

void Sprite::releaseResources()
{
	unlink();

	delete p;
}
