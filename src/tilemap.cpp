/*
** tilemap.cpp
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

#include "tilemap.h"

#include "viewport.h"
#include "bitmap.h"
#include "table.h"

#include "sharedstate.h"
#include "glstate.h"
#include "gl-util.h"
#include "etc-internal.h"
#include "quadarray.h"
#include "texpool.h"
#include "quad.h"
#include "tileatlas.h"

#include <sigc++/connection.h>

#include <string.h>
#include <stdint.h>
#include <algorithm>
#include <vector>

#include <SDL_surface.h>

extern const StaticRect autotileRects[];

typedef std::vector<SVertex> SVVector;

static const int tilesetW  = 8 * 32;
static const int autotileW = 3 * 32;
static const int autotileH = 4 * 32;

static const int autotileCount = 7;

static const int atAreaW = autotileW * 4;
static const int atAreaH = autotileH * autotileCount;

static const int tsLaneW = tilesetW / 2;

/* Map viewport size */
static const int viewpW = 21;
static const int viewpH = 16;

static const size_t scanrowsMax = viewpH + 5;

/* Vocabulary:
 *
 * Atlas: A texture containing both the tileset and all
 *   autotile images. This is so the entire tilemap can
 *   be drawn from one texture (for performance reasons).
 *   This means that we have to watch the 'modified' signals
 *   of all Bitmaps that make up the atlas, and update it
 *   as required during runtime.
 *   The atlas is tightly packed, with the autotiles located
 *   in the top left corener and the tileset image filing the
 *   remaining open space (below the autotiles as well as
 *   besides it). The tileset is vertically cut in half, where
 *   the first half fills available texture space, and then the
 *   other half (as if the right half was cut and pasted below
 *   the left half before fitting it all into the atlas).
 *   Internally these halves are called "tileset lanes".
 *   There is a 32 pixel wide empty buffer below the autotile
 *   area so the vertex shader can safely differentiate between
 *   autotile and tileset vertices (relevant for autotile animation).
 *
 *                  Tile atlas
 *   *-----------------------*--------------*
 *   |     |     |     |     |       ¦       |
 *   | AT1 | AT1 | AT1 | AT1 |       ¦       |
 *   | FR0 | FR1 | FR2 | FR3 |   |   ¦   |   |
 *   |-----|-----|-----|-----|   v   ¦   v   |
 *   |     |     |     |     |       ¦       |
 *   | AT1 |     |     |     |       ¦       |
 *   |     |     |     |     |       ¦       |
 *   |-----|-----|-----|-----|       ¦       |
 *   |[...]|     |     |     |       ¦       |
 *   |-----|-----|-----|-----|       ¦       |
 *   |     |     |     |     |   |   ¦   |   |
 *   | AT7 |     |     |     |   v   ¦   v   |
 *   |     |     |     |     |       ¦       |
 *   |-----|-----|-----|-----|       ¦       |
 *   |      Empty space      |       |       |
 *   |-----------------------|       |       |
 *   |       ¦       ¦       ¦       ¦       |
 *   | Tile- ¦   |   ¦   |   ¦       ¦       |
 *   |  set  ¦   v   ¦   v   ¦       ¦       |
 *   |       ¦       ¦       ¦   |   ¦   |   |
 *   |   |   ¦       ¦       ¦   v   ¦   v   |
 *   |   v   ¦   |   ¦   |   ¦       ¦       |
 *   |       ¦   v   ¦   v   ¦       ¦       |
 *   |       ¦       ¦       ¦       ¦       |
 *   *---------------------------------------*
 *
 *   When allocating the atlas size, we first expand vertically
 *   until all the space immediately below the autotile area
 *   is used up, and then, when the max texture size
 *   is reached, horizontally.
 *
 *   To animate the autotiles, we catch any autotile vertices in
 *   the tilemap shader based on their texcoord, and offset them
 *   horizontally by (animation index) * (autotile frame width = 96).
 *
 * Elements:
 *   Even though the Tilemap carries similarities with other
 *   SceneElements, it is not one itself but composed of multiple
 *   such elements (GroundLayer and ScanRows).
 *
 * GroundLayer:
 *   Every tile with priority=0 is drawn at z=0, so we
 *   collect all such tiles in one big quad array and
 *   draw them at once.
 *
 * ScanRow:
 *   Each tile in row n with priority=m is drawn at the same
 *   z as every tile in row n-1 with priority=m-1. This means
 *   we can collect all tiles sharing the same z in one quad
 *   array and draw them at once. I call these collections
 *   'scanrows', as they're drawn from the top part of the map
 *   (lowest z) to the bottom part (highest z).
 *   Objects that would end up on the same scanrow are eg. trees.
 *
 * Map viewport:
 *   This rectangle describes the subregion of the map that is
 *   actually translated to vertices and stored on the GPU ready
 *   for rendering. Whenever, ox/oy are modified, its position is
 *   adjusted if necessary and the data is regenerated. Its size
 *   is fixed. This is NOT related to the RGSS Viewport class!
 *
 */

