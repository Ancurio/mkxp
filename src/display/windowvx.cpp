/*
** window-vx.cpp
**
** This file is part of mkxp.
**
** Copyright (C) 2014 Jonas Kulla <Nyocurio@gmail.com>
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

#include "windowvx.h"

#include "bitmap.h"
#include "etc.h"
#include "etc-internal.h"
#include "quad.h"
#include "quadarray.h"
#include "sharedstate.h"
#include "texpool.h"
#include "tilequad.h"
#include "glstate.h"
#include "shader.h"

#include <limits>
#include <algorithm>
#include "sigslot/signal.hpp"

#define DEF_Z         (rgssVer >= 3 ? 100 :   0)
#define DEF_PADDING   (rgssVer >= 3 ?  12 :  16)
#define DEF_BACK_OPAC (rgssVer >= 3 ? 192 : 255)
#define DEF_SPRITE_Y  (rgssVer >= 3 ? std::numeric_limits<int>::max() : 0) /* See scene.h */

template<typename T>
struct Sides
{
	T l, r, t, b;
};

template<typename T>
struct Corners
{
	T tl, tr, bl, br;
};

/* Offsetting this by one gives a great visual improvement
 * in Majo no Ie */
static const IntRect bgStretchSrc( 1,  1, 62, 62 );
static const IntRect bgTileSrc   ( 0, 64, 64, 64 );

static const Corners<IntRect> cornerSrc =
{
	IntRect(  64,  0, 16, 16 ),
	IntRect( 112,  0, 16, 16 ),
	IntRect(  64, 48, 16, 16 ),
	IntRect( 112, 48, 16, 16 )
};

static const Sides<IntRect> borderSrc =
{
	IntRect(  64, 16, 16, 32 ),
	IntRect( 112, 16, 16, 32 ),
	IntRect(  80,  0, 32, 16 ),
	IntRect(  80, 48, 32, 16 )
};

static const Sides<IntRect> scrollArrowSrc =
{
	IntRect(  80, 24,  8, 16 ),
	IntRect( 104, 24,  8, 16 ),
	IntRect(  88, 16, 16,  8 ),
	IntRect(  88, 40, 16,  8 )
};

static const IntRect pauseSrc[4] =
{
	IntRect(  96, 64, 16, 16 ),
	IntRect( 112, 64, 16, 16 ),
	IntRect(  96, 80, 16, 16 ),
	IntRect( 112, 80, 16, 16 )
};

struct CursorSrc
{
	Corners<IntRect> corners;
	Sides<IntRect> border;
	IntRect bg;
};

static const CursorSrc cursorSrc =
{
	{
		IntRect( 64, 64, 4, 4 ),
		IntRect( 92, 64, 4, 4 ),
		IntRect( 64, 92, 4, 4 ),
		IntRect( 92, 92, 4, 4 )
	},
	{
		IntRect( 64, 68,  4, 24 ),
		IntRect( 92, 68,  4, 24 ),
		IntRect( 68, 64, 24,  4 ),
		IntRect( 68, 92, 24,  4 )
	},
	IntRect( 68, 68, 24, 24 )
};

static const uint8_t cursorAlpha[] =
{
	/* Fade out */
	0xFF, 0xF7, 0xEF, 0xE7, 0xDF, 0xD7, 0xCF, 0xC7, 0xBF, 0xB7,
	0xAF, 0xA7, 0x9F, 0x97, 0x8F, 0x87, 0x7F, 0x77, 0x6F, 0x67,
	/* Fade in */
	0x5F, 0x67, 0x6F, 0x77, 0x7F, 0x87, 0x8F, 0x97, 0x9F, 0xA7,
	0xAF, 0xB7, 0xBF, 0xC7, 0xCF, 0xD7, 0xDF, 0xE7, 0xEF, 0xF7
};

static elementsN(cursorAlpha);

static const uint8_t cursorAlphaResetIdx = 0x10;

