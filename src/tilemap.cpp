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

#include "globalstate.h"
#include "glstate.h"
#include "gl-util.h"
#include "etc-internal.h"
#include "quadarray.h"
#include "texpool.h"
#include "quad.h"

#include "sigc++/connection.h"

#include <string.h>
#include <stdint.h>

#include <QVector>

extern const StaticRect autotileRects[];

typedef QVector<SVertex> SVVector;

static const int tilesetW  = 8 * 32;
static const int autotileW = 3 * 32;
static const int autotileH = 4 * 32;
static const int atlasFrameW = tilesetW + autotileW;

/* Vocabulary:
 *
 * Atlas: A texture containing both the tileset and all
 *   autotile images. This is so the entire tilemap can
 *   be drawn from one texture (for performance reasons).
 *   This means that we have to watch the 'modified' signals
 *   of all Bitmaps that make up the atlas, and update it
 *   as required during runtime.
 *   The atlas is made up of one or four 'atlas frames'.
 *   They look like this: ('AT' = Autotile)
 *
 *         Atlas frame
 *   ----------------*--------
 *   |               |       |
 *   |               |  AT1  |
 *   |               |       |
 *   |               |       |
 *   |               |-------|
 *   |    Tileset    |       |
 *   |               |  AT2  |
 *   |               |       |
 *   |               |       |
 *   |               |-------|
 *   |               |       |
 *              ...
 *
 *   If none of the attached autotiles is animated, the
 *   atlas consists of only one such frame. In case of
 *   animated autotiles, it consists of four such frames,
 *   where each autotile area contains the respective
 *   animation frame of the autotile. Static autotiles
 *   and the tileset area are identical for all frames.
 *   This allows us to keep vertex data static and only
 *   change the x-offset in the atlas texture during animation.
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
 * Replica:
 *   A tilemap is not drawn as one rectangular object, but
 *   "tiled" if there is an area on the screen that would
 *   otherwise not be covered by it. RGSS does this so when the
 *   game orders a screen shake which draws the tilemap at slightly
 *   different x-offsets, black area isn't exposed (this would
 *   always happen for 20x15 maps). 'Replicas' describes where the
 *   tilemap needs to be drawn again to achieve this tiled effect
 *   (above the tilemap, to the right, above and right etc.).
 *   'Normal' means no replica needs to be drawn.
 *   Because the minimum map size in RMXP covers the entire screen,
 *   we don't have to worry about ever drawing more than one replica
 *   in each dimension.
 *
 */

/* Replica positions */
enum Position
{
	Normal = 1 << 0,

	Left   = 1 << 1,
	Right  = 1 << 2,
	Top    = 1 << 3,
	Bottom = 1 << 4,

	TopLeft     = Top | Left,
	TopRight    = Top | Right,
	BottomLeft  = Bottom | Left,
	BottomRight = Bottom | Right
};

static const Position positions[] =
{
	Normal,
	Left, Right, Top, Bottom,
	TopLeft, TopRight, BottomLeft, BottomRight
};

static elementsN(positions);

struct GroundLayer : public ViewportElement
{
	GLsizei vboCount;
	TilemapPrivate *p;

	GroundLayer(TilemapPrivate *p, Viewport *viewport);

	void draw();
	void drawInt();
	void drawFlashInt();

	void onGeometryChange(const Scene::Geometry &geo);
};

struct ScanRow : public ViewportElement
{
	const int index;
	GLintptr vboOffset;
	GLsizei vboCount;
	TilemapPrivate *p;

	ScanRow(TilemapPrivate *p, Viewport *viewport, int index);

	void draw();
	void drawInt();

	void updateZ();
};

struct TilemapPrivate
{
	Viewport *viewport;

	Tilemap::Autotiles autotilesProxy;
	Bitmap *autotiles[7];

	Bitmap *tileset;
	Table *mapData;
	Table *flashData;
	Table *priorities;
	bool visible;
	Vec2i offset;

	Vec2i dispPos;

	/* For updating scanrow z */
	int tileYOffset;