static int wrap(size_t value, size_t range)
{
	int res = value % range;
	return res < 0 ? res + range : res;
}

static int16_t tableGetWrapped(const Table *t, int x, int y, int z = 0)
{
	return t->get(wrap(x, t->xSize()),
	              wrap(y, t->ySize()),
	              z);
}

/* Autotile animation */
static const uint8_t atAnimation[16*4] =
{
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3
};

static elementsN(atAnimation);

/* Flash tiles pulsing opacity */
static const uint8_t flashAlpha[] =
{
	/* Fade in */
	0x3C, 0x3C, 0x3C, 0x3C, 0x4B, 0x4B, 0x4B, 0x4B,
	0x5A, 0x5A, 0x5A, 0x5A, 0x69, 0x69, 0x69, 0x69,
	/* Fade out */
	0x78, 0x78, 0x78, 0x78, 0x69, 0x69, 0x69, 0x69,
	0x5A, 0x5A, 0x5A, 0x5A, 0x4B, 0x4B, 0x4B, 0x4B
};

static elementsN(flashAlpha);

struct GroundLayer : public ViewportElement
{
	GLsizei vboCount;
	TilemapPrivate *p;

	GroundLayer(TilemapPrivate *p, Viewport *viewport);

	void updateVboCount();

	void draw();
	void drawInt();
	void drawFlashInt();

	void onGeometryChange(const Scene::Geometry &geo);
};

struct ScanRow : public ViewportElement
{
	size_t index;
	GLintptr vboOffset;
	GLsizei vboCount;
	TilemapPrivate *p;

	/* If this row is part of a batch and not
	 * the head, it is 'muted' via this flag */
	bool batchedFlag;

	/* If this row is a batch head, this variable
	 * holds the element count of the entire batch */
	GLsizei vboBatchCount;

	ScanRow(TilemapPrivate *p, Viewport *viewport);

	void setIndex(int value);

	void draw();
	void drawInt();

	static int calculateZ(TilemapPrivate *p, int index);

	void initUpdateZ();
	void finiUpdateZ(ScanRow *prev);
};

struct TilemapPrivate
{
	Viewport *viewport;

	Tilemap::Autotiles autotilesProxy;
	Bitmap *autotiles[autotileCount];

	Bitmap *tileset;
	Table *mapData;
	Table *flashData;
	Table *priorities;
	bool visible;
	Vec2i offset;

	Vec2i dispPos;

	/* Tile atlas */
	struct {
		TEXFBO gl;

		Vec2i size;

		/* Effective tileset height,
		 * clamped to a multiple of 32 */
		int efTilesetH;

		/* Indices of usable
		 * (not null, not disposed) autotiles */
		std::vector<uint8_t> usableATs;

		/* Indices of animated autotiles */
		std::vector<uint8_t> animatedATs;
	} atlas;

	/* Map viewport position */
	Vec2i viewpPos;

	/* Ground layer vertices */
	SVVector groundVert;

	/* Scanrow vertices */
	SVVector scanrowVert[scanrowsMax];

	/* Base quad indices of each scanrow
	 * in the shared buffer */
	size_t scanrowBases[scanrowsMax+1];

	/* Shared buffers for all tiles */
	struct
	{
		VAO::ID vao;
		VBO::ID vbo;
		bool animated;

		/* Animation state */
		uint8_t frameIdx;
		uint8_t aniIdx;
	} tiles;

	/* Flash buffers */
	struct
	{
		VAO::ID vao;
		VBO::ID vbo;
		size_t quadCount;
		uint8_t alphaIdx;
	} flash;

	/* Scene elements */
	struct
	{
		GroundLayer *ground;
		ScanRow* scanrows[scanrowsMax];
		/* Used rows out of 'scanrows' (rest is hidden) */
		size_t activeRows;
		Scene::Geometry sceneGeo;
		Vec2i sceneOffset;

		/* The ground and scanrow elements' creationStamp
		 * should be aquired once (at Tilemap construction)
		 * instead of regenerated everytime the elements are
		 * (re)created. Scanrows can share one stamp because
		 * their z always differs anway */
		unsigned int groundStamp;
		unsigned int scanrowStamp;
	} elem;

	/* Affected by: autotiles, tileset */
	bool atlasSizeDirty;
	/* Affected by: autotiles(.changed), tileset(.changed), allocateAtlas */
	bool atlasDirty;
	/* Affected by: mapData(.changed), priorities(.changed) */
	bool buffersDirty;
	/* Affected by: ox, oy */
	bool mapViewportDirty;
	/* Affected by: oy */
	bool zOrderDirty;
	/* Affected by: flashData, buffersDirty */
	bool flashDirty;