/* No cycle */
static const uint8_t pauseAlpha[] =
{
	0x00, 0x20, 0x40, 0x60, 0x80, 0xA0, 0xC0, 0xE0,
	0xFF
};

static elementsN(pauseAlpha);

/* Cycling */
static const uint8_t pauseQuad[] =
{
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3
};

static elementsN(pauseQuad);

struct WindowVXPrivate
{
	Bitmap *windowskin;

	Bitmap *contents;

	Rect *cursorRect;
	bool active;
	bool arrowsVisible;
	bool pause;

	/* Externally visible width/height */
	int width, height;

	/* Window geometry (x, Y, width, height)
	 * with non-negative size */
	IntRect geo;

	/* ox / oy */
	Vec2i contentsOff;

	int padding;
	int paddingBottom;

	NormValue opacity;
	NormValue backOpacity;
	NormValue contentsOpacity;

	NormValue openness;
	Tone *tone;

	sigslot::connection cursorRectCon;
	sigslot::connection toneCon;
	sigslot::connection prepareCon;

	EtcTemps tmp;

	struct
	{
		TEXFBO tex;
		ColorQuadArray vert;
		size_t bgTileQuads;
		size_t borderQuads;
		Quad quad;

		bool vertDirty;
		bool texSizeDirty;
		bool texDirty;
	} base;

	ColorQuadArray ctrlVert;
	size_t ctrlQuads;
	Vertex *pauseVert;

	Quad contentsQuad;

	IntRect padRect;
	IntRect clipRect;

	ColorQuadArray cursorVert;

	bool ctrlVertDirty;
	bool ctrlVertArrayDirty;
	bool clipRectDirty;
	bool cursorVertDirty;
	bool cursorVertArrayDirty;

	uint8_t pauseAlphaIdx;
	uint8_t pauseQuadIdx;
	uint8_t cursorAlphaIdx;

	Vec2i sceneOffset;

	WindowVXPrivate(int x, int y, int w, int h)
	    : windowskin(0),
	      contents(0),
	      cursorRect(&tmp.rect),
	      active(true),
	      arrowsVisible(true),
	      pause(false),
	      width(w),
	      height(h),
	      geo(x, y, w, h),
	      padding(DEF_PADDING),
	      paddingBottom(padding),
	      opacity(255),
	      backOpacity(DEF_BACK_OPAC),
	      contentsOpacity(255),
	      openness(255),
	      tone(&tmp.tone),
	      ctrlVertDirty(false),
	      ctrlVertArrayDirty(false),
	      clipRectDirty(false),
	      cursorVertDirty(false),
	      cursorVertArrayDirty(false),
	      pauseAlphaIdx(0),
	      pauseQuadIdx(0),
	      cursorAlphaIdx(0)
	{
		/* 4 scroll arrows + pause */
		ctrlVert.resize(4 + 1);
		pauseVert = &ctrlVert.vertices[4*4];

		base.vertDirty = false;
		base.texSizeDirty = false;
		base.texDirty = false;

		if (w > 0 || h > 0)
		{
			base.vertDirty = true;
			base.texSizeDirty = true;
			clipRectDirty = true;
			ctrlVertDirty = true;
		}

		prepareCon = shState->prepareDraw.connect
			(&WindowVXPrivate::prepare, this);

		refreshCursorRectCon();
		refreshToneCon();
		updateBaseQuad();
	}

	~WindowVXPrivate()
	{
		shState->texPool().release(base.tex);

		cursorRectCon.disconnect();
		toneCon.disconnect();
		prepareCon.disconnect();
	}

	void invalidateCursorVert()
	{
		cursorVertDirty = true;
	}

	void invalidateBaseTex()
	{
		base.texDirty = true;
	}

	void refreshCursorRectCon()
	{
		cursorRectCon.disconnect();
		cursorRectCon = cursorRect->valueChanged.connect
		        (&WindowVXPrivate::invalidateCursorVert, this);
	}