	/* Tile atlas */
	struct {
		TEXFBO gl;
		bool animated;
		int frameH;
		int width;
		/* Indices of animated autotiles */
		QVector<uint8_t> animatedATs;
		/* Animation state */
		uint8_t frameIdx;
		uint8_t aniIdx;
	} atlas;

	/* Map size in tiles */
	int mapWidth;
	int mapHeight;

	/* Ground layer vertices */
	SVVector groundVert;

	/* Scanrow vertices */
	QVector<SVVector> scanrowVert;

	/* Base quad indices of each scanrow
	 * in the shared buffer */
	QVector<int> scanrowBases;
	int scanrowCount;

	/* Shared buffers for all tiles */
	struct
	{
		VAO::ID vao;
		VBO::ID vbo;
	} tiles;

	/* Flash buffers */
	struct
	{
		VAO::ID vao;
		VBO::ID vbo;
		int quadCount;
	} flash;

	/* Scene elements */
	struct
	{
		GroundLayer *ground;
		QVector<ScanRow*> scanrows;
		Scene::Geometry sceneGeo;
		Vec2i sceneOffset;
	} elem;

	/* Replica bitmask */
	uint8_t replicas;

	/* Affected by: autotiles, tileset */
	bool atlasSizeDirty;
	/* Affected by: autotiles(.changed), tileset(.changed), allocateAtlas */
	bool atlasDirty;
	/* Affected by: mapData(.changed), priorities(.changed) */
	bool buffersDirty;
	/* Affected by: oy */
	bool zOrderDirty;
	/* Affected by: flashData, buffersDirty */
	bool flashDirty;

	/* Resources are sufficient and tilemap is ready to be drawn */
	bool tilemapReady;

	/* Change watches */
	sigc::connection tilesetCon;
	sigc::connection autotilesCon[7];
	sigc::connection mapDataCon;
	sigc::connection prioritiesCon;
	sigc::connection flashDataCon;

	/* Dispose watches */
	sigc::connection autotilesDispCon[7];

	/* Draw prepare call */
	sigc::connection prepareCon;

	TilemapPrivate(Viewport *viewport)
	    : viewport(viewport),
	      tileset(0),
	      mapData(0),
	      flashData(0),
	      priorities(0),
	      visible(true),
	      tileYOffset(0),
	      replicas(Normal),
	      atlasSizeDirty(false),
	      atlasDirty(false),
	      buffersDirty(false),
	      zOrderDirty(false),
	      flashDirty(false),
	      tilemapReady(false)
	{
		memset(autotiles, 0, sizeof(autotiles));

		atlas.animatedATs.reserve(7);
		atlas.frameIdx = 0;
		atlas.aniIdx = 0;

		/* Init tile buffers */
		tiles.vbo = VBO::gen();

		tiles.vao = VAO::gen();
		VAO::bind(tiles.vao);
		glEnableClientState(GL_VERTEX_ARRAY);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);

		VBO::bind(tiles.vbo);
		gState->bindQuadIBO();

		glVertexPointer  (2, GL_FLOAT, sizeof(SVertex), SVertex::posOffset());
		glTexCoordPointer(2, GL_FLOAT, sizeof(SVertex), SVertex::texPosOffset());

		VAO::unbind();
		VBO::unbind();
		IBO::unbind();

		/* Init flash buffers */
		flash.vbo = VBO::gen();
		flash.vao = VAO::gen();
		flash.quadCount = 0;

		VAO::bind(flash.vao);

		glEnableClientState(GL_VERTEX_ARRAY);
		glEnableClientState(GL_COLOR_ARRAY);

		VBO::bind(flash.vbo);
		gState->bindQuadIBO();

		glVertexPointer(2, GL_FLOAT, sizeof(CVertex), CVertex::posOffset());
		glColorPointer(4, GL_FLOAT, sizeof(CVertex), CVertex::colorOffset());

		VAO::unbind();
		VBO::unbind();
		IBO::unbind();

		elem.ground = 0;