	/* Resources are sufficient and tilemap is ready to be drawn */
	bool tilemapReady;

	/* Change watches */
	sigc::connection tilesetCon;
	sigc::connection autotilesCon[autotileCount];
	sigc::connection mapDataCon;
	sigc::connection prioritiesCon;
	sigc::connection flashDataCon;

	/* Dispose watches */
	sigc::connection autotilesDispCon[autotileCount];

	/* Draw prepare call */
	sigc::connection prepareCon;

	TilemapPrivate(Viewport *viewport)
	    : viewport(viewport),
	      tileset(0),
	      mapData(0),
	      flashData(0),
	      priorities(0),
	      visible(true),
	      atlasSizeDirty(false),
	      atlasDirty(false),
	      buffersDirty(false),
	      mapViewportDirty(false),
	      zOrderDirty(false),
	      flashDirty(false),
	      tilemapReady(false)
	{
		memset(autotiles, 0, sizeof(autotiles));

		atlas.animatedATs.reserve(autotileCount);
		atlas.efTilesetH = 0;

		tiles.animated = false;
		tiles.frameIdx = 0;
		tiles.aniIdx = 0;

		/* Init tile buffers */
		tiles.vbo = VBO::gen();

		tiles.vao = VAO::gen();
		VAO::bind(tiles.vao);

		gl.EnableVertexAttribArray(Shader::Position);
		gl.EnableVertexAttribArray(Shader::TexCoord);

		VBO::bind(tiles.vbo);
		shState->bindQuadIBO();

		gl.VertexAttribPointer(Shader::Position, 2, GL_FLOAT, GL_FALSE, sizeof(SVertex), SVertex::posOffset());
		gl.VertexAttribPointer(Shader::TexCoord, 2, GL_FLOAT, GL_FALSE, sizeof(SVertex), SVertex::texPosOffset());

		VAO::unbind();
		VBO::unbind();
		IBO::unbind();

		/* Init flash buffers */
		flash.vbo = VBO::gen();
		flash.vao = VAO::gen();
		flash.quadCount = 0;
		flash.alphaIdx = 0;

		VAO::bind(flash.vao);

		gl.EnableVertexAttribArray(Shader::Color);
		gl.EnableVertexAttribArray(Shader::Position);

		VBO::bind(flash.vbo);
		shState->bindQuadIBO();

		gl.VertexAttribPointer(Shader::Color,    4, GL_FLOAT, GL_FALSE, sizeof(CVertex), CVertex::colorOffset());
		gl.VertexAttribPointer(Shader::Position, 2, GL_FLOAT, GL_FALSE, sizeof(CVertex), CVertex::posOffset());

		VAO::unbind();
		VBO::unbind();
		IBO::unbind();

		elem.ground = 0;

		elem.groundStamp = shState->genTimeStamp();
		elem.scanrowStamp = shState->genTimeStamp();

		elem.ground = new GroundLayer(this, viewport);

		for (size_t i = 0; i < scanrowsMax; ++i)
			elem.scanrows[i] = new ScanRow(this, viewport);

		prepareCon = shState->prepareDraw.connect
		        (sigc::mem_fun(this, &TilemapPrivate::prepare));
	}

	~TilemapPrivate()
	{
		/* Destroy elements */
		delete elem.ground;
		for (size_t i = 0; i < scanrowsMax; ++i)
			delete elem.scanrows[i];

		shState->releaseAtlasTex(atlas.gl);

		/* Destroy tile buffers */
		VAO::del(tiles.vao);
		VBO::del(tiles.vbo);

		/* Destroy flash buffers */
		VAO::del(flash.vao);
		VBO::del(flash.vbo);

		/* Disconnect signal handlers */
		tilesetCon.disconnect();
		for (size_t i = 0; i < autotileCount; ++i)
		{
			autotilesCon[i].disconnect();
			autotilesDispCon[i].disconnect();
		}
		mapDataCon.disconnect();
		prioritiesCon.disconnect();
		flashDataCon.disconnect();

		prepareCon.disconnect();
	}

	void updateAtlasInfo()
	{
		if (!tileset || tileset->isDisposed())
		{
			atlas.size = Vec2i();
			return;
		}

		int tsH = tileset->height();
		atlas.efTilesetH = tsH - (tsH % 32);

		atlas.size = TileAtlas::minSize(atlas.efTilesetH, glState.caps.maxTexSize);

		if (atlas.size.x < 0)
			throw Exception(Exception::MKXPError,
		                    "Cannot allocate big enough texture for tileset atlas");
	}