	void refreshToneCon()
	{
		toneCon.disconnect();
		toneCon = tone->valueChanged.connect
			(&WindowVXPrivate::invalidateBaseTex, this);
	}

	void updateBaseTexSize()
	{
		if (base.tex.width >= geo.w && base.tex.height >= geo.h)
			return;

		if (base.tex.tex != TEX::ID(0))
		{
			TEX::bind(base.tex.tex);
			TEX::setSmooth(false); // XXX make pool set this up at alloc time
		}

		shState->texPool().release(base.tex);
		TEXFBO::clear(base.tex);

		if (geo.w == 0 || geo.h == 0)
			return;

		base.tex = shState->texPool().request(geo.w, geo.h);
		TEX::bind(base.tex.tex);
		TEX::setSmooth(true);
	}

	void rebuildBaseVert()
	{
		if (geo.w == 0 || geo.h == 0)
		{
			base.vert.clear();
			base.vert.commit();

			return;
		}

		const IntRect bgPos(2, 2, geo.w-4, geo.h-4);
		size_t count = 0;

		/* Stretched layer (1) */
		count += 1;

		/* Tiled layer (2) */
		base.bgTileQuads = TileQuads::twoDimCount(bgTileSrc.w, bgTileSrc.h, bgPos.w, bgPos.h);
		count += base.bgTileQuads;

		const Vec2 corOff(geo.w - 16, geo.h - 16);

		const Corners<FloatRect> cornerPos =
		{
			FloatRect(        0,        0, 16, 16 ), /* Top left */
			FloatRect( corOff.x,        0, 16, 16 ), /* Top right */
			FloatRect(        0, corOff.y, 16, 16 ), /* Bottom left */
			FloatRect( corOff.x, corOff.y, 16, 16 )  /* Bottom right */
		};

		const Vec2i sideLen(geo.w - 16*2, geo.h - 16*2);

		bool drawSidesLR = sideLen.x > 0;
		bool drawSidesTB = sideLen.y > 0;

		base.borderQuads = 0;
		base.borderQuads += 4; /* 4 corners */

		if (drawSidesLR)
			base.borderQuads += TileQuads::oneDimCount(32, sideLen.y) * 2;

		if (drawSidesTB)
			base.borderQuads += TileQuads::oneDimCount(32, sideLen.x) * 2;

		count += base.borderQuads;

		base.vert.resize(count);

		Vertex *vert = dataPtr(base.vert.vertices);
		size_t i = 0;

		/* Stretched background */
		i += Quad::setTexPosRect(&vert[i*4], bgStretchSrc, bgPos);

		/* Tiled background */
		i += TileQuads::build(bgTileSrc, bgPos, &vert[i*4]);

		/* Corners */
		i += Quad::setTexPosRect(&vert[i*4], cornerSrc.tl, cornerPos.tl);
		i += Quad::setTexPosRect(&vert[i*4], cornerSrc.tr, cornerPos.tr);
		i += Quad::setTexPosRect(&vert[i*4], cornerSrc.bl, cornerPos.bl);
		i += Quad::setTexPosRect(&vert[i*4], cornerSrc.br, cornerPos.br);

		/* Sides */
		if (drawSidesLR)
		{
			i += TileQuads::buildV(borderSrc.l, sideLen.y,        0,       16, &vert[i*4]);
			i += TileQuads::buildV(borderSrc.r, sideLen.y, corOff.x,       16, &vert[i*4]);
		}

		if (drawSidesTB)
		{
			i += TileQuads::buildH(borderSrc.t, sideLen.x,       16,        0, &vert[i*4]);
			i += TileQuads::buildH(borderSrc.b, sideLen.x,       16, corOff.y, &vert[i*4]);
		}

		base.vert.commit();
	}

