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
#include "config.h"
#include "glstate.h"
#include "gl-util.h"
#include "gl-meta.h"
#include "global-ibo.h"
#include "etc-internal.h"
#include "quadarray.h"
#include "texpool.h"
#include "quad.h"
#include "vertex.h"
#include "tileatlas.h"
#include "tilemap-common.h"

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

static const int atFrames = 8;
static const int atFrameDur = 15;
static const int atAreaW = autotileW * atFrames;
static const int atAreaH = autotileH * autotileCount;

static const int tsLaneW = tilesetW / 1;

/* Map viewport size */
static const int viewpW = 21;
static const int viewpH = 16;

static const size_t zlayersMax = viewpH + 5;

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
 *   | AT0 | AT0 | AT0 | AT0 |       ¦       |
 *   | FR0 | FR1 | FR2 | FR3 |   |   ¦   |   |
 *   |-----|-----|-----|-----|   v   ¦   v   |
 *   |     |     |     |     |       ¦       |
 *   | AT1 |     |     |     |       ¦       |
 *   |     |     |     |     |       ¦       |
 *   |-----|-----|-----|-----|       ¦       |
 *   |[...]|     |     |     |       ¦       |
 *   |-----|-----|-----|-----|       ¦       |
 *   |     |     |     |     |   |   ¦   |   |
 *   | AT6 |     |     |     |   v   ¦   v   |
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
 *   such elements (GroundLayer and ZLayers).
 *
 * GroundLayer:
 *   Every tile with priority=0 is drawn at z=0, so we
 *   collect all such tiles in one big quad array and
 *   draw them at once.
 *
 * ZLayer:
 *   Each tile in row n with priority=m is drawn at the same
 *   z as every tile in row n-1 with priority=m-1. This means
 *   we can collect all tiles sharing the same z in one quad
 *   array and draw them at once. I call these collections
 *   'zlayers'. They're drawn from the top part of the map
 *   (lowest z) to the bottom part (highest z).
 *   Objects that would end up on the same zlayer are eg. trees.
 *
 * Map viewport:
 *   This rectangle describes the subregion of the map that is
 *   actually translated to vertices and stored on the GPU ready
 *   for rendering. Whenever, ox/oy are modified, its position is
 *   adjusted if necessary and the data is regenerated. Its size
 *   is fixed. This is NOT related to the RGSS Viewport class!
 *
 */

/* Autotile animation */
// static const uint8_t atAnimation[16*8] =
// {
//     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
//     1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
//     2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
//     3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
//     4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
//     5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
//     6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
//     7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7
// };

// static elementsN(atAnimation);

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

	void onGeometryChange(const Scene::Geometry &geo);

	ABOUT_TO_ACCESS_NOOP
};

struct ZLayer : public ViewportElement
{
	size_t index;
	GLintptr vboOffset;
	GLsizei vboCount;
	TilemapPrivate *p;

	/* If this layer is part of a batch and not
	 * the head, it is 'muted' via this flag */
	bool batchedFlag;

	/* If this layer is a batch head, this variable
	 * holds the element count of the entire batch */
	GLsizei vboBatchCount;

	ZLayer(TilemapPrivate *p, Viewport *viewport);

	void setIndex(int value);

	void draw();
	void drawInt();

	static int calculateZ(TilemapPrivate *p, int index);

	void initUpdateZ();
	void finiUpdateZ(ZLayer *prev);

	ABOUT_TO_ACCESS_NOOP
};

struct TilemapPrivate
{
	Viewport *viewport;

	Bitmap *autotiles[autotileCount];

	Bitmap *tileset;

	Table *mapData;
	Table *priorities;
	bool visible;
	Vec2i origin;

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

		/* Whether each autotile is 3x4 or not */
		bool smallATs[autotileCount] = {false};