	void updateAutotileInfo()
	{
		/* Check if and which autotiles are animated */
		std::vector<uint8_t> &usableATs = atlas.usableATs;
		std::vector<uint8_t> &animatedATs = atlas.animatedATs;

		usableATs.clear();

		for (size_t i = 0; i < autotileCount; ++i)
		{
			if (!autotiles[i])
				continue;

			if (autotiles[i]->isDisposed())
				continue;

			if (autotiles[i]->megaSurface())
				continue;

			usableATs.push_back(i);

			if (autotiles[i]->width() > autotileW)
				animatedATs.push_back(i);
		}

		tiles.animated = !animatedATs.empty();
	}

	void updateSceneGeometry(const Scene::Geometry &geo)
	{
		elem.sceneOffset.x = geo.rect.x - geo.xOrigin;
		elem.sceneOffset.y = geo.rect.y - geo.yOrigin;
		elem.sceneGeo = geo;
	}

	void updatePosition()
	{
		dispPos.x = -(offset.x - viewpPos.x * 32) + elem.sceneOffset.x;
		dispPos.y = -(offset.y - viewpPos.y * 32) + elem.sceneOffset.x;
	}

	void invalidateAtlasSize()
	{
		atlasSizeDirty = true;
	}

	void invalidateAtlasContents()
	{
		atlasDirty = true;
	}

	void invalidateBuffers()
	{
		buffersDirty = true;
	}

	void invalidateFlash()
	{
		flashDirty = true;
	}

	/* Checks for the minimum amount of data needed to display */
	bool verifyResources()
	{
		if (!tileset)
			return false;

		if (tileset->isDisposed())
			return false;

		if (!mapData)
			return false;

		return true;
	}

	/* Allocates correctly sized TexFBO for atlas */
	void allocateAtlas()
	{
		updateAtlasInfo();

		/* Aquire atlas tex */
		shState->releaseAtlasTex(atlas.gl);
		shState->requestAtlasTex(atlas.size.x, atlas.size.y, atlas.gl);

		atlasDirty = true;
	}

	/* Assembles atlas from tileset and autotile bitmaps */
	void buildAtlas()
	{
		updateAutotileInfo();

		TileAtlas::BlitVec blits = TileAtlas::calcBlits(atlas.efTilesetH, atlas.size);

		/* Clear atlas */
		FBO::bind(atlas.gl.fbo, FBO::Draw);
		glState.clearColor.pushSet(Vec4());
		glState.scissorTest.pushSet(false);

		FBO::clear();

		glState.scissorTest.pop();
		glState.clearColor.pop();

		/* Blit autotiles */
		for (size_t i = 0; i < atlas.usableATs.size(); ++i)
		{
			const uint8_t atInd = atlas.usableATs[i];
			Bitmap *autotile = autotiles[atInd];

			int blitW = std::min(autotile->width(), atAreaW);
			int blitH = std::min(autotile->height(), atAreaH);

			FBO::bind(autotile->getGLTypes().fbo, FBO::Read);

			if (blitW <= autotileW)
			{
				/* Static autotile */
				for (int j = 0; j < 4; ++j)
					FBO::blit(0, 0, autotileW*j, atInd*autotileH, blitW, blitH);
			}
			else
			{
				/* Animated autotile */
				FBO::blit(0, 0, 0, atInd*autotileH, blitW, blitH);
			}
		}

		/* Blit tileset */
		if (tileset->megaSurface())
		{
			/* Mega surface tileset */
			FBO::unbind(FBO::Draw);
			TEX::bind(atlas.gl.tex);

			SDL_Surface *tsSurf = tileset->megaSurface();

			for (size_t i = 0; i < blits.size(); ++i)
			{
				const TileAtlas::Blit &blitOp = blits[i];

				PixelStore::setupSubImage(tsSurf->w, blitOp.src.x, blitOp.src.y);

				TEX::uploadSubImage(blitOp.dst.x, blitOp.dst.y, tsLaneW, blitOp.h, tsSurf->pixels, GL_RGBA);
			}

			PixelStore::reset();
		}
		else
		{
			/* Regular tileset */
			FBO::bind(tileset->getGLTypes().fbo, FBO::Read);

			for (size_t i = 0; i < blits.size(); ++i)
			{
				const TileAtlas::Blit &blitOp = blits[i];

				FBO::blit(blitOp.src.x, blitOp.src.y, blitOp.dst.x, blitOp.dst.y, tsLaneW, blitOp.h);
			}
		}
	}

	int samplePriority(int tileInd)
	{
		if (!priorities)
			return 0;

		if (tileInd > priorities->xSize()-1)
			return 0;

		int value = priorities->at(tileInd);

		if (value > 5)
			return -1;

		return value;
	}

	FloatRect getAutotilePieceRect(int x, int y, /* in pixel coords */
	                               int corner)
	{
		switch (corner)
		{
		case 0 : break;
		case 1 : x += 16; break;
		case 2 : x += 16; y += 16; break;
		case 3 : y += 16; break;
		default: abort();
		}

		return FloatRect(x, y, 16, 16);
	}