	void redrawBaseTex()
	{
		if (nullOrDisposed(windowskin))
			return;

		if (base.tex.tex == TEX::ID(0))
			return;

		FBO::bind(base.tex.fbo);

		/* Clear texture */
		glState.clearColor.pushSet(Vec4());
		FBO::clear();
		glState.clearColor.pop();

		glState.viewport.pushSet(IntRect(0, 0, base.tex.width, base.tex.height));
		glState.blend.pushSet(false);

		ShaderBase *shader;

		if (backOpacity < 255 || tone->hasEffect())
		{
			PlaneShader &planeShader = shState->shaders().plane;
			planeShader.bind();

			planeShader.setColor(Vec4());
			planeShader.setFlash(Vec4());
			planeShader.setTone(tone->norm);
			planeShader.setOpacity(backOpacity.norm);

			shader = &planeShader;
		}
		else
		{
			shader = &shState->shaders().simple;
			shader->bind();
		}

		windowskin->bindTex(*shader);
		TEX::setSmooth(true);

		shader->setTranslation(Vec2i());
		shader->applyViewportProj();

		/* Draw stretched layer */
		base.vert.draw(0, 1);

		glState.blend.set(true);
		glState.blendMode.pushSet(BlendKeepDestAlpha);

		/* Draw tiled layer */
		base.vert.draw(1, base.bgTileQuads);

		glState.blendMode.set(BlendNormal);

		/* If we used plane shader before, switch to simple */
		if (shader != &shState->shaders().simple)
		{
			shader = &shState->shaders().simple;
			shader->bind();
			shader->setTranslation(Vec2i());
			shader->applyViewportProj();
			windowskin->bindTex(*shader);
		}

		base.vert.draw(1+base.bgTileQuads, base.borderQuads);

		TEX::setSmooth(false);

		glState.blendMode.pop();
		glState.blend.pop();
		glState.viewport.pop();
	}

	void updateBaseQuad()
	{
		const FloatRect tex(0, 0, geo.w, geo.h);
		const FloatRect pos(0, (geo.h / 2.0f) * (1.0f - openness.norm),
		                    geo.w, geo.h * openness.norm);

		base.quad.setTexPosRect(tex, pos);
	}

	void updateClipRect()
	{
		SDL_Rect winRect = { 0, 0, geo.w, geo.h };

		padRect.x = padRect.y = padding;
		padRect.w = std::max(0, geo.w - padding*2);
		padRect.h = std::max(0, geo.h - (padding+paddingBottom));

		SDL_Rect tmp = padRect;

		SDL_IntersectRect(&winRect, &tmp, &tmp);
		clipRect = IntRect(tmp.x, tmp.y, tmp.w, tmp.h);

		ctrlVertDirty = true;
	}

	void rebuildCtrlVert()
	{
		/* Scroll arrow position: Top Bottom X, Left Right Y */
		const Vec2i arrow = (geo.size() - Vec2i(16)) / 2;

		const Sides<FloatRect> arrowPos =
		{
			FloatRect(          4,    arrow.y, 8, 16 ), /* Left */
			FloatRect( geo.w - 12,    arrow.y, 8, 16 ), /* Right */
			FloatRect(    arrow.x,          4, 16, 8 ), /* Top */
			FloatRect(    arrow.x, geo.h - 12, 16, 8 )  /* Bottom */
		};

		size_t i = 0;
		Vertex *vert = dataPtr(ctrlVert.vertices);

		if (!nullOrDisposed(contents) && arrowsVisible)
		{
			if (contentsOff.x > 0)
				i += Quad::setTexPosRect(&vert[i*4], scrollArrowSrc.l, arrowPos.l);
			if (contentsOff.y > 0)
				i += Quad::setTexPosRect(&vert[i*4], scrollArrowSrc.t, arrowPos.t);

			if (padRect.w < (contents->width() - contentsOff.x))
				i += Quad::setTexPosRect(&vert[i*4], scrollArrowSrc.r, arrowPos.r);

			if (padRect.h < (contents->height() - contentsOff.y))
				i += Quad::setTexPosRect(&vert[i*4], scrollArrowSrc.b, arrowPos.b);
		}

		pauseVert = 0;

		if (pause)
		{
			const FloatRect pausePos(arrow.x, geo.h - 16, 16, 16);
			pauseVert = &vert[i*4];

			i += Quad::setTexPosRect(&vert[i*4], pauseSrc[0], pausePos);
		}

		ctrlQuads = i;
		ctrlVertArrayDirty = true;
	}