		prepareCon = gState->prepareDraw.connect
		        (sigc::mem_fun(this, &TilemapPrivate::prepare));
	}

	~TilemapPrivate()
	{
		destroyElements();

		gState->texPool().release(atlas.gl);
		VAO::del(tiles.vao);
		VBO::del(tiles.vbo);

		VAO::del(flash.vao);
		VBO::del(flash.vbo);

		tilesetCon.disconnect();
		for (int i = 0; i < 7; ++i)
		{
			autotilesCon[i].disconnect();
			autotilesDispCon[i].disconnect();
		}
		mapDataCon.disconnect();
		prioritiesCon.disconnect();
		flashDataCon.disconnect();

		prepareCon.disconnect();
	}

	void updateSceneGeometry(const Scene::Geometry &geo)
	{
		elem.sceneOffset.x = geo.rect.x - geo.xOrigin;
		elem.sceneOffset.y = geo.rect.y - geo.yOrigin;
		elem.sceneGeo = geo;
	}

	void updatePosition()
	{
		dispPos.x = -offset.x + elem.sceneOffset.x;
		dispPos.y = -offset.y + elem.sceneOffset.y;

		if (mapWidth == 0 || mapHeight == 0)
			return;

		dispPos.x %= mapWidth  * 32;
		dispPos.y %= mapHeight * 32;
	}

	/* Compute necessary replicas and store this
	 * information in a bitfield */
	void updateReplicas()
	{
		replicas = Normal;
		const IntRect &sRect = elem.sceneGeo.rect;

		if (dispPos.x > sRect.x)
			replicas |= Left;
		if (dispPos.y > sRect.y)
			replicas |= Top;
		if (dispPos.x+mapWidth*32 < sRect.x+sRect.w)
			replicas |= Right;
		if (dispPos.y+mapHeight*32 < sRect.y+sRect.h)
			replicas |= Bottom;
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
		/* Check if and which autotiles are animated */
		QVector<uint8_t> &animatedATs = atlas.animatedATs;

		for (int i = 0; i < 7; ++i)
		{
			Bitmap *autotile = autotiles[i];

			if (!autotile)
				continue;

			if (autotile->isDisposed())
				continue;

			autotile->flush();

			if (autotile->width() > autotileW)
				animatedATs.append(i);
		}

		atlas.animated = !animatedATs.empty();

		atlas.frameH = max(tileset->height(), autotileH*7);

		/* Aquire atlas tex */
		atlas.width = animatedATs.empty() ? atlasFrameW : atlasFrameW * 4;

		gState->texPool().release(atlas.gl);
		atlas.gl = gState->texPool().request(atlas.width, atlas.frameH);

		atlasDirty = true;
	}

	/* Assembles atlas from tileset and autotile bitmaps */
	void buildAtlas()
	{
		tileset->flush();

		QVector<uint8_t> &animatedATs = atlas.animatedATs;

		/* Clear atlas */
		FBO::bind(atlas.gl.fbo, FBO::Draw);
		glState.clearColor.pushSet(Vec4());
		glState.scissorTest.pushSet(false);

		glClear(GL_COLOR_BUFFER_BIT);

		glState.scissorTest.pop();
		glState.clearColor.pop();

		int tsW = tilesetW;
		int tsH = tileset->height();

		/* Assemble first frame with static content */
		FBO::bind(tileset->getGLTypes().fbo, FBO::Read);
		FBO::blit(0, 0, 0, 0, tsW, tsH);

		for (int i = 0; i < 7; ++i)
		{
			if (!autotiles[i])
				continue;

			if (autotiles[i]->isDisposed())
				continue;

			if (animatedATs.contains(i))
				continue;

			FBO::bind(autotiles[i]->getGLTypes().fbo, FBO::Read);
			FBO::blit(0, 0, tsW, i*autotileH, autotileW, autotileH);
		}

		/* If there aren't any animated autotiles, we're done */
		if (!atlas.animated)
			return;

		/* Copy the first frame to the remaining 3 */
		FBO::bind(atlas.gl.fbo, FBO::Read);

		for (int i = 1; i < 4; ++i)
			FBO::blit(0, 0, i*atlasFrameW, 0, atlasFrameW, atlas.frameH);

		/* Finally, patch in the animated autotiles */
		for (int i = 0; i < animatedATs.count(); ++i)
		{
			int atInd = animatedATs[i];
			Bitmap *at = autotiles[atInd];

			if (at->isDisposed())
				continue;

			FBO::bind(at->getGLTypes().fbo, FBO::Read);

			for (int j = 0; j < 4; ++j)
				FBO::blit(j*autotileW, 0, (j*atlasFrameW)+tsW, atInd*autotileH,
				          autotileW, autotileH);
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

	void handleAutotile(int x, int y, int tileInd, SVVector &array)
	{
		/* Which autotile [0-7] */
		int atInd = tileInd / 48 - 1;
		/* Which tile pattern of the autotile [0-47] */
		int subInd = tileInd % 48;

		const StaticRect *pieceRect = &autotileRects[subInd*4];

		for (int i = 0; i < 4; ++i)
		{
			FloatRect posRect = getAutotilePieceRect(x*32, y*32, i);
			FloatRect texRect = pieceRect[i];

			/* Adjust to atlas coordinates */
			texRect.x += tilesetW;
			texRect.y += atInd * autotileH;

			SVertex v[4];
			Quad::setTexPosRect(v, texRect, posRect);

			for (int i = 0; i < 4; ++i)
				array.append(v[i]);
		}
	}

	void handleTile(int x, int y, int z)
	{
		int tileInd = mapData->at(x, y, z);

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
			handleAutotile(x, y, tileInd, *targetArray);
			return;
		}

		int tsInd = tileInd - 48*8;
		int tileX = tsInd % 8;
		int tileY = tsInd / 8;

		FloatRect texRect(tileX*32+.5, tileY*32+.5, 31, 31);
		FloatRect posRect(x*32, y*32, 32, 32);

		SVertex v[4];
		Quad::setTexPosRect(v, texRect, posRect);

		for (int i = 0; i < 4; ++i)
			targetArray->append(v[i]);
	}

	void clearQuadArrays()
	{
		groundVert.clear();
		scanrowVert.clear();
		scanrowBases.clear();
	}

	void buildQuadArray()
	{
		clearQuadArrays();

		mapWidth = mapData->xSize();
		mapHeight = mapData->ySize();
		int mapDepth = mapData->zSize();

		scanrowVert.resize(mapHeight + 5);

		for (int x = 0; x < mapWidth; ++x)
			for (int y = 0; y < mapHeight; ++y)
				for (int z = 0; z < mapDepth; ++z)
					handleTile(x, y, z);
	}

	static int quadDataSize(int quadCount)
	{
		return quadCount * sizeof(SVertex) * 4;
	}

	int scanrowSize(int index)
	{
		return scanrowBases[index+1] - scanrowBases[index];
	}

	void uploadBuffers()
	{
		scanrowCount = scanrowVert.count();
		scanrowBases.resize(scanrowCount + 1);

		/* Calculate total quad count */
		int groundQuadCount = groundVert.count() / 4;
		int quadCount = groundQuadCount;

		for (int i = 0; i < scanrowCount; ++i)
		{
			scanrowBases[i] = quadCount;
			quadCount += scanrowVert[i].count() / 4;
		}

		scanrowBases[scanrowCount] = quadCount;

		VBO::bind(tiles.vbo);
		VBO::allocEmpty(quadDataSize(quadCount));

		VBO::uploadSubData(0, quadDataSize(groundQuadCount), groundVert.constData());

		for (int i = 0; i < scanrowCount; ++i)
		{
			VBO::uploadSubData(quadDataSize(scanrowBases[i]),
			                   quadDataSize(scanrowSize(i)),
			                   scanrowVert[i].constData());
		}

		VBO::unbind();

		/* Ensure global IBO size */
		gState->ensureQuadIBO(quadCount);
	}

	void bindAtlasWithMatrix()
	{
		TEX::bind(atlas.gl.tex);
		TEX::bindMatrix(atlas.animated ? atlasFrameW * 4 : atlasFrameW, atlas.frameH,
		                atlas.frameIdx * atlasFrameW);
	}

	Vec2i getReplicaOffset(Position pos)
	{
		Vec2i offset;

		if (pos & Left)
			offset.x -= mapWidth*32;
		if (pos & Right)
			offset.x += mapWidth*32;
		if (pos & Top)
			offset.y -= mapHeight*32;
		if (pos & Bottom)
			offset.y += mapHeight*32;

		return offset;
	}

	void translateMatrix(Position replicaPos)
	{
		Vec2i repOff = getReplicaOffset(replicaPos);

		glTranslatef(dispPos.x + repOff.x,
		             dispPos.y + repOff.y, 0);
	}

	bool sampleFlashColor(Vec4 &out, int x, int y)
	{
		const int _x = x % flashData->xSize();
		const int _y = y % flashData->ySize();

		int16_t packed = flashData->at(_x, _y);

		if (packed == 0)
			return false;

		const float max = 60.0 / 255.0;
		const float step = max / 0xF;

		float b = ((packed & 0x000F) >> 0) * step;
		float g = ((packed & 0x00F0) >> 4) * step;
		float r = ((packed & 0x0F00) >> 8) * step;

		out = Vec4(r, g, b, 1);

		return true;
	}

	void updateFlash()
	{
		QVector<CVertex> vertices;

		for (int x = 0; x < mapWidth; ++x)
			for (int y = 0; y < mapHeight; ++y)
			{
				Vec4 color;
				if (!sampleFlashColor(color, x, y))
					continue;

				FloatRect posRect(x*32, y*32, 32, 32);

				CVertex v[4];
				Quad::setPosRect(v, posRect);
				Quad::setColor(v, color);

				for (int i = 0; i < 4; ++i)
					vertices.append(v[i]);
			}

		flash.quadCount = vertices.count() / 4;

		if (flash.quadCount == 0)
			return;

		VBO::bind(flash.vbo);
		VBO::uploadData(sizeof(CVertex) * vertices.count(), vertices.constData());
		VBO::unbind();

		/* Ensure global IBO size */
		gState->ensureQuadIBO(flash.quadCount);
	}

	void destroyElements()
	{
		delete elem.ground;
		elem.ground = 0;

		for (int i = 0; i < elem.scanrows.count(); ++i)
			delete elem.scanrows[i];

		elem.scanrows.clear();
	}

	void generateElements(QVector<int> &scanrowInd)
	{
		elem.ground = new GroundLayer(this, viewport);

		for (int i = 0; i < scanrowInd.count(); ++i)
		{
			int index = scanrowInd[i];
			elem.scanrows.append(new ScanRow(this, viewport, index));
		}
	}

	void generateSceneElements()
	{
		destroyElements();

		/* Only generate elements for non-emtpy scanrows */
		QVector<int> scanrowInd;
		for (int i = 0; i < scanrowCount; ++i)
			if (scanrowVert[i].count() > 0)
				scanrowInd.append(i);

		generateElements(scanrowInd);
	}

	void updateZOrder()
	{
		for (int i = 0; i < elem.scanrows.count(); ++i)
			elem.scanrows[i]->updateZ();
	}

	void prepare()
	{
		if (!verifyResources())
		{
			if (elem.ground)
				destroyElements();
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

		if (buffersDirty)
		{
			buildQuadArray();
			uploadBuffers();
			generateSceneElements();
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

		tilemapReady = true;
	}
};

GroundLayer::GroundLayer(TilemapPrivate *p, Viewport *viewport)
    : ViewportElement(viewport),
      p(p)
{
	vboCount = p->scanrowBases[0] * 6;

	onGeometryChange(scene->getGeometry());
}

void GroundLayer::draw()
{
	p->bindAtlasWithMatrix();

	VAO::bind(p->tiles.vao);

	glPushMatrix();

	for (int i = 0; i < positionsN; ++i)
	{
		const Position pos = positions[i];

		if (!(p->replicas & pos))
			continue;

		glLoadIdentity();
		p->translateMatrix(pos);

		drawInt();
	}

	if (p->flash.quadCount > 0)
	{
		VAO::bind(p->flash.vao);
		glState.blendMode.pushSet(BlendAddition);
		glState.texture2D.pushSet(false);

		for (int i = 0; i < positionsN; ++i)
		{
			const Position pos = positions[i];

			if (!(p->replicas & pos))
				continue;

			glLoadIdentity();
			p->translateMatrix(pos);

			drawFlashInt();
		}

		glState.texture2D.pop();
		glState.blendMode.pop();
	}

	glPopMatrix();

	VAO::unbind();
}

void GroundLayer::drawInt()
{
	glDrawElements(GL_TRIANGLES, vboCount,
	               GL_UNSIGNED_INT, 0);
}

void GroundLayer::drawFlashInt()
{
	glDrawElements(GL_TRIANGLES, p->flash.quadCount * 6, GL_UNSIGNED_INT, 0);
}

void GroundLayer::onGeometryChange(const Scene::Geometry &geo)
{
	p->updateSceneGeometry(geo);
	p->updatePosition();
	p->updateReplicas();
}

ScanRow::ScanRow(TilemapPrivate *p, Viewport *viewport, int index)
    : ViewportElement(viewport, 32 + index*32),
      index(index),
      p(p)
{
	vboOffset = p->scanrowBases[index] * sizeof(uint) * 6;
	vboCount = p->scanrowSize(index) * 6;
}

void ScanRow::draw()
{
	p->bindAtlasWithMatrix();

	VAO::bind(p->tiles.vao);

	glPushMatrix();
	glLoadIdentity();
	p->translateMatrix(Normal);

	for (int i = 0; i < positionsN; ++i)
	{
		const Position pos = positions[i];

		if (!(p->replicas & pos))
			continue;

		glLoadIdentity();
		p->translateMatrix(pos);

		drawInt();
	}

	glPopMatrix();

	VAO::unbind();
}

void ScanRow::drawInt()
{
	glDrawElements(GL_TRIANGLES, vboCount,
	               GL_UNSIGNED_INT, (const GLvoid*) vboOffset);
}

void ScanRow::updateZ()
{
	int y = (p->offset.y > 0) ? -p->offset.y : 0;
	setZ(32 + (((y) / 32) + index) * 32);
}

void Tilemap::Autotiles::set(int i, Bitmap *bitmap)
{
	if (i < 0 || i > 6)
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
}

Bitmap *Tilemap::Autotiles::get(int i) const
{
	if (i < 0 || i > 6)
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

static const uchar atAnimation[16*4] =
{
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3
};

void Tilemap::update()
{
	if (!p->atlas.animated)
		return;

	p->atlas.frameIdx = atAnimation[p->atlas.aniIdx];

	if (++p->atlas.aniIdx >= 16*4)
		p->atlas.aniIdx = 0;
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

void Tilemap::setViewport(Viewport *value)
{
	GUARD_DISPOSED

	if (p->viewport == value)
		return;

	p->viewport = value;

	if (!p->tilemapReady)
		return;

	p->elem.ground->setViewport(value);

	for (int i = 0; i < p->elem.scanrows.count(); ++i)
		p->elem.scanrows[i]->setViewport(value);
}

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
	for (int i = 0; i < p->elem.scanrows.count(); ++i)
		p->elem.scanrows[i]->setVisible(value);
}

void Tilemap::setOX(int value)
{
	GUARD_DISPOSED

	if (p->offset.x == value)
		return;

	p->offset.x = value;
	p->updatePosition();
	p->updateReplicas();
}

void Tilemap::setOY(int value)
{
	GUARD_DISPOSED

	if (p->offset.y == value)
		return;

	p->offset.y = value;
	p->updatePosition();
	p->updateReplicas();

	int tileYOffset = value / 32;
	if (tileYOffset != p->tileYOffset)
	{
		p->tileYOffset = tileYOffset;
		p->zOrderDirty = true;
	}
}


void Tilemap::releaseResources()
{
	delete p;
}