	void handleAutotile(int x, int y, int tileInd, SVVector *array)
	{
		/* Which autotile [0-7] */
		int atInd = tileInd / 48 - 1;
		/* Which tile pattern of the autotile [0-47] */
		int subInd = tileInd % 48;

		const StaticRect *pieceRect = &autotileRects[subInd*4];

		/* Iterate over the 4 tile pieces */
		for (int i = 0; i < 4; ++i)
		{
			FloatRect posRect = getAutotilePieceRect(x*32, y*32, i);
			FloatRect texRect = pieceRect[i];

			/* Adjust to atlas coordinates */
			texRect.y += atInd * autotileH;

			SVertex v[4];
			Quad::setTexPosRect(v, texRect, posRect);

			/* Iterate over 4 vertices */
			for (size_t i = 0; i < 4; ++i)
				array->push_back(v[i]);
		}
	}

	void handleTile(int x, int y, int z)
	{
		int tileInd =
			tableGetWrapped(mapData, x + viewpPos.x, y + viewpPos.y, z);

		/* Check for empty space */
		if (tileInd < 48)
			return;

		int prio = samplePriority(tileInd);

		/* Check for faulty data */
		if (prio == -1)
			return;

		SVVector *targetArray;

		/* Prio 0 tiles are all part of the same ground layer */
		if (prio == 0)
		{
			targetArray = &groundVert;
		}
		else
		{
			int scanInd = y + prio;
			targetArray = &scanrowVert[scanInd];
		}

		/* Check for autotile */
		if (tileInd < 48*8)
		{
			handleAutotile(x, y, tileInd, targetArray);
			return;
		}

		int tsInd = tileInd - 48*8;
		int tileX = tsInd % 8;
		int tileY = tsInd / 8;

		Vec2i texPos = TileAtlas::tileToAtlasCoor(tileX, tileY, atlas.efTilesetH, atlas.size.y);
		FloatRect texRect((float) texPos.x+.5, (float) texPos.y+.5, 31, 31);
		FloatRect posRect(x*32, y*32, 32, 32);

		SVertex v[4];
		Quad::setTexPosRect(v, texRect, posRect);

		for (size_t i = 0; i < 4; ++i)
			targetArray->push_back(v[i]);
	}

	void clearQuadArrays()
	{
		groundVert.clear();

		for (size_t i = 0; i < scanrowsMax; ++i)
			scanrowVert[i].clear();
	}

	void buildQuadArray()
	{
		clearQuadArrays();

		for (int x = 0; x < viewpW; ++x)
			for (int y = 0; y < viewpH; ++y)
				for (int z = 0; z < mapData->zSize(); ++z)
					handleTile(x, y, z);
	}

	static size_t quadDataSize(size_t quadCount)
	{
		return quadCount * sizeof(SVertex) * 4;
	}

	size_t scanrowSize(size_t index)
	{
		return scanrowBases[index+1] - scanrowBases[index];
	}

	void uploadBuffers()
	{
		/* Calculate total quad count */
		size_t groundQuadCount = groundVert.size() / 4;
		size_t quadCount = groundQuadCount;

		for (size_t i = 0; i < scanrowsMax; ++i)
		{
			scanrowBases[i] = quadCount;
			quadCount += scanrowVert[i].size() / 4;
		}

		scanrowBases[scanrowsMax] = quadCount;

		VBO::bind(tiles.vbo);
		VBO::allocEmpty(quadDataSize(quadCount));

		VBO::uploadSubData(0, quadDataSize(groundQuadCount), &groundVert[0]);

		for (size_t i = 0; i < scanrowsMax; ++i)
		{
			if (scanrowVert[i].empty())
				continue;

			VBO::uploadSubData(quadDataSize(scanrowBases[i]),
			                   quadDataSize(scanrowSize(i)), &scanrowVert[i][0]);
		}

		VBO::unbind();

		/* Ensure global IBO size */
		shState->ensureQuadIBO(quadCount);
	}

	void bindShader(ShaderBase *&shaderVar)
	{
		if (tiles.animated)
		{
			TilemapShader &tilemapShader = shState->shaders().tilemap;
			tilemapShader.bind();
			tilemapShader.setAniIndex(tiles.frameIdx);
			shaderVar = &tilemapShader;
		}
		else
		{
			shaderVar = &shState->shaders().simple;
			shaderVar->bind();
		}

		shaderVar->applyViewportProj();
	}

	void bindAtlas(ShaderBase &shader)
	{
		TEX::bind(atlas.gl.tex);
		shader.setTexSize(atlas.size);
	}