		/* The number of frames for each autotile */
		int nATFrames[autotileCount] = {1};
	} atlas;

	/* Map viewport position */
	Vec2i viewpPos;

	/* Ground layer vertices */
	SVVector groundVert;

	/* ZLayer vertices */
	SVVector zlayerVert[zlayersMax];

	/* Base quad indices of each zlayer
	 * in the shared buffer */
	size_t zlayerBases[zlayersMax+1];

	/* Shared buffers for all tiles */
	struct
	{
		GLMeta::VAO vao;
		VBO::ID vbo;
		bool animated;

		/* Animation state */
		uint32_t aniIdx;
	} tiles;

	FlashMap flashMap;
	uint8_t flashAlphaIdx;

	/* Scene elements */
	struct
	{
		GroundLayer *ground;
		ZLayer* zlayers[zlayersMax];
		/* Used layers out of 'zlayers' (rest is hidden) */
		size_t activeLayers;
		Scene::Geometry sceneGeo;
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

	/* Resources are sufficient and tilemap is ready to be drawn */
	bool tilemapReady;

	/* Change watches */
	sigc::connection tilesetCon;
	sigc::connection autotilesCon[autotileCount];
	sigc::connection mapDataCon;
	sigc::connection prioritiesCon;

	/* Dispose watches */
	sigc::connection autotilesDispCon[autotileCount];

	/* Draw prepare call */
	sigc::connection prepareCon;

	NormValue opacity;
	BlendType blendType;
	Color *color;
	Tone *tone;

	EtcTemps tmp;

	TilemapPrivate(Viewport *viewport)
	    : viewport(viewport),
	      tileset(0),
	      mapData(0),
	      priorities(0),
	      visible(true),
	      flashAlphaIdx(0),
	      atlasSizeDirty(false),
	      atlasDirty(false),
	      buffersDirty(false),
	      mapViewportDirty(false),
	      zOrderDirty(false),
	      tilemapReady(false),

		  opacity(255),
	      blendType(BlendNormal),
	      color(&tmp.color),
	      tone(&tmp.tone)
	{
		memset(autotiles, 0, sizeof(autotiles));

		atlas.animatedATs.reserve(autotileCount);
		atlas.efTilesetH = 0;

		tiles.animated = false;
		tiles.aniIdx = 0;

		/* Init tile buffers */
		tiles.vbo = VBO::gen();

		GLMeta::vaoFillInVertexData<SVertex>(tiles.vao);
		tiles.vao.vbo = tiles.vbo;
		tiles.vao.ibo = shState->globalIBO().ibo;

		GLMeta::vaoInit(tiles.vao);

		elem.ground = new GroundLayer(this, viewport);

		for (size_t i = 0; i < zlayersMax; ++i)
			elem.zlayers[i] = new ZLayer(this, viewport);

		prepareCon = shState->prepareDraw.connect
		        (sigc::mem_fun(this, &TilemapPrivate::prepare));

		updateFlashMapViewport();
	}

	~TilemapPrivate()
	{
		/* Destroy elements */
		delete elem.ground;
		for (size_t i = 0; i < zlayersMax; ++i)
			delete elem.zlayers[i];

		shState->releaseAtlasTex(atlas.gl);

		/* Destroy tile buffers */
		GLMeta::vaoFini(tiles.vao);
		VBO::del(tiles.vbo);

		/* Disconnect signal handlers */
		tilesetCon.disconnect();
		for (int i = 0; i < autotileCount; ++i)
		{
			autotilesCon[i].disconnect();
			autotilesDispCon[i].disconnect();
		}
		mapDataCon.disconnect();
		prioritiesCon.disconnect();

		prepareCon.disconnect();
	}

	void updateFlashMapViewport()
	{
		flashMap.setViewport(IntRect(viewpPos, Vec2i(viewpW, viewpH)));
	}

	void updateAtlasInfo()
	{
		if (nullOrDisposed(tileset))
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
		animatedATs.clear();

		for (int i = 0; i < autotileCount; ++i)
		{
			if (nullOrDisposed(autotiles[i]) || autotiles[i]->megaSurface())
			{
				atlas.nATFrames[i] = 1;
				continue;
			}

			usableATs.push_back(i);

			if (autotiles[i]->height() == 32)
			{
				atlas.smallATs[i] = true;
				atlas.nATFrames[i] = autotiles[i]->width()/32;
				animatedATs.push_back(i);
			}
			else
			{
				atlas.smallATs[i] = false;
				atlas.nATFrames[i] = autotiles[i]->width()/autotileW;
				if (atlas.nATFrames[i] > 1)
					animatedATs.push_back(i);
			}
		}

		tiles.animated = !animatedATs.empty();
	}

	void updateSceneGeometry(const Scene::Geometry &geo)
	{
		elem.sceneGeo = geo;
		mapViewportDirty = true;
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

	/* Checks for the minimum amount of data needed to display */
	bool verifyResources()
	{
		if (nullOrDisposed(tileset))
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
		FBO::bind(atlas.gl.fbo);
		glState.clearColor.pushSet(Vec4());
		glState.scissorTest.pushSet(false);

		FBO::clear();

		glState.scissorTest.pop();
		glState.clearColor.pop();

		GLMeta::blitBegin(atlas.gl);

		/* Blit autotiles */
		for (size_t i = 0; i < atlas.usableATs.size(); ++i)
		{
			const uint8_t atInd = atlas.usableATs[i];
			Bitmap *autotile = autotiles[atInd];

			int atW = autotile->width();
			int atH = autotile->height();
			int blitW = std::min(atW, atAreaW);
			int blitH = std::min(atH, autotileH);

			GLMeta::blitSource(autotile->getGLTypes());

			if (atW <= autotileW && tiles.animated && !atlas.smallATs[atInd])
			{
				/* Static autotile */
				for (int j = 0; j < atFrames; ++j)
					GLMeta::blitRectangle(IntRect(0, 0, blitW, blitH),
					                      Vec2i(autotileW*j, atInd*autotileH));
			}
			else
			{
				/* Animated autotile */
				if (atlas.smallATs[atInd])
				{
					int frames = atW/32;
					for (int j = 0; j < atFrames*autotileH/32; ++j)
					{
						GLMeta::blitRectangle(IntRect(32*(j % frames), 0, 32, 32),
						                      Vec2i(autotileW*(j % atFrames), atInd*autotileH + 32*(j / atFrames)));
					}
				}
				else
					GLMeta::blitRectangle(IntRect(0, 0, blitW, blitH),
					                      Vec2i(0, atInd*autotileH));
			}
		}

		GLMeta::blitEnd();

		/* Blit tileset */
		if (tileset->megaSurface())
		{
			/* Mega surface tileset */
			SDL_Surface *tsSurf = tileset->megaSurface();

			if (shState->config().subImageFix)
			{
				/* Implementation for broken GL drivers */
				FBO::bind(atlas.gl.fbo);
				glState.blend.pushSet(false);
				glState.viewport.pushSet(IntRect(0, 0, atlas.size.x, atlas.size.y));

				SimpleShader &shader = shState->shaders().simple;
				shader.bind();
				shader.applyViewportProj();
				shader.setTranslation(Vec2i());

				Quad &quad = shState->gpQuad();

				for (size_t i = 0; i < blits.size(); ++i)
				{
					const TileAtlas::Blit &blitOp = blits[i];

					Vec2i texSize;
					shState->ensureTexSize(tsLaneW, blitOp.h, texSize);
					shState->bindTex();
					GLMeta::subRectImageUpload(tsSurf->w, blitOp.src.x, blitOp.src.y,
					                           0, 0, tsLaneW, blitOp.h, tsSurf, GL_RGBA);

					shader.setTexSize(texSize);
					quad.setTexRect(FloatRect(0, 0, tsLaneW, blitOp.h));
					quad.setPosRect(FloatRect(blitOp.dst.x, blitOp.dst.y, tsLaneW, blitOp.h));

					quad.draw();
				}

				GLMeta::subRectImageEnd();
				glState.viewport.pop();
				glState.blend.pop();
			}
			else
			{
				/* Clean implementation */
				TEX::bind(atlas.gl.tex);

				for (size_t i = 0; i < blits.size(); ++i)
				{
					const TileAtlas::Blit &blitOp = blits[i];

					GLMeta::subRectImageUpload(tsSurf->w, blitOp.src.x, blitOp.src.y,
					                           blitOp.dst.x, blitOp.dst.y, tsLaneW, blitOp.h, tsSurf, GL_RGBA);
				}

				GLMeta::subRectImageEnd();
			}

		}
		else
		{
			/* Regular tileset */
			GLMeta::blitBegin(atlas.gl);
			GLMeta::blitSource(tileset->getGLTypes());

			for (size_t i = 0; i < blits.size(); ++i)
			{
				const TileAtlas::Blit &blitOp = blits[i];

				GLMeta::blitRectangle(IntRect(blitOp.src.x, blitOp.src.y, tsLaneW, blitOp.h),
				                      blitOp.dst);
			}

			GLMeta::blitEnd();
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

	void handleAutotile(int x, int y, int tileInd, SVVector *array)
	{
		/* Which autotile [0-7] */
		int atInd = tileInd / 48 - 1;
		if (!atlas.smallATs[atInd])
		{
			/* Which tile pattern of the autotile [0-47] */
			int subInd = tileInd % 48;

			const StaticRect *pieceRect = &autotileRects[subInd*4];

			/* Iterate over the 4 tile pieces */
			for (int i = 0; i < 4; ++i)
			{
				FloatRect posRect(x*32, y*32, 16, 16);
				atSelectSubPos(posRect, i);

				FloatRect texRect = pieceRect[i];

				/* Adjust to atlas coordinates */
				texRect.y += atInd * autotileH;

				SVertex v[4];
				Quad::setTexPosRect(v, texRect, posRect);

				/* Iterate over 4 vertices */
				for (size_t j = 0; j < 4; ++j)
					array->push_back(v[j]);
			}
		}
		else
		{
			FloatRect posRect(x*32, y*32, 32, 32);
			FloatRect texRect(0.5f, atInd * autotileH + 0.5f, 31, 31);
			SVertex v[4];
			Quad::setTexPosRect(v, texRect, posRect);

			/* Iterate over 4 vertices */
			for (size_t j = 0; j < 4; ++j)
				array->push_back(v[j]);
		}
	}

	void handleTile(int x, int y, int z)
	{
		int tileInd =
			tableGetWrapped(*mapData, x + viewpPos.x, y + viewpPos.y, z);

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
			int layerInd = y + prio;
			if (layerInd >= zlayersMax)
				return;
			targetArray = &zlayerVert[layerInd];
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
		FloatRect texRect((float) texPos.x+0.5f, (float) texPos.y+0.5f, 31, 31);
		FloatRect posRect(x*32, y*32, 32, 32);

		SVertex v[4];
		Quad::setTexPosRect(v, texRect, posRect);

		for (size_t i = 0; i < 4; ++i)
			targetArray->push_back(v[i]);
	}

	void clearQuadArrays()
	{
		groundVert.clear();

		for (size_t i = 0; i < zlayersMax; ++i)
			zlayerVert[i].clear();
	}

	void buildQuadArray()
	{
		clearQuadArrays();

		int ox = viewpPos.x;
		int oy = viewpPos.y;
		int mapW = mapData->xSize();
		int mapH = mapData->ySize();

		int minX = 0;
		int minY = 0;
		if (ox < 0)
			minX = -ox;
		if (oy < 0)
			minY = -oy;

		// There could be off-by-one issues in these couple sections.
		int maxX = viewpW;
		int maxY = viewpH;
		if (ox + maxX >= mapW)
			maxX = mapW - ox - 1;
		if (oy + maxY >= mapH)
			maxY = mapH - oy - 1;

		if ((minX > maxX) || (minY > maxY))
			return;
		for (int x = minX; x <= maxX; ++x)
			for (int y = minY; y <= maxY; ++y)
				for (int z = 0; z < mapData->zSize(); ++z)
					handleTile(x, y, z);
	}

	static size_t quadDataSize(size_t quadCount)
	{
		return quadCount * sizeof(SVertex) * 4;
	}

	size_t zlayerSize(size_t index)
	{
		return zlayerBases[index+1] - zlayerBases[index];
	}

	void uploadBuffers()
	{
		/* Calculate total quad count */
		size_t groundQuadCount = groundVert.size() / 4;
		size_t quadCount = groundQuadCount;

		for (size_t i = 0; i < zlayersMax; ++i)
		{
			zlayerBases[i] = quadCount;
			quadCount += zlayerVert[i].size() / 4;
		}

		zlayerBases[zlayersMax] = quadCount;

		VBO::bind(tiles.vbo);
		VBO::allocEmpty(quadDataSize(quadCount));

		VBO::uploadSubData(0, quadDataSize(groundQuadCount), dataPtr(groundVert));

		for (size_t i = 0; i < zlayersMax; ++i)
		{
			if (zlayerVert[i].empty())
				continue;

			VBO::uploadSubData(quadDataSize(zlayerBases[i]),
			                   quadDataSize(zlayerSize(i)), dataPtr(zlayerVert[i]));
		}

		VBO::unbind();

		/* Ensure global IBO size */
		shState->ensureQuadIBO(quadCount);
	}

	void bindShader(ShaderBase *&shaderVar)
	{
		if (tiles.animated || color->hasEffect() || tone->hasEffect() || opacity != 255)
		{
			TilemapShader &tilemapShader = shState->shaders().tilemap;
			tilemapShader.bind();
			tilemapShader.applyViewportProj();
			tilemapShader.setTone(tone->norm);
			tilemapShader.setColor(color->norm);
			tilemapShader.setOpacity(opacity.norm);
			tilemapShader.setAniIndex(tiles.aniIdx / atFrameDur);
			tilemapShader.setATFrames(atlas.nATFrames);
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

	void updateActiveElements(std::vector<int> &zlayerInd)
	{
		elem.ground->updateVboCount();

		for (size_t i = 0; i < zlayersMax; ++i)
		{
			if (i < zlayerInd.size())
			{
				int index = zlayerInd[i];
				elem.zlayers[i]->setVisible(visible);
				elem.zlayers[i]->setIndex(index);
			}
			else
			{
				/* Hide unused layers */
				elem.zlayers[i]->setVisible(false);
			}
		}
	}

	void updateSceneElements()
	{
		/* Only allocate elements for non-emtpy zlayers */
		std::vector<int> zlayerInd;

		for (size_t i = 0; i < zlayersMax; ++i)
			if (zlayerVert[i].size() > 0)
				zlayerInd.push_back(i);

		updateActiveElements(zlayerInd);
		elem.activeLayers = zlayerInd.size();
		zOrderDirty = false;
	}

	void hideElements()
	{
		elem.ground->setVisible(false);

		for (size_t i = 0; i < zlayersMax; ++i)
			elem.zlayers[i]->setVisible(false);
	}

	void updateZOrder()
	{
		if (elem.activeLayers == 0)
			return;

		for (size_t i = 0; i < elem.activeLayers; ++i)
			elem.zlayers[i]->initUpdateZ();

		ZLayer *prev = elem.zlayers[0];
		prev->finiUpdateZ(0);

		for (size_t i = 1; i < elem.activeLayers; ++i)
		{
			ZLayer *layer = elem.zlayers[i];
			layer->finiUpdateZ(prev);
			prev = layer;
		}
	}

	/* When there are two or more zlayers with no other
	 * elements between them in the scene list, we can
	 * render them in a batch (as the zlayer data itself
	 * is ordered sequentially in VRAM). Every frame, we
	 * scan the scene list for such sequential layers and
	 * batch them up for drawing. The first layer of the batch
	 * (the "batch head") executes the draw call, all others
	 * are muted via the 'batchedFlag'. For simplicity,
	 * single sized batches are possible. */
	void prepareZLayerBatches()
	{
		ZLayer *const *zlayers = elem.zlayers;

		for (size_t i = 0; i < elem.activeLayers; ++i)
		{
			ZLayer *batchHead = zlayers[i];
			batchHead->batchedFlag = false;

			GLsizei vboBatchCount = batchHead->vboCount;
			IntruListLink<SceneElement> *iter = &batchHead->link;

			for (i = i+1; i < elem.activeLayers; ++i)
			{
				iter = iter->next;
				ZLayer *layer = zlayers[i];

				/* Check if the next SceneElement is also
				 * the next zlayer in our list. If not,
				 * the current batch is complete */
				if (iter != &layer->link)
					break;

				vboBatchCount += layer->vboCount;
				layer->batchedFlag = true;
			}

			batchHead->vboBatchCount = vboBatchCount;
			--i;
		}
	}

	void updateMapViewport()
	{
		const Vec2i combOrigin = origin + elem.sceneGeo.orig;
		const Vec2i mvpPos = getTilePos(combOrigin);

		if (mvpPos != viewpPos)
		{
			viewpPos = mvpPos;
			buffersDirty = true;
			updateFlashMapViewport();
		}

		dispPos = elem.sceneGeo.rect.pos() - wrap(combOrigin, 32);
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

		flashMap.prepare();

		if (zOrderDirty)
		{
			updateZOrder();
			zOrderDirty = false;
		}

		prepareZLayerBatches();

		tilemapReady = true;
	}
};

GroundLayer::GroundLayer(TilemapPrivate *p, Viewport *viewport)
    : ViewportElement(viewport, 0),
      vboCount(0),
      p(p)
{
	onGeometryChange(scene->getGeometry());
}

void GroundLayer::updateVboCount()
{
	vboCount = p->zlayerBases[0] * 6;
}

void GroundLayer::draw()
{
	if (p->groundVert.size() == 0)
		return;

	if (!p->opacity)
		return;

	ShaderBase *shader;

	p->bindShader(shader);
	p->bindAtlas(*shader);

	glState.blendMode.pushSet(p->blendType);

	GLMeta::vaoBind(p->tiles.vao);

	shader->setTranslation(p->dispPos);
	drawInt();

	GLMeta::vaoUnbind(p->tiles.vao);

	p->flashMap.draw(flashAlpha[p->flashAlphaIdx] / 255.f, p->dispPos);

	glState.blendMode.pop();
}

void GroundLayer::drawInt()
{
	gl.DrawElements(GL_TRIANGLES, vboCount, _GL_INDEX_TYPE, (GLvoid*) 0);
}

void GroundLayer::onGeometryChange(const Scene::Geometry &geo)
{
	p->updateSceneGeometry(geo);
}

ZLayer::ZLayer(TilemapPrivate *p, Viewport *viewport)
    : ViewportElement(viewport, 0),
      index(0),
      vboOffset(0),
      vboCount(0),
      p(p),
      vboBatchCount(0)
{}

void ZLayer::setIndex(int value)
{
	index = value;

	z = calculateZ(p, index);
	scene->reinsert(*this);

	vboOffset = p->zlayerBases[index] * sizeof(index_t) * 6;
	vboCount = p->zlayerSize(index) * 6;
}

void ZLayer::draw()
{
	if (batchedFlag)
		return;

	ShaderBase *shader;

	p->bindShader(shader);
	p->bindAtlas(*shader);

	glState.blendMode.pushSet(p->blendType);

	GLMeta::vaoBind(p->tiles.vao);

	shader->setTranslation(p->dispPos);
	drawInt();

	GLMeta::vaoUnbind(p->tiles.vao);

	glState.blendMode.pop();
}

void ZLayer::drawInt()
{
	gl.DrawElements(GL_TRIANGLES, vboBatchCount, _GL_INDEX_TYPE, (GLvoid*) vboOffset);
}

int ZLayer::calculateZ(TilemapPrivate *p, int index)
{
	return 32 * (index + p->viewpPos.y + 1) - p->origin.y;
}

void ZLayer::initUpdateZ()
{
	unlink();
}

void ZLayer::finiUpdateZ(ZLayer *prev)
{
	z = calculateZ(p, index);

	if (prev)
		scene->insertAfter(*this, *prev);
	else
		scene->insert(*this);
}

void Tilemap::Autotiles::set(int i, Bitmap *bitmap)
{
	if (!p)
		return;

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
	if (!p)
		return 0;

	if (i < 0 || i > autotileCount-1)
		return 0;

	return p->autotiles[i];
}

Tilemap::Tilemap(Viewport *viewport)
{
	p = new TilemapPrivate(viewport);
	atProxy.p = p;
}

Tilemap::~Tilemap()
{
	dispose();
}

void Tilemap::update()
{
	guardDisposed();

	if (!p->tilemapReady)
		return;

	/* Animate flash */
	if (++p->flashAlphaIdx >= flashAlphaN)
		p->flashAlphaIdx = 0;

	/* Animate autotiles */
	if (!p->tiles.animated)
		return;

	++p->tiles.aniIdx;
}

Tilemap::Autotiles &Tilemap::getAutotiles()
{
	guardDisposed();

	return atProxy;
}

DEF_ATTR_RD_SIMPLE(Tilemap, Viewport, Viewport*, p->viewport)
DEF_ATTR_RD_SIMPLE(Tilemap, Tileset, Bitmap*, p->tileset)
DEF_ATTR_RD_SIMPLE(Tilemap, MapData, Table*, p->mapData)
DEF_ATTR_RD_SIMPLE(Tilemap, FlashData, Table*, p->flashMap.getData())
DEF_ATTR_RD_SIMPLE(Tilemap, Priorities, Table*, p->priorities)
DEF_ATTR_RD_SIMPLE(Tilemap, Visible, bool, p->visible)
DEF_ATTR_RD_SIMPLE(Tilemap, OX, int, p->origin.x)
DEF_ATTR_RD_SIMPLE(Tilemap, OY, int, p->origin.y)

DEF_ATTR_RD_SIMPLE(Tilemap, BlendType, int, p->blendType)
DEF_ATTR_SIMPLE(Tilemap, Opacity,   int,     p->opacity)
DEF_ATTR_SIMPLE(Tilemap, Color,     Color&, *p->color)
DEF_ATTR_SIMPLE(Tilemap, Tone,      Tone&,  *p->tone)

void Tilemap::setTileset(Bitmap *value)
{
	guardDisposed();

	if (p->tileset == value)
		return;

	p->tileset = value;

	if (!value)
		return;

	p->invalidateAtlasSize();
	p->tilesetCon.disconnect();
	p->tilesetCon = value->modified.connect
	        (sigc::mem_fun(p, &TilemapPrivate::invalidateAtlasSize));

	p->updateAtlasInfo();
}

void Tilemap::setMapData(Table *value)
{
	guardDisposed();

	if (p->mapData == value)
		return;

	p->mapData = value;

	if (!value)
		return;

	p->invalidateBuffers();
	p->mapDataCon.disconnect();
	p->mapDataCon = value->modified.connect
	        (sigc::mem_fun(p, &TilemapPrivate::invalidateBuffers));
}

void Tilemap::setFlashData(Table *value)
{
	guardDisposed();

	p->flashMap.setData(value);
}

void Tilemap::setPriorities(Table *value)
{
	guardDisposed();

	if (p->priorities == value)
		return;

	p->priorities = value;

	if (!value)
		return;

	p->invalidateBuffers();
	p->prioritiesCon.disconnect();
	p->prioritiesCon = value->modified.connect
	        (sigc::mem_fun(p, &TilemapPrivate::invalidateBuffers));
}

void Tilemap::setVisible(bool value)
{
	guardDisposed();

	if (p->visible == value)
		return;

	p->visible = value;

	if (!p->tilemapReady)
		return;

	p->elem.ground->setVisible(value);
	for (size_t i = 0; i < p->elem.activeLayers; ++i)
		p->elem.zlayers[i]->setVisible(value);
}

void Tilemap::setOX(int value)
{
	guardDisposed();

	if (p->origin.x == value)
		return;

	p->origin.x = value;
	p->mapViewportDirty = true;
}

void Tilemap::setOY(int value)
{
	guardDisposed();

	if (p->origin.y == value)
		return;

	p->origin.y = value;
	p->zOrderDirty = true;
	p->mapViewportDirty = true;
}

void Tilemap::setBlendType(int value)
{
	guardDisposed();

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

void Tilemap::initDynAttribs()
{
	p->color = new Color;
	p->tone = new Tone;
}

void Tilemap::releaseResources()
{
	delete p;
	atProxy.p = 0;
}
