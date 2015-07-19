/*
** tileatlasvx.cpp
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

#include "tileatlasvx.h"

#include "tilemap-common.h"
#include "bitmap.h"
#include "table.h"
#include "etc-internal.h"
#include "gl-util.h"
#include "gl-meta.h"
#include "sharedstate.h"
#include "glstate.h"
#include "texpool.h"
#include "util.h"

#include <assert.h>
#include <vector>

/* Regular (A) autotile patterns */
extern const StaticRect autotileVXRectsA[];
extern const int autotileVXRectsAN;

/* Table (A2) autotile patterns */
extern const StaticRect autotileVXRectsA2[];
extern const int autotileVXRectsA2N;
extern const float autotileVXRectsA2Sizes[];

/* Wall (B) autotile patterns */
extern const StaticRect autotileVXRectsB[];
extern const int autotileVXRectsBN;

/* Waterfall (C) autotile patterns */
static const StaticRect autotileVXRectsC[] =
{
	{ 32.5f, 0.5f, 15.0f, 31.0f },
	{ 16.5f, 0.5f, 15.0f, 31.0f },
	{  0.0f, 0.5f, 15.0f, 31.0f },
	{ 16.5f, 0.5f, 15.0f, 31.0f },
	{ 32.5f, 0.5f, 15.0f, 31.0f },
	{ 48.5f, 0.5f, 15.0f, 31.0f },
	{  0.0f, 0.5f, 15.0f, 31.0f },
	{ 48.5f, 0.5f, 15.0f, 31.0f }
};

static elementsN(autotileVXRectsC);

namespace TileAtlasVX
{

static int16_t
tableGetSafe(const Table *t, int x)
{
	if (!t)
		return 0;

	if (x < 0 || x >= t->xSize())
		return 0;

	return t->at(x);
}

/* All below constants are in tiles (= 32 pixels) */

struct Size
{
	int w, h;

	Size(int w, int h)
	    : w(w), h(h)
	{}
};

static const Size bmSizes[BM_COUNT] =
{
	Size(16, 12), /* A1 */
	Size(16, 12), /* A2 */
	Size(16,  8), /* A3 */
	Size(16, 15), /* A4 */
	Size( 8, 16), /* A5 */
	Size(16, 16), /* B */
	Size(16, 16), /* C */
	Size(16, 16), /* D */
	Size(16, 16), /* E */
};

static const Size atArea(16, 13);

static const Vec2i freeArea(8, 48);

static const Vec2i CDEArea(16, 0);

struct Blit
{
	IntRect src;
	Vec2i dst;

	Blit(int srcX, int srcY, int w, int h, int dstX, int dstY)
	    : src(srcX, srcY, w, h),
	      dst(dstX, dstY)
	{}