	bool sampleFlashColor(Vec4 &out, int x, int y)
	{
		int16_t packed = tableGetWrapped(flashData, x, y);

		if (packed == 0)
			return false;

		const float max = 0xF;

		float b = ((packed & 0x000F) >> 0) / max;
		float g = ((packed & 0x00F0) >> 4) / max;
		float r = ((packed & 0x0F00) >> 8) / max;

		out = Vec4(r, g, b, 1);

		return true;
	}

	void updateFlash()
	{
		if (!flashData)
			return;

		std::vector<CVertex> vertices;

		for (int x = 0; x < viewpW; ++x)
			for (int y = 0; y < viewpH; ++y)
			{
				Vec4 color;
				if (!sampleFlashColor(color, x+viewpPos.x, y+viewpPos.y))
					continue;

				FloatRect posRect(x*32, y*32, 32, 32);

				CVertex v[4];
				Quad::setPosRect(v, posRect);
				Quad::setColor(v, color);

				for (size_t i = 0; i < 4; ++i)
					vertices.push_back(v[i]);
			}

		flash.quadCount = vertices.size() / 4;

		if (flash.quadCount == 0)
			return;

		VBO::bind(flash.vbo);
		VBO::uploadData(sizeof(CVertex) * vertices.size(), &vertices[0]);
		VBO::unbind();

		/* Ensure global IBO size */
		shState->ensureQuadIBO(flash.quadCount);
	}

	void updateActiveElements(std::vector<int> &scanrowInd)
	{
		elem.ground->updateVboCount();

		for (size_t i = 0; i < scanrowsMax; ++i)
		{
			if (i < scanrowInd.size())
			{
				int index = scanrowInd[i];
				elem.scanrows[i]->setVisible(visible);
				elem.scanrows[i]->setIndex(index);
			}
			else
			{
				/* Hide unused rows */
				elem.scanrows[i]->setVisible(false);
			}
		}
	}

	void updateSceneElements()
	{
		/* Only allocate elements for non-emtpy scanrows */
		std::vector<int> scanrowInd;

		for (size_t i = 0; i < scanrowsMax; ++i)
			if (scanrowVert[i].size() > 0)
				scanrowInd.push_back(i);

		updateActiveElements(scanrowInd);
		elem.activeRows = scanrowInd.size();
		zOrderDirty = false;
	}

	void hideElements()
	{
		elem.ground->setVisible(false);

		for (size_t i = 0; i < scanrowsMax; ++i)
			elem.scanrows[i]->setVisible(false);
	}

	void updateZOrder()
	{
		if (elem.activeRows == 0)
			return;

		for (size_t i = 0; i < elem.activeRows; ++i)
			elem.scanrows[i]->initUpdateZ();

		ScanRow *prev = elem.scanrows[0];
		prev->finiUpdateZ(0);

		for (size_t i = 1; i < elem.activeRows; ++i)
		{
			ScanRow *row = elem.scanrows[i];
			row->finiUpdateZ(prev);
			prev = row;
		}
	}

	/* When there are two or more scanrows with no other
	 * elements between them in the scene list, we can
	 * render them in a batch (as the scanrow data itself
	 * is ordered sequentially in VRAM). Every frame, we
	 * scan the scene list for such sequential rows and
	 * batch them up for drawing. The first row of the batch
	 * (the "batch head") executes the draw call, all others
	 * are muted via the 'batchedFlag'. For simplicity,
	 * single sized batches are possible. */
	void prepareScanrowBatches()
	{
		ScanRow *const *scanrows = elem.scanrows;

		for (size_t i = 0; i < elem.activeRows; ++i)
		{
			ScanRow *batchHead = scanrows[i];
			batchHead->batchedFlag = false;

			GLsizei vboBatchCount = batchHead->vboCount;
			IntruListLink<SceneElement> *iter = &batchHead->link;

			for (i = i+1; i < elem.activeRows; ++i)
			{
				iter = iter->next;
				ScanRow *row = scanrows[i];

				/* Check if the next SceneElement is also
				 * the next scanrow in our list. If not,
				 * the current batch is complete */
				if (iter != &row->link)
					break;

				vboBatchCount += row->vboCount;
				row->batchedFlag = true;
			}

			batchHead->vboBatchCount = vboBatchCount;
			--i;
		}
	}

	void updateMapViewport()
	{
		int tileOX, tileOY;

		if (offset.x >= 0)
			tileOX = offset.x / 32;
		else
			tileOX = -(-(offset.x-31) / 32);

		if (offset.y >= 0)
			tileOY = offset.y / 32;
		else
			tileOY = -(-(offset.y-31) / 32);

		bool dirty = false;

		if (tileOX < viewpPos.x || tileOX + 21 > viewpPos.x + viewpW)
		{
			viewpPos.x = tileOX;
			dirty = true;
		}

		if (tileOY < viewpPos.y || tileOY + 16 > viewpPos.y + viewpH)
		{
			viewpPos.y = tileOY;
			dirty = true;
		}

		if (dirty)
		{
			buffersDirty = true;
			flashDirty = true;
			updatePosition();
		}
	}

