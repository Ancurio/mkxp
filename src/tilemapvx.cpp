/*
** tilemapvx.cpp
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

#include "tilemapvx.h"

#include "tileatlasvx.h"
#include "etc-internal.h"
#include "bitmap.h"
#include "table.h"
#include "viewport.h"
#include "gl-util.h"
#include "sharedstate.h"
#include "glstate.h"
#include "vertex.h"
#include "quad.h"
#include "quadarray.h"
#include "shader.h"

#include <vector>
#include <sigc++/connection.h>
#include <sigc++/bind.h>

/* Map viewport size */
// FIXME: This will be wrong if resolution is changed
static const int viewpW = 18;
static const int viewpH = 14;

/* How many tiles are max visible on screen at once */
static const Vec2i screenTiles(18, 14);

// FIXME: Implement flash

struct TilemapVXPrivate : public ViewportElement, TileAtlasVX::Reader
{
	TilemapVX::BitmapArray bitmapsProxy;
	Bitmap *bitmaps[BM_COUNT];

	Table *mapData;
	Table *flashData;
	Table *flags;
	Vec2i offset;

	Vec2i dispPos;
	/* Map viewport position */
	Vec2i viewpPos;
	Vec2i sceneOffset;
	Scene::Geometry sceneGeo;

	std::vector<SVertex> groundVert;
	std::vector<SVertex> aboveVert;

	TEXFBO atlas;
	VBO::ID vbo;
	GLMeta::VAO vao;

	size_t allocQuads;

	size_t groundQuads;
	size_t aboveQuads;

	uint16_t frameIdx;
	uint8_t aniIdxA;
	uint8_t aniIdxC;
	Vec2 aniOffset;

	bool atlasDirty;
	bool buffersDirty;
	bool mapViewportDirty;

	sigc::connection mapDataCon;
	sigc::connection flagsCon;

	sigc::connection prepareCon;
	sigc::connection bmChangedCons[BM_COUNT];
	sigc::connection bmDisposedCons[BM_COUNT];

	struct AboveLayer : public ViewportElement
	{
		TilemapVXPrivate *p;

		AboveLayer(TilemapVXPrivate *p, Viewport *viewport)
		    : ViewportElement(viewport, 200),
		      p(p)
		{}

		void draw()
		{
			p->drawAbove();
		}
	};

	AboveLayer above;

	TilemapVXPrivate(Viewport *viewport)
	    : ViewportElement(viewport),
	      mapData(0),
	      flashData(0),
	      flags(0),
	      allocQuads(0),
	      groundQuads(0),
	      aboveQuads(0),
	      frameIdx(0),
	      aniIdxA(0),
	      aniIdxC(0),
	      atlasDirty(true),
	      buffersDirty(false),
	      mapViewportDirty(false),
	      above(this, viewport)
	{
		memset(bitmaps, 0, sizeof(bitmaps));

		shState->requestAtlasTex(ATLASVX_W, ATLASVX_H, atlas);

		vbo = VBO::gen();

		GLMeta::vaoFillInVertexData<SVertex>(vao);
		vao.vbo = vbo;
		vao.ibo = shState->globalIBO().ibo;
		GLMeta::vaoInit(vao);

		prepareCon = shState->prepareDraw.connect
			(sigc::mem_fun(this, &TilemapVXPrivate::prepare));
	}

	virtual ~TilemapVXPrivate()
	{
		GLMeta::vaoFini(vao);
		VBO::del(vbo);

		shState->releaseAtlasTex(atlas);

		prepareCon.disconnect();

		mapDataCon.disconnect();
		flagsCon.disconnect();

		for (size_t i = 0; i < BM_COUNT; ++i)
		{
			bmChangedCons[i].disconnect();
			bmDisposedCons[i].disconnect();
		}
	}

	void invalidateAtlas()
	{
		atlasDirty = true;
	}

	void onBitmapDisposed(int i)
	{
		bitmaps[i] = 0;
		bmChangedCons[i].disconnect();
		bmDisposedCons[i].disconnect();

		atlasDirty = true;
	}

	void invalidateBuffers()
	{
		buffersDirty = true;
	}

	void rebuildAtlas()
	{
		TileAtlasVX::build(atlas, bitmaps);
	}

	void updatePosition()
	{
		dispPos.x = -(offset.x - viewpPos.x * 32) + sceneOffset.x;
		dispPos.y = -(offset.y - viewpPos.y * 32) + sceneOffset.y;
	}