	void rebuildCursorVert()
	{
		const IntRect rect = cursorRect->toIntRect();
		const CursorSrc &src = cursorSrc;

		cursorVertArrayDirty = true;

		if (rect.w <= 0 || rect.h <= 0)
		{
			cursorVert.clear();
			return;
		}

		const Vec2 corOff(rect.w - 4, rect.h - 4);

		const Corners<FloatRect> cornerPos =
		{
			FloatRect(        0,        0, 4, 4 ), /* Top left */
			FloatRect( corOff.x,        0, 4, 4 ), /* Top right */
			FloatRect(        0, corOff.y, 4, 4 ), /* Bottom left */
			FloatRect( corOff.x, corOff.y, 4, 4 )  /* Bottom right */
		};

		const Vec2i sideLen(rect.w - 4*2, rect.h - 4*2);

		const Sides<FloatRect> sidePos =
		{
		    FloatRect(        0,        4,         4, sideLen.y ), /* Left */
		    FloatRect( corOff.x,        4,         4, sideLen.y ), /* Right */
		    FloatRect(        4,        0, sideLen.x,         4 ), /* Top */
		    FloatRect(        4, corOff.y, sideLen.x,         4 )  /* Bottom */
		};

		const FloatRect bgPos(4, 4, sideLen.x, sideLen.y);

		bool drawSidesLR = rect.h > 8;
		bool drawSidesTB = rect.w > 8;
		bool drawBg = drawSidesLR && drawSidesTB;

		size_t quads = 0;
		quads += 4; /* 4 corners */

		if (drawSidesLR)
			quads += 2;

		if (drawSidesTB)
			quads += 2;

		if (drawBg)
			quads += 1;

		cursorVert.resize(quads);
		Vertex *vert = dataPtr(cursorVert.vertices);
		size_t i = 0;

		i += Quad::setTexPosRect(&vert[i*4], src.corners.tl, cornerPos.tl);
		i += Quad::setTexPosRect(&vert[i*4], src.corners.tr, cornerPos.tr);
		i += Quad::setTexPosRect(&vert[i*4], src.corners.bl, cornerPos.bl);
		i += Quad::setTexPosRect(&vert[i*4], src.corners.br, cornerPos.br);

		if (drawSidesLR)
		{
			i += Quad::setTexPosRect(&vert[i*4], src.border.l, sidePos.l);
			i += Quad::setTexPosRect(&vert[i*4], src.border.r, sidePos.r);
		}

		if (drawSidesTB)
		{
			i += Quad::setTexPosRect(&vert[i*4], src.border.t, sidePos.t);
			i += Quad::setTexPosRect(&vert[i*4], src.border.b, sidePos.b);
		}

		if (drawBg)
			Quad::setTexPosRect(&vert[i*4], src.bg, bgPos);
	}

	void updatePauseQuad()
	{
		if (!pauseVert)
			return;

		/* Set quad */
		Quad::setTexRect(pauseVert, pauseSrc[pauseQuad[pauseQuadIdx]]);

		/* Set opacity */
		Quad::setColor(pauseVert, Vec4(1, 1, 1, pauseAlpha[pauseAlphaIdx] / 255.0f));

		ctrlVertArrayDirty = true;
	}