	Blit(int srcX, int srcY, const Size &size, int dstX, int dstY)
	    : src(srcX, srcY, size.w, size.h),
	      dst(dstX, dstY)
	{}
};

static const Blit blitsA1[] =
{
	/* Animated A autotiles */
	Blit(0, 0, 6, 12, 0, 0),
	Blit(8, 0, 6, 12, 6, 0),
	/* Unanimated A autotiles */
	Blit(6, 0, 2, 6, freeArea.x, freeArea.y),
	/* C autotiles */
	Blit(14, 0, 2, 6, 12, 0),
	Blit(6, 6, 2, 6, 14, 0),
	Blit(14, 6, 2, 6, 12, 6)
};

static const Blit blitsA2[] =
{
	Blit(0, 0, bmSizes[BM_A2], 0, atArea.h)
};

static const Blit blitsA3[] =
{
	Blit(0, 0, bmSizes[BM_A3], 0, blitsA2[0].dst.y+blitsA2[0].src.h)
};

static const Blit blitsA4[] =
{
	Blit(0, 0, bmSizes[BM_A4], 0, blitsA3[0].dst.y+blitsA3[0].src.h)
};

static const Blit blitsA5[] =
{
	Blit(0, 0, bmSizes[BM_A5], 0, blitsA4[0].dst.y+blitsA4[0].src.h)
};

static const Blit blitsB[] =
{
	Blit(0, 0, bmSizes[BM_B], atArea.w, 0)
};

static const Blit blitsC[] =
{
	Blit(0, 0, bmSizes[BM_C], blitsA2[0].src.w, blitsB[0].dst.y+blitsB[0].src.h)
};

static const Blit blitsD[] =
{
	Blit(0, 0, bmSizes[BM_D], blitsC[0].dst.x, blitsC[0].dst.y+blitsC[0].src.h)
};

static const Blit blitsE[] =
{
	Blit(0, 0, bmSizes[BM_E], blitsD[0].dst.x, blitsD[0].dst.y+blitsD[0].src.h)
};

static elementsN(blitsA1);
static elementsN(blitsA2);
static elementsN(blitsA3);
static elementsN(blitsA4);
static elementsN(blitsA5);
static elementsN(blitsB);
static elementsN(blitsC);
static elementsN(blitsD);
static elementsN(blitsE);

/* 'Waterfall' autotiles atlas origin */
static const Vec2i AEPartsDst[] =
{
	Vec2i(12, 0),
	Vec2i(12, 3),
	Vec2i(14, 0),
	Vec2i(14, 3),
	Vec2i(12, 6),
	Vec2i(12, 9)
};

static const Vec2i shadowArea(freeArea.x+2, freeArea.y);

static SDL_Surface*
createShadowSet()
{
	int bpp;
	Uint32 rm, gm, bm, am;

	SDL_PixelFormatEnumToMasks(SDL_PIXELFORMAT_ABGR8888, &bpp, &rm, &gm, &bm, &am);
	SDL_Surface *surf = SDL_CreateRGBSurface(0, 1*32, 16*32, bpp, rm, gm, bm, am);

	std::vector<SDL_Rect> rects;
	SDL_Rect rect = { 0, 0, 16, 16 };

	for (int val = 0; val < 16; ++val)
	{
		int origY = val*32;

		/* Top left */
		if (val & (1 << 0))
		{
			rect.x = 0;
			rect.y = origY;
			rects.push_back(rect);
		}

		/* Top Right */
		if (val & (1 << 1))
		{
			rect.x = 16;
			rect.y = origY;
			rects.push_back(rect);
		}

		/* Bottom left */
		if (val & (1 << 2))
		{
			rect.x = 0;
			rect.y = origY+16;
			rects.push_back(rect);
		}

		/* Bottom right */
		if (val & (1 << 3))
		{
			rect.x = 16;
			rect.y = origY+16;
			rects.push_back(rect);
		}
	}

	/* Fill rects with half opacity black */
	uint32_t color = (0x80808080 & am);
	SDL_FillRects(surf, dataPtr(rects), rects.size(), color);

	return surf;
}

static void doBlit(Bitmap *bm, const IntRect &src, const Vec2i &dst)
{
	/* Translate tile to pixel units */
	IntRect _src(src.x*32, src.y*32, src.w*32, src.h*32);
	Vec2i _dst(dst.x*32, dst.y*32);
	IntRect bmr(0, 0, bm->width(), bm->height());

	if (!SDL_IntersectRect(&_src, &bmr, &_src))
		return;

	GLMeta::blitRectangle(_src, _dst);
}

void build(TEXFBO &tf, Bitmap *bitmaps[BM_COUNT])
{
	assert(tf.width == ATLASVX_W && tf.height == ATLASVX_H);

	GLMeta::blitBegin(tf);

	glState.clearColor.pushSet(Vec4());
	FBO::clear();
	glState.clearColor.pop();

	if (rgssVer >= 3)
	{
		SDL_Surface *shadow = createShadowSet();
		TEX::bind(tf.tex);
		TEX::uploadSubImage(shadowArea.x*32, shadowArea.y*32,
							shadow->w, shadow->h, shadow->pixels, GL_RGBA);
		SDL_FreeSurface(shadow);
	}

	Bitmap *bm;
#define EXEC_BLITS(part) \
	if (!nullOrDisposed(bm = bitmaps[BM_##part])) \
	{ \
		GLMeta::blitSource(bm->getGLTypes()); \
		for (size_t i = 0; i < blits##part##N; ++i) \
		{\
			const IntRect &src = blits##part[i].src; \
			const Vec2i &dst = blits##part[i].dst; \
			doBlit(bm, src, dst); \
		} \
	}

	EXEC_BLITS(A1);
	EXEC_BLITS(A2);
	EXEC_BLITS(A3);
	EXEC_BLITS(A4);
	EXEC_BLITS(A5);
	EXEC_BLITS(B);
	EXEC_BLITS(C);
	EXEC_BLITS(D);
	EXEC_BLITS(E);

#undef EXEC_BLITS

	GLMeta::blitEnd();
}

#define OVER_PLAYER_FLAG (1 << 4)
#define TABLE_FLAG       (1 << 7)

/* Reference: http://www.tktkgame.com/tkool/memo/vx/tile_id.html */

static void
readAutotile(Reader &reader, int patternID,
             const Vec2i &orig, int x, int y,
             const StaticRect rectSource[], int rectSourceN)
{
	FloatRect tex[4], pos[4];
	(void) rectSourceN;

	for (int i = 0; i < 4; ++i)
	{
		assert((patternID*4 + i) < rectSourceN);

		tex[i] = FloatRect(rectSource[patternID*4 + i]);
		tex[i].x += orig.x*32;
		tex[i].y += orig.y*32;

		pos[i] = FloatRect(x*32, y*32, 16, 16);
		atSelectSubPos(pos[i], i);
	}

	reader.onQuads(tex, pos, 4, false);
}

static void
readAutotileA(Reader &reader, int patternID,
              const Vec2i &orig, int x, int y)
{
	readAutotile(reader, patternID, orig, x, y,
	             autotileVXRectsA, autotileVXRectsAN);
}

static void
readAutotileA2(Reader &reader, int patternID,
               const Vec2i &orig, int x, int y)
{
	FloatRect tex[6], pos[6];

	for (int i = 0; i < 6; ++i)
	{
		assert((patternID*6 + i) < autotileVXRectsA2N);

		tex[i] = FloatRect(autotileVXRectsA2[patternID*6 + i]);
		tex[i].x += orig.x*32;
		tex[i].y += orig.y*32;

		float size = autotileVXRectsA2Sizes[patternID*6 + i];

		pos[i] = FloatRect(x*32, y*32, size, size);
		atSelectSubPos(pos[i], i);
	}

	reader.onQuads(tex, pos, 6, false);
}

static void
readAutotileB(Reader &reader, int patternID,
              const Vec2i &orig, int x, int y)
{
	if (patternID >= 0x10)
		return;

	readAutotile(reader, patternID, orig, x, y,
	             autotileVXRectsB, autotileVXRectsBN);
}

static void
readAutotileC(Reader &reader, int patternID,
              const Vec2i &orig, int x, int y)
{
	if (patternID > 0x3)
		return;

	FloatRect tex[2], pos[2];

	for (size_t i = 0; i < 2; ++i)
	{
		tex[i] = autotileVXRectsC[patternID*2+i];
		tex[i].x += orig.x*32;
		tex[i].y += orig.y*32;

		pos[i] = FloatRect(x*32, y*32, 16, 32);
		pos[i].x += i*16;
	}

	reader.onQuads(tex, pos, 2, false);
}

static void
onTileA1(Reader &reader, int16_t tileID,
         const int x, const int y)
{
	tileID -= 0x0800;

	int patternID  = tileID % 0x30;
	int autotileID = tileID / 0x30;

	const Vec2i autotileC(-1, -1);
	const Vec2i atOrig[] =
	{
		Vec2i(0, 0),
		Vec2i(0, 3),
		Vec2i(freeArea),
		Vec2i(freeArea.x, freeArea.y+3),
		Vec2i(6, 0),
		autotileC,
		Vec2i(6, 3),
		autotileC,

		Vec2i(0, 6),
		autotileC,
		Vec2i(0, 9),
		autotileC,
		Vec2i(6, 6),
		autotileC,
		Vec2i(6, 9),
		autotileC
	};

	const Vec2i orig = atOrig[autotileID];

	if (orig.x == -1)
	{
		int cID = (autotileID - 5) / 2;
		const Vec2i orig = AEPartsDst[cID];

		readAutotileC(reader, patternID, orig, x, y);
		return;
	}

	readAutotileA(reader, patternID, orig, x, y);
}

static void
onTileA2(Reader &reader, int16_t tileID,
         int x, int y, bool isTable)
{
	Vec2i orig = blitsA2[0].dst;
	tileID -= 0x0B00;

	int patternID  = tileID % 0x30;
	int autotileID = tileID / 0x30;

	orig.x += (autotileID % 8) * 2;
	orig.y += (autotileID / 8) * 3;

	/* The table autotile handling isn't 100% accurate;
	 * for that, we'd need to prerender layer 1 into a separate
	 * temp texture with blending turned off and render that
	 * to the screen. But in 99% of cases it shouldn't matter */

	if (isTable)
		readAutotileA2(reader, patternID, orig, x, y);
	else
		readAutotileA(reader, patternID, orig, x, y);
}

static void
onTileA3(Reader &reader, int16_t tileID,
         int x, int y)
{
	Vec2i orig = blitsA3[0].dst;
	tileID -= 0x1100;

	int patternID  = tileID % 0x30;
	int autotileID = tileID / 0x30;

	orig.x += (autotileID % 8) * 2;
	orig.y += (autotileID / 8) * 2;

	readAutotileB(reader, patternID, orig, x, y);
}

static void
onTileA4(Reader &reader, int16_t tileID,
         int x, int y)
{
	Vec2i orig = blitsA4[0].dst;
	tileID -= 0x1700;

	static const int offY[] = { 0, 3, 5, 8, 10, 13 };

	int patternID  = tileID % 0x30;
	int autotileID = tileID / 0x30;

	int offYI = autotileID / 8;
	orig.x += (autotileID % 8) * 2;
	orig.y += offY[offYI];

	if ((offYI % 2) == 0)
		readAutotileA(reader, patternID, orig, x, y);
	else
		readAutotileB(reader, patternID, orig, x, y);
}

static void
onTileA5(Reader &reader, int16_t tileID,
         int x, int y, bool overPlayer)
{
	const Vec2i orig = blitsA5[0].dst;
	tileID -= 0x0600;

	int ox = tileID % 0x8;
	int oy = tileID / 0x8;

	FloatRect tex((orig.x+ox)*32+0.5, (orig.y+oy)*32+0.5, 31, 31);
	FloatRect pos(x*32, y*32, 32, 32);

	reader.onQuads(&tex, &pos, 1, overPlayer);
}

static void
onTileBCDE(Reader &reader, int16_t tileID,
           int x, int y, bool overPlayer)
{
	int ox = tileID % 0x8;
	int oy = (tileID / 0x8) % 0x10;
	int ob = tileID / (0x8*0x10);

	ox += (ob % 2) * 0x8;
	oy += (ob / 2) * 0x10;

	FloatRect tex((CDEArea.x+ox)*32+0.5, (CDEArea.y+oy)*32+0.5, 31, 31);
	FloatRect pos(x*32, y*32, 32, 32);

	reader.onQuads(&tex, &pos, 1, overPlayer);
}

static void
onTile(Reader &reader, int16_t tileID,
       int x, int y, const Table *flags)
{
	int16_t flag = tableGetSafe(flags, tileID);
	bool overPlayer = flag & OVER_PLAYER_FLAG;
	bool isTable;

	if (rgssVer >= 3)
		isTable = flag & TABLE_FLAG;
	else
		isTable = (tileID - 0x0B00) % (8 * 0x30) >= (7 * 0x30);

	/* B ~ E */
	if (tileID < 0x0400)
	{
		onTileBCDE(reader, tileID, x, y, overPlayer);
		return;
	}

	/* A5 */
	if (tileID >= 0x0600 && tileID < 0x0680)
	{
		onTileA5(reader, tileID, x, y, overPlayer);
		return;
	}

	if (tileID >= 0x0800 && tileID < 0x0B00)
	{
		onTileA1(reader, tileID, x, y);
		return;
	}

	/* A2 */
	if (tileID >= 0x0B00 && tileID < 0x1100)
	{
		onTileA2(reader, tileID, x, y, isTable);
		return;
	}

	/* A3 */
	if (tileID < 0x1700)
	{
		onTileA3(reader, tileID, x, y);
		return;
	}

	/* A4 */
	if (tileID < 0x2000)
	{
		onTileA4(reader, tileID, x, y);
		return;
	}
}

static void
readLayer(Reader &reader, const Table &data,
          const Table *flags, int ox, int oy, int w, int h, int z)
{
	/* The table autotile pattern (A2) has two quads (table
	 * legs, etc.) which extend over the tile below. We process
	 * the tiles in rows from bottom to top so the table extents
	 * are added after the tile below and drawn over it. */

	for (int y = h-1; y >= 0; --y)
		for (int x = 0; x < w; ++x)
		{
			int16_t tileID = tableGetWrapped(data, x+ox, y+oy, z);

			if (tileID <= 0)
				continue;

			onTile(reader, tileID, x, y, flags);
		}
}

static void
onShadowTile(Reader &reader, int8_t value,
             int x, int y)
{
	if (value == 0)
		return;

	int oy = value;

	FloatRect tex((shadowArea.x)*32+0.5, (shadowArea.y+oy)*32+0.5, 31, 31);
	FloatRect pos(x*32, y*32, 32, 32);

	reader.onQuads(&tex, &pos, 1, false);
}

static void
readShadowLayer(Reader &reader, const Table &data,
                int ox, int oy, int w, int h)
{
	for (int y = 0; y < h; ++y)
		for (int x = 0; x < w; ++x)
		{
			int16_t value = tableGetWrapped(data, x+ox, y+oy, 3);
			onShadowTile(reader, value & 0xF, x, y);
		}
}

void readTiles(Reader &reader, const Table &data,
               const Table *flags, int ox, int oy, int w, int h)
{
	for (int i = 0; i < 2; ++i)
		readLayer(reader, data, flags, ox, oy, w, h, i);

	if (rgssVer >= 3)
		readShadowLayer(reader, data, ox, oy, w, h);

	readLayer(reader, data, flags, ox, oy, w, h, 2);
}

}