	void prepare()
	{
		if (!verifyResources())
		{
			if (tilemapReady)
				hideElements();
			tilemapReady = false;

			return;
		}

		if (atlasSizeDirty)
		{
			allocateAtlas();
			atlasSizeDirty = false;
		}

		if (atlasDirty)
		{
			buildAtlas();
			atlasDirty = false;
		}

		if (mapViewportDirty)
		{
			updateMapViewport();
			mapViewportDirty = false;
		}

		if (buffersDirty)
		{
			buildQuadArray();
			uploadBuffers();
			updateSceneElements();
			buffersDirty = false;
		}

		if (flashDirty)
		{
			updateFlash();
			flashDirty = false;
		}

		if (zOrderDirty)
		{
			updateZOrder();
			zOrderDirty = false;
		}

		prepareScanrowBatches();

		tilemapReady = true;
	}
};

GroundLayer::GroundLayer(TilemapPrivate *p, Viewport *viewport)
    : ViewportElement(viewport, 0, p->elem.groundStamp),
      vboCount(0),
      p(p)
{
	onGeometryChange(scene->getGeometry());
}

void GroundLayer::updateVboCount()
{
	vboCount = p->scanrowBases[0] * 6;
}

void GroundLayer::draw()
{
	ShaderBase *shader;

	p->bindShader(shader);
	p->bindAtlas(*shader);

	VAO::bind(p->tiles.vao);

	shader->setTranslation(p->dispPos);
	drawInt();

	if (p->flash.quadCount > 0)
	{
		VAO::bind(p->flash.vao);
		glState.blendMode.pushSet(BlendAddition);

		FlashMapShader &shader = shState->shaders().flashMap;
		shader.bind();
		shader.applyViewportProj();
		shader.setAlpha(flashAlpha[p->flash.alphaIdx] / 255.f);
		shader.setTranslation(p->dispPos);

		drawFlashInt();

		glState.blendMode.pop();
	}

	VAO::unbind();
}

void GroundLayer::drawInt()
{
	gl.DrawElements(GL_TRIANGLES, vboCount, GL_UNSIGNED_INT, (GLvoid*) 0);
}

void GroundLayer::drawFlashInt()
{
	gl.DrawElements(GL_TRIANGLES, p->flash.quadCount * 6, GL_UNSIGNED_INT, 0);
}

void GroundLayer::onGeometryChange(const Scene::Geometry &geo)
{
	p->updateSceneGeometry(geo);
	p->updatePosition();
}

ScanRow::ScanRow(TilemapPrivate *p, Viewport *viewport)
    : ViewportElement(viewport, 0, p->elem.scanrowStamp),
      index(0),
      vboOffset(0),
      vboCount(0),
      p(p),
      vboBatchCount(0)
{}

void ScanRow::setIndex(int value)
{
	index = value;

	z = calculateZ(p, index);
	scene->reinsert(*this);

	vboOffset = p->scanrowBases[index] * sizeof(uint32_t) * 6;
	vboCount = p->scanrowSize(index) * 6;
}

void ScanRow::draw()
{
	if (batchedFlag)
		return;

	ShaderBase *shader;

	p->bindShader(shader);
	p->bindAtlas(*shader);

	VAO::bind(p->tiles.vao);

	shader->setTranslation(p->dispPos);
	drawInt();

	VAO::unbind();
}

void ScanRow::drawInt()
{
	gl.DrawElements(GL_TRIANGLES, vboBatchCount, GL_UNSIGNED_INT, (GLvoid*) vboOffset);
}

int ScanRow::calculateZ(TilemapPrivate *p, int index)
{
	return 32 * (index + p->viewpPos.y + 1) - p->offset.y;
}

void ScanRow::initUpdateZ()
{
	unlink();
}

void ScanRow::finiUpdateZ(ScanRow *prev)
{
	z = calculateZ(p, index);

	if (prev)
		scene->insertAfter(*this, *prev);
	else
		scene->insert(*this);
}

void Tilemap::Autotiles::set(int i, Bitmap *bitmap)
{
	if (i < 0 || i > autotileCount-1)
		return;

	if (p->autotiles[i] == bitmap)
		return;

	p->autotiles[i] = bitmap;

	p->invalidateAtlasContents();

	p->autotilesCon[i].disconnect();
	p->autotilesCon[i] = bitmap->modified.connect
	        (sigc::mem_fun(p, &TilemapPrivate::invalidateAtlasContents));

	p->autotilesDispCon[i].disconnect();
	p->autotilesDispCon[i] = bitmap->wasDisposed.connect
	        (sigc::mem_fun(p, &TilemapPrivate::invalidateAtlasContents));

	p->updateAutotileInfo();
}