	void updateCursorAlpha()
	{
		if (cursorVert.count() == 0)
			return;

		Vec4 color(1, 1, 1, cursorAlpha[cursorAlphaIdx] / 255.0f);

		for (size_t i = 0; i < cursorVert.count(); ++i)
			Quad::setColor(&cursorVert.vertices[i*4], color);

		cursorVertArrayDirty = true;
	}

	void stepAnimations()
	{
		if (active)
			if (++cursorAlphaIdx == cursorAlphaN)
				cursorAlphaIdx = 0;

		if (pause)
		{
			if (pauseAlphaIdx < pauseAlphaN-1)
				++pauseAlphaIdx;

			if (++pauseQuadIdx == pauseQuadN)
				pauseQuadIdx = 0;
		}
	}

	void prepare()
	{
		if (base.vertDirty)
		{
			rebuildBaseVert();
			base.vertDirty = false;
			base.texDirty = true;
		}

		if (base.texSizeDirty)
		{
			updateBaseTexSize();
			base.texSizeDirty = false;
			base.texDirty = true;
		}

		if (base.texDirty)
		{
			redrawBaseTex();
			base.texDirty = false;
		}

		if (clipRectDirty)
		{
			updateClipRect();
			clipRectDirty = false;
		}

		if (ctrlVertDirty)
		{
			rebuildCtrlVert();
			updatePauseQuad();
			ctrlVertDirty = false;
		}

		if (ctrlVertArrayDirty)
		{
			ctrlVert.commit();
			ctrlVertArrayDirty = false;
		}

		if (cursorVertDirty)
		{
			rebuildCursorVert();
			updateCursorAlpha();
			cursorVertDirty = false;
		}

		if (cursorVertArrayDirty)
		{
			cursorVert.commit();
			cursorVertArrayDirty = false;
		}
	}

	void draw()
	{
		if (base.tex.tex == TEX::ID(0))
			return;

		bool windowskinValid = !nullOrDisposed(windowskin);
		bool contentsValid = !nullOrDisposed(contents);

		Vec2i trans = geo.pos() + sceneOffset;

		SimpleAlphaShader &shader = shState->shaders().simpleAlpha;
		shader.bind();
		shader.applyViewportProj();

		if (windowskinValid)
		{
			shader.setTranslation(trans);
			shader.setTexSize(Vec2i(base.tex.width, base.tex.height));

			TEX::bind(base.tex.tex);
			base.quad.draw();

			if (openness < 255)
				return;

			windowskin->bindTex(shader);

			TEX::setSmooth(true);
			ctrlVert.draw(0, ctrlQuads);
			TEX::setSmooth(false);
		}

		if (openness < 255)
			return;

		bool drawCursor = cursorVert.count() > 0 && windowskinValid;

		if (drawCursor || contentsValid)
		{
			/* Translate cliprect from local into screen space */
			IntRect clip = clipRect;
			clip.setPos(clip.pos() + trans);

			glState.scissorBox.push();
			glState.scissorTest.pushSet(true);

			if (rgssVer >= 3)
				glState.scissorBox.setIntersect(clip);
			else
				glState.scissorBox.setIntersect(IntRect(trans, geo.size()));

			IntRect pad = padRect;
			pad.setPos(pad.pos() + trans);

			if (drawCursor)
			{
				Vec2i contTrans = pad.pos();
				contTrans.x += cursorRect->x;
				contTrans.y += cursorRect->y;

				if (rgssVer >= 3)
					contTrans -= contentsOff;

				shader.setTranslation(contTrans);

				TEX::setSmooth(true);
				cursorVert.draw();
				TEX::setSmooth(false);
			}

			if (contentsValid)
			{
				if (rgssVer <= 2)
					glState.scissorBox.setIntersect(clip);

				Vec2i contTrans = pad.pos();
				contTrans -= contentsOff;
				shader.setTranslation(contTrans);

				TEX::setSmooth(false); // XXX
				contents->bindTex(shader);
				contentsQuad.draw();
			}

			glState.scissorBox.pop();
			glState.scissorTest.pop();
		}

		TEX::setSmooth(false); // XXX FIND out a way to eliminate
	}
};

