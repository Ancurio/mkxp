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
#include "tilemap-common.h"

#include <vector>
#include <sigc++/connection.h>

/* Flash tiles pulsing opacity */
static const uint8_t flashAlpha[] =
{
	/* Fade in */
	0x78, 0x78, 0x78, 0x78, 0x96, 0x96, 0x96, 0x96,
	0xB4, 0xB4, 0xB4, 0xB4, 0xD2, 0xD2, 0xD2, 0xD2,
	/* Fade out */
	0xF0, 0xF0, 0xF0, 0xF0, 0xD2, 0xD2, 0xD2, 0xD2,
	0xB4, 0xB4, 0xB4, 0xB4, 0x96, 0x96, 0x96, 0x96
};

static elementsN(flashAlpha);

struct TilemapVXPrivate : public ViewportElement, TileAtlasVX::Reader
{
	Bitmap *bitmaps[BM_COUNT];

	Table *mapData;
	Table *flags;
	Vec2i origin;

	/* Subregion of the map that is drawn to screen (map viewport) */
	IntRect mapViewp;
	/* Position on screen the map subregion is drawn at */
	Vec2i dispPos;
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
	Vec2 aniOffset;

	FlashMap flashMap;
	uint8_t flashAlphaIdx;

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
			p->drawFlashLayer();
		}

		ABOUT_TO_ACCESS_NOOP
	};

	AboveLayer above;

	TilemapVXPrivate(Viewport *viewport)
	    : ViewportElement(viewport),
	      mapData(0),
	      flags(0),
	      allocQuads(0),
	      groundQuads(0),
	      aboveQuads(0),
	      frameIdx(0),
	      flashAlphaIdx(0),
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

		onGeometryChange(scene->getGeometry());

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

	void invalidateBuffers()
	{
		buffersDirty = true;
	}

	void rebuildAtlas()
	{
		TileAtlasVX::build(atlas, bitmaps);
	}

	void updateMapViewport()
	{
		/* Note: We include one extra row at the top above
		 * the normal map viewport to ensure the legs of table
		 * tiles off screen are properly drawn */

		IntRect newMvp;

		const Vec2i combOrigin = origin + sceneGeo.orig;
		const Vec2i geoSize = sceneGeo.rect.size();

		newMvp.setPos(getTilePos(combOrigin) - Vec2i(0, 1));

		/* Ensure that the size is big enough to cover the whole viewport,
		 * and add one tile row/column as a buffer for scrolling */
		newMvp.setSize((geoSize / 32) + !!(geoSize % 32) + Vec2i(1, 2));

		if (newMvp != mapViewp)
		{
			mapViewp = newMvp;
			flashMap.setViewport(newMvp);
			buffersDirty = true;
		}

		dispPos = sceneGeo.rect.pos() - wrap(combOrigin, 32) - Vec2i(0, 32);
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
		                       mapViewp.x, mapViewp.y, mapViewp.w, mapViewp.h);

		groundQuads = groundVert.size() / 4;
		aboveQuads = aboveVert.size() / 4;
		size_t totalQuads = groundQuads + aboveQuads;

		VBO::bind(vbo);

		if (totalQuads > allocQuads)
		{
			VBO::allocEmpty(quadBytes(totalQuads), GL_DYNAMIC_DRAW);
			allocQuads = totalQuads;
		}

		VBO::uploadSubData(0, quadBytes(groundQuads), dataPtr(groundVert));
		VBO::uploadSubData(quadBytes(groundQuads), quadBytes(aboveQuads), dataPtr(aboveVert));

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

		flashMap.prepare();
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
		drawGround();
		drawFlashLayer();
	}

	void drawGround()
	{
		if (groundQuads == 0)
			return;

		ShaderBase *shader;

		if (!nullOrDisposed(bitmaps[BM_A1]))
		{
			/* Animated tileset */
			TilemapVXShader &tmShader = shState->shaders().tilemapVX;
			tmShader.bind();
			tmShader.setAniOffset(aniOffset);

			shader = &tmShader;
		}
		else
		{
			/* Static tileset */
			shader = &shState->shaders().simple;
			shader->bind();
		}

		shader->setTexSize(Vec2i(atlas.width, atlas.height));
		shader->applyViewportProj();
		shader->setTranslation(dispPos);

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

	void drawFlashLayer()
	{
		/* Flash tiles are drawn twice at half opacity, once over the
		 * ground layer, and once over the above layer */
		float alpha = (flashAlpha[flashAlphaIdx] / 255.f) / 2;
		flashMap.draw(alpha, dispPos);
	}

	void onGeometryChange(const Scene::Geometry &geo)
	{
		sceneGeo = geo;

		buffersDirty = true;
		mapViewportDirty = true;
	}

	ABOUT_TO_ACCESS_NOOP

	/* TileAtlasVX::Reader */
	void onQuads(const FloatRect *t, const FloatRect *p,
	             size_t n, bool overPlayer)
	{
		SVertex *vert = allocVert(overPlayer ? aboveVert : groundVert, n*4);

		for (size_t i = 0; i < n; ++i)
			Quad::setTexPosRect(&vert[i*4], t[i], p[i]);
	}
};