Bitmap *Tilemap::Autotiles::get(int i) const
{
	if (i < 0 || i > autotileCount-1)
		return 0;

	return p->autotiles[i];
}

Tilemap::Tilemap(Viewport *viewport)
{
	p = new TilemapPrivate(viewport);
	p->autotilesProxy.p = p;
}

Tilemap::~Tilemap()
{
	dispose();
}

void Tilemap::update()
{
	if (!p->tilemapReady)
		return;

	/* Animate flash */
	if (++p->flash.alphaIdx >= flashAlphaN)
		p->flash.alphaIdx = 0;

	/* Animate autotiles */
	if (!p->tiles.animated)
		return;

	p->tiles.frameIdx = atAnimation[p->tiles.aniIdx];

	if (++p->tiles.aniIdx >= atAnimationN)
		p->tiles.aniIdx = 0;
}

Tilemap::Autotiles &Tilemap::getAutotiles() const
{
	return p->autotilesProxy;
}

#define DISP_CLASS_NAME "tilemap"

DEF_ATTR_RD_SIMPLE(Tilemap, Viewport, Viewport*, p->viewport)
DEF_ATTR_RD_SIMPLE(Tilemap, Tileset, Bitmap*, p->tileset)
DEF_ATTR_RD_SIMPLE(Tilemap, MapData, Table*, p->mapData)
DEF_ATTR_RD_SIMPLE(Tilemap, FlashData, Table*, p->flashData)
DEF_ATTR_RD_SIMPLE(Tilemap, Priorities, Table*, p->priorities)
DEF_ATTR_RD_SIMPLE(Tilemap, Visible, bool, p->visible)
DEF_ATTR_RD_SIMPLE(Tilemap, OX, int, p->offset.x)
DEF_ATTR_RD_SIMPLE(Tilemap, OY, int, p->offset.y)

#ifdef RGSS2

void Tilemap::setViewport(Viewport *value)
{
	GUARD_DISPOSED

	if (p->viewport == value)
		return;

	p->viewport = value;

	if (!p->tilemapReady)
		return;

	p->elem.ground->setViewport(value);

	for (size_t i = 0; i < p->elem.scanrows.size(); ++i)
		p->elem.scanrows[i]->setViewport(value);
}

#endif

void Tilemap::setTileset(Bitmap *value)
{
	GUARD_DISPOSED

	if (p->tileset == value)
		return;

	p->tileset = value;

	p->invalidateAtlasSize();
	p->tilesetCon.disconnect();
	p->tilesetCon = value->modified.connect
	        (sigc::mem_fun(p, &TilemapPrivate::invalidateAtlasSize));

	p->updateAtlasInfo();
}

void Tilemap::setMapData(Table *value)
{
	GUARD_DISPOSED

	if (p->mapData == value)
		return;

	p->mapData = value;

	p->invalidateBuffers();
	p->mapDataCon.disconnect();
	p->mapDataCon = value->modified.connect
	        (sigc::mem_fun(p, &TilemapPrivate::invalidateBuffers));
}

void Tilemap::setFlashData(Table *value)
{
	GUARD_DISPOSED

	if (p->flashData == value)
		return;

	p->flashData = value;

	p->invalidateFlash();
	p->flashDataCon.disconnect();
	p->flashDataCon = value->modified.connect
	        (sigc::mem_fun(p, &TilemapPrivate::invalidateFlash));
}

void Tilemap::setPriorities(Table *value)
{
	GUARD_DISPOSED

	if (p->priorities == value)
		return;

	p->priorities = value;

	p->invalidateBuffers();
	p->prioritiesCon.disconnect();
	p->prioritiesCon = value->modified.connect
	        (sigc::mem_fun(p, &TilemapPrivate::invalidateBuffers));
}

void Tilemap::setVisible(bool value)
{
	GUARD_DISPOSED

	if (p->visible == value)
		return;

	p->visible = value;

	if (!p->tilemapReady)
		return;

	p->elem.ground->setVisible(value);
	for (size_t i = 0; i < p->elem.activeRows; ++i)
		p->elem.scanrows[i]->setVisible(value);
}

void Tilemap::setOX(int value)
{
	GUARD_DISPOSED

	if (p->offset.x == value)
		return;

	p->offset.x = value;
	p->updatePosition();
	p->mapViewportDirty = true;
}

void Tilemap::setOY(int value)
{
	GUARD_DISPOSED

	if (p->offset.y == value)
		return;

	p->offset.y = value;
	p->updatePosition();
	p->zOrderDirty = true;
	p->mapViewportDirty = true;
}


void Tilemap::releaseResources()
{
	delete p;
}