	void updateMapViewport()
	{
		int tileOX, tileOY;

		Vec2i offs(offset.x-sceneOffset.x, offset.y-sceneOffset.y);

		if (offs.x >= 0)
			tileOX = offs.x / 32;
		else
			tileOX = -(-(offs.x-31) / 32);

		if (offs.y >= 0)
			tileOY = offs.y / 32;
		else
			tileOY = -(-(offs.y-31) / 32);

		bool dirty = false;

		if (tileOX < viewpPos.x || tileOX + screenTiles.x > viewpPos.x + viewpW)
		{
			viewpPos.x = tileOX;
			dirty = true;
		}

		if (tileOY < viewpPos.y || tileOY + screenTiles.y > viewpPos.y + viewpH)
		{
			viewpPos.y = tileOY;
			dirty = true;
		}

		if (dirty)
		{
			buffersDirty = true;
		}

		updatePosition();
	}

	static size_t quadBytes(size_t quads)
	{
		return quads * 4 * sizeof(SVertex);
	}

	void rebuildBuffers()
	{
		if (!mapData)
			return;

		groundVert.clear();
		aboveVert.clear();

		TileAtlasVX::readTiles(*this, *mapData, flags,
		                       viewpPos.x, viewpPos.y, viewpW, viewpH);

		groundQuads = groundVert.size() / 4;
		aboveQuads = aboveVert.size() / 4;
		size_t totalQuads = groundQuads + aboveQuads;

		VBO::bind(vbo);

		if (totalQuads > allocQuads)
		{
			VBO::allocEmpty(quadBytes(totalQuads), GL_DYNAMIC_DRAW);
			allocQuads = totalQuads;
		}

		VBO::uploadSubData(0, quadBytes(groundQuads), &groundVert[0]);
		VBO::uploadSubData(quadBytes(groundQuads), quadBytes(aboveQuads), &aboveVert[0]);

		VBO::unbind();

		shState->ensureQuadIBO(totalQuads);
	}

	void prepare()
	{
		if (!mapData)
			return;

		if (atlasDirty)
		{
			rebuildAtlas();
			atlasDirty = false;
		}

		if (mapViewportDirty)
		{
			updateMapViewport();
			mapViewportDirty = false;
		}

		if (buffersDirty)
		{
			rebuildBuffers();
			buffersDirty = false;
		}
	}

	SVertex *allocVert(std::vector<SVertex> &vec, size_t count)
	{
		size_t size = vec.size();
		vec.resize(size + count);

		return &vec[size];
	}

	/* SceneElement */
	void draw()
	{
		TilemapVXShader &shader = shState->shaders().tilemapVX;
		shader.bind();
		shader.setTexSize(Vec2i(atlas.width, atlas.height));
		shader.applyViewportProj();
		shader.setTranslation(dispPos);
		shader.setAniOffset(aniOffset);

		TEX::bind(atlas.tex);
		GLMeta::vaoBind(vao);

		gl.DrawElements(GL_TRIANGLES, groundQuads*6, _GL_INDEX_TYPE, 0);

		GLMeta::vaoUnbind(vao);
	}

	void drawAbove()
	{
		if (aboveQuads == 0)
			return;

		SimpleShader &shader = shState->shaders().simple;
		shader.bind();
		shader.setTexSize(Vec2i(atlas.width, atlas.height));
		shader.applyViewportProj();
		shader.setTranslation(dispPos);

		TEX::bind(atlas.tex);
		GLMeta::vaoBind(vao);

		gl.DrawElements(GL_TRIANGLES, aboveQuads*6, _GL_INDEX_TYPE,
		                (GLvoid*) (groundQuads*6*sizeof(index_t)));

		GLMeta::vaoUnbind(vao);
	}

	void onGeometryChange(const Scene::Geometry &geo)
	{
		sceneOffset.x = geo.rect.x - geo.xOrigin;
		sceneOffset.y = geo.rect.y - geo.yOrigin;
		sceneGeo = geo;

		mapViewportDirty = true;
	}

	/* TileAtlasVX::Reader */
	void onQuads1(const FloatRect &t1, const FloatRect &p1,
	              bool overPlayer)
	{
		SVertex *vert;

		if (overPlayer)
			vert = allocVert(aboveVert, 4);
		else
			vert = allocVert(groundVert, 4);

		Quad::setTexPosRect(vert, t1, p1);
	}