void TilemapVX::BitmapArray::set(int i, Bitmap *bitmap)
{
	if (!p)
		return;

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
		(sigc::mem_fun(p, &TilemapVXPrivate::invalidateAtlas));
}

Bitmap *TilemapVX::BitmapArray::get(int i) const
{
	if (!p)
		return 0;

	if (i < 0 || i >= BM_COUNT)
		return 0;

	return p->bitmaps[i];
}

TilemapVX::TilemapVX(Viewport *viewport)
{
	p = new TilemapVXPrivate(viewport);
	bmProxy.p = p;
}

TilemapVX::~TilemapVX()
{
	dispose();
}

void TilemapVX::update()
{
	guardDisposed();

	/* Animate tiles */
	if (++p->frameIdx >= 30*3*4)
		p->frameIdx = 0;

	const uint8_t aniIndicesA[3*4] =
		{ 0, 1, 2, 1, 0, 1, 2, 1, 0, 1, 2, 1 };
	const uint8_t aniIndicesC[3*4] =
		{ 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2 };

	uint8_t aniIdxA = aniIndicesA[p->frameIdx / 30];
	uint8_t aniIdxC = aniIndicesC[p->frameIdx / 30];

	p->aniOffset = Vec2(aniIdxA * 2 * 32, aniIdxC * 32);

	/* Animate flash */
	if (++p->flashAlphaIdx >= flashAlphaN)
		p->flashAlphaIdx = 0;
}

TilemapVX::BitmapArray &TilemapVX::getBitmapArray()
{
	guardDisposed();

	return bmProxy;
}

DEF_ATTR_RD_SIMPLE(TilemapVX, MapData, Table*, p->mapData)
DEF_ATTR_RD_SIMPLE(TilemapVX, FlashData, Table*, p->flashMap.getData())
DEF_ATTR_RD_SIMPLE(TilemapVX, Flags, Table*, p->flags)
DEF_ATTR_RD_SIMPLE(TilemapVX, OX, int, p->origin.x)
DEF_ATTR_RD_SIMPLE(TilemapVX, OY, int, p->origin.y)

Viewport *TilemapVX::getViewport() const
{
	guardDisposed();

	return p->getViewport();
}

bool TilemapVX::getVisible() const
{
	guardDisposed();

	return p->getVisible();
}

void TilemapVX::setViewport(Viewport *value)
{
	guardDisposed();

	p->setViewport(value);
	p->above.setViewport(value);
}

void TilemapVX::setMapData(Table *value)
{
	guardDisposed();

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
	guardDisposed();

	p->flashMap.setData(value);
}

void TilemapVX::setFlags(Table *value)
{
	guardDisposed();

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
	guardDisposed();

	p->setVisible(value);
	p->above.setVisible(value);
}

void TilemapVX::setOX(int value)
{
	guardDisposed();

	if (p->origin.x == value)
		return;

	p->origin.x = value;
	p->mapViewportDirty = true;
}

void TilemapVX::setOY(int value)
{
	guardDisposed();

	if (p->origin.y == value)
		return;

	p->origin.y = value;
	p->mapViewportDirty = true;
}

void TilemapVX::releaseResources()
{
	delete p;
	bmProxy.p = 0;
}