WindowVX::WindowVX(Viewport *viewport)
    : ViewportElement(viewport, DEF_Z, DEF_SPRITE_Y)
{
	p = new WindowVXPrivate(0, 0, 0, 0);
	onGeometryChange(scene->getGeometry());
}

WindowVX::WindowVX(int x, int y, int width, int height)
    : ViewportElement(0, DEF_Z, DEF_SPRITE_Y)
{
	p = new WindowVXPrivate(x, y, width, height);
	onGeometryChange(scene->getGeometry());
}

WindowVX::~WindowVX()
{
	dispose();
}

void WindowVX::update()
{
	guardDisposed();

	p->stepAnimations();

	p->updatePauseQuad();
	p->updateCursorAlpha();
}

void WindowVX::move(int x, int y, int width, int height)
{
	guardDisposed();

	p->width = width;
	p->height = height;

	const Vec2i size(std::max(0, width), std::max(0, height));

	if (p->geo.size() != size)
	{
		p->base.vertDirty = true;
		p->base.texSizeDirty = true;
		p->clipRectDirty = true;
		p->ctrlVertDirty = true;
	}

	p->geo = IntRect(Vec2i(x, y), size);
	p->updateBaseQuad();
}

bool WindowVX::isOpen() const
{
	guardDisposed();

	return p->openness == 255;
}

bool WindowVX::isClosed() const
{
	guardDisposed();

	return p->openness == 0;
}

DEF_ATTR_SIMPLE(WindowVX, X,          int,     p->geo.x)
DEF_ATTR_SIMPLE(WindowVX, Y,          int,     p->geo.y)
DEF_ATTR_SIMPLE(WindowVX, CursorRect, Rect&,  *p->cursorRect)
DEF_ATTR_SIMPLE(WindowVX, Tone,       Tone&,  *p->tone)

DEF_ATTR_RD_SIMPLE(WindowVX, Windowskin,      Bitmap*, p->windowskin)
DEF_ATTR_RD_SIMPLE(WindowVX, Contents,        Bitmap*, p->contents)
DEF_ATTR_RD_SIMPLE(WindowVX, Active,          bool,    p->active)
DEF_ATTR_RD_SIMPLE(WindowVX, ArrowsVisible,   bool,    p->arrowsVisible)
DEF_ATTR_RD_SIMPLE(WindowVX, Pause,           bool,    p->pause)
DEF_ATTR_RD_SIMPLE(WindowVX, Width,           int,     p->width)
DEF_ATTR_RD_SIMPLE(WindowVX, Height,          int,     p->height)
DEF_ATTR_RD_SIMPLE(WindowVX, OX,              int,     p->contentsOff.x)
DEF_ATTR_RD_SIMPLE(WindowVX, OY,              int,     p->contentsOff.y)
DEF_ATTR_RD_SIMPLE(WindowVX, Padding,         int,     p->padding)
DEF_ATTR_RD_SIMPLE(WindowVX, PaddingBottom,   int,     p->paddingBottom)
DEF_ATTR_RD_SIMPLE(WindowVX, Opacity,         int,     p->opacity)
DEF_ATTR_RD_SIMPLE(WindowVX, BackOpacity,     int,     p->backOpacity)
DEF_ATTR_RD_SIMPLE(WindowVX, ContentsOpacity, int,     p->contentsOpacity)
DEF_ATTR_RD_SIMPLE(WindowVX, Openness,        int,     p->openness)

void WindowVX::setWindowskin(Bitmap *value)
{
	guardDisposed();

	if (p->windowskin == value)
		return;

	p->windowskin = value;
	p->base.texDirty = true;
}