	void onQuads2(const FloatRect t[2], const FloatRect p[2])
	{
		SVertex *vert = allocVert(groundVert, 8);

		Quad::setTexPosRect(&vert[0], t[0], p[0]);
		Quad::setTexPosRect(&vert[4], t[1], p[1]);
	}

	void onQuads4(const FloatRect t[4], const FloatRect p[4])
	{
		SVertex *vert = allocVert(groundVert, 16);

		Quad::setTexPosRect(&vert[ 0], t[0], p[0]);
		Quad::setTexPosRect(&vert[ 4], t[1], p[1]);
		Quad::setTexPosRect(&vert[ 8], t[2], p[2]);
		Quad::setTexPosRect(&vert[12], t[3], p[3]);
	}
};

void TilemapVX::BitmapArray::set(int i, Bitmap *bitmap)
{
	if (i < 0 || i >= BM_COUNT)
		return;

	if (p->bitmaps[i] == bitmap)
		return;

	p->bitmaps[i] = bitmap;
	p->atlasDirty = true;

	p->bmChangedCons[i].disconnect();
	p->bmChangedCons[i] = bitmap->modified.connect
		(sigc::mem_fun(p, &TilemapVXPrivate::invalidateAtlas));

	p->bmDisposedCons[i].disconnect();
	p->bmDisposedCons[i] = bitmap->wasDisposed.connect
		(sigc::bind(sigc::mem_fun(p, &TilemapVXPrivate::onBitmapDisposed), i));
}

Bitmap *TilemapVX::BitmapArray::get(int i) const
{
	if (i < 0 || i >= BM_COUNT)
		return 0;

	return p->bitmaps[i];
}

TilemapVX::TilemapVX(Viewport *viewport)
{
	p = new TilemapVXPrivate(viewport);
	p->bitmapsProxy.p = p;
}

TilemapVX::~TilemapVX()
{
	delete p;
}

void TilemapVX::update()
{
	if (++p->frameIdx >= 30*3*4)
		p->frameIdx = 0;

	const uint8_t aniIndicesA[3*4] =
		{ 0, 1, 2, 1, 0, 1, 2, 1, 0, 1, 2, 1 };
	const uint8_t aniIndicesC[3*4] =
		{ 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2 };

	p->aniIdxA = aniIndicesA[p->frameIdx / 30];
	p->aniIdxC = aniIndicesC[p->frameIdx / 30];

	p->aniOffset = Vec2(p->aniIdxA * 2 * 32, p->aniIdxC * 32);
}

TilemapVX::BitmapArray &TilemapVX::getBitmapArray() const
{
	return p->bitmapsProxy;
}

DEF_ATTR_RD_SIMPLE(TilemapVX, MapData, Table*, p->mapData)
DEF_ATTR_RD_SIMPLE(TilemapVX, FlashData, Table*, p->flashData)
DEF_ATTR_RD_SIMPLE(TilemapVX, Flags, Table*, p->flags)
DEF_ATTR_RD_SIMPLE(TilemapVX, OX, int, p->offset.x)
DEF_ATTR_RD_SIMPLE(TilemapVX, OY, int, p->offset.y)

Viewport *TilemapVX::getViewport() const
{
	return p->getViewport();
}

bool TilemapVX::getVisible() const
{
	return p->getVisible();
}

void TilemapVX::setViewport(Viewport *value)
{
	p->setViewport(value);
	p->above.setViewport(value);
}

void TilemapVX::setMapData(Table *value)
{
	if (p->mapData == value)
		return;

	p->mapData = value;
	p->buffersDirty = true;

	p->mapDataCon.disconnect();
	p->mapDataCon = value->modified.connect
		(sigc::mem_fun(p, &TilemapVXPrivate::invalidateBuffers));
}

void TilemapVX::setFlashData(Table *value)
{
	if (p->flashData == value)
		return;

	p->flashData = value;
}

void TilemapVX::setFlags(Table *value)
{
	if (p->flags == value)
		return;

	p->flags = value;
	p->buffersDirty = true;

	p->flagsCon.disconnect();
	p->flagsCon = value->modified.connect
		(sigc::mem_fun(p, &TilemapVXPrivate::invalidateBuffers));
}

void TilemapVX::setVisible(bool value)
{
	p->setVisible(value);
	p->above.setVisible(value);
}

void TilemapVX::setOX(int value)
{
	if (p->offset.x == value)
		return;

	p->offset.x = value;
	p->mapViewportDirty = true;
}

void TilemapVX::setOY(int value)
{
	if (p->offset.y == value)
		return;

	p->offset.y = value;
	p->mapViewportDirty = true;
}