void WindowVX::setContents(Bitmap *value)
{
	guardDisposed();

	if (p->contents == value)
		return;

	p->contents = value;

	if (nullOrDisposed(value))
		return;

	FloatRect rect = p->contents->rect();
	p->contentsQuad.setTexPosRect(rect, rect);
	p->ctrlVertDirty = true;
}

void WindowVX::setActive(bool value)
{
	guardDisposed();

	if (p->active == value)
		return;

	p->active = value;
	p->cursorAlphaIdx = cursorAlphaResetIdx;
	p->updateCursorAlpha();
}

void WindowVX::setArrowsVisible(bool value)
{
	guardDisposed();

	if (p->arrowsVisible == value)
		return;

	p->arrowsVisible = value;
	p->ctrlVertDirty = true;
}

void WindowVX::setPause(bool value)
{
	guardDisposed();

	if (p->pause == value)
		return;

	p->pause = value;
	p->pauseAlphaIdx = 0;
	p->pauseQuadIdx = 0;
	p->ctrlVertDirty = true;
}

void WindowVX::setWidth(int value)
{
	guardDisposed();

	if (p->width == value)
		return;

	p->width = value;
	p->geo.w = std::max(0, value);
	p->base.vertDirty = true;
	p->base.texSizeDirty = true;
	p->clipRectDirty = true;
	p->ctrlVertDirty = true;
	p->updateBaseQuad();
}

void WindowVX::setHeight(int value)
{
	guardDisposed();

	if (p->height == value)
		return;

	p->height = value;
	p->geo.h = std::max(0, value);
	p->base.vertDirty = true;
	p->base.texSizeDirty = true;
	p->clipRectDirty = true;
	p->ctrlVertDirty = true;
	p->updateBaseQuad();
}

void WindowVX::setOX(int value)
{
	guardDisposed();

	if (p->contentsOff.x == value)
		return;

	p->contentsOff.x = value;
	p->ctrlVertDirty = true;
}

void WindowVX::setOY(int value)
{
	guardDisposed();

	if (p->contentsOff.y == value)
		return;

	p->contentsOff.y = value;
	p->ctrlVertDirty = true;
}

void WindowVX::setPadding(int value)
{
	guardDisposed();

	if (p->padding == value)
		return;

	p->padding = value;
	p->paddingBottom = value;
	p->clipRectDirty = true;
}

void WindowVX::setPaddingBottom(int value)
{
	guardDisposed();

	if (p->paddingBottom == value)
		return;

	p->paddingBottom = value;
	p->clipRectDirty = true;
}

void WindowVX::setOpacity(int value)
{
	guardDisposed();

	if (p->opacity == value)
		return;

	p->opacity = value;
	p->base.quad.setColor(Vec4(1, 1, 1, p->opacity.norm));
}

void WindowVX::setBackOpacity(int value)
{
	guardDisposed();

	if (p->backOpacity == value)
		return;

	p->backOpacity = value;
	p->base.texDirty = true;
}

void WindowVX::setContentsOpacity(int value)
{
	guardDisposed();

	if (p->contentsOpacity == value)
		return;

	p->contentsOpacity = value;
	p->contentsQuad.setColor(Vec4(1, 1, 1, p->contentsOpacity.norm));
}

void WindowVX::setOpenness(int value)
{
	guardDisposed();

	if (p->openness == value)
		return;

	p->openness = value;
	p->updateBaseQuad();
}

void WindowVX::initDynAttribs()
{
	p->cursorRect = new Rect;
	p->refreshCursorRectCon();

	if (rgssVer >= 3)
	{
		p->tone = new Tone;
		p->refreshToneCon();
	}
}

void WindowVX::draw()
{
	p->draw();
}

void WindowVX::onGeometryChange(const Scene::Geometry &geo)
{
	p->sceneOffset = geo.offset();
}

void WindowVX::releaseResources()
{
	unlink();

	delete p;
}
