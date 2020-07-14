/*
** tileatlas.cpp
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

#include "tileatlas.h"

namespace TileAtlas
{

/* A Column represents a Rect
 * with undefined width */
struct Column
{
	int x, y, h;

	Column(int x, int y, int h)
	    : x(x), y(y), h(h)
	{}
};

typedef std::vector<Column> ColumnVec;

/* Buffer between autotile area and tileset */
static const int atBuffer = 32;
/* Autotile area width */
static const int atAreaW = 32*3*8;
/* Autotile area height */
static const int atAreaH = 32*4*7 + atBuffer;

static const int tilesetW = 32*8;
static const int tsLaneW = tilesetW / 1;
static const int underAtLanes = atAreaW / tsLaneW + !!(atAreaW % tsLaneW);

/* Autotile area */
static const int atArea = underAtLanes * tsLaneW * atAreaH;

static int freeArea(int width, int height)
{
	return width * height - atArea;
}

Vec2i minSize(int tilesetH, int maxAtlasSize)
{
	int width = underAtLanes * tsLaneW;
	int height = atAreaH;

	const int tsArea = tilesetW * tilesetH;

	/* Expand vertically */
	while (freeArea(width, height) < tsArea && height < maxAtlasSize)
		height += 32;

	if (freeArea(width, height) >= tsArea && height <= maxAtlasSize)
		return Vec2i(width, height);

	/* Expand horizontally */
	while (freeArea(width, height) < tsArea && width < maxAtlasSize)
		width += tsLaneW;

	if (freeArea(width, height) >= tsArea && width <= maxAtlasSize)
		return Vec2i(width, height);

	return Vec2i(-1, -1);
}

static ColumnVec calcSrcCols(int tilesetH)
{
	ColumnVec cols;
	// cols.reserve(2);

	cols.push_back(Column(0, 0, tilesetH));
	// cols.push_back(Column(tsLaneW, 0, tilesetH));

	return cols;
}

static ColumnVec calcDstCols(int atlasW, int atlasH)
{
	ColumnVec cols;
	cols.reserve(underAtLanes);

	/* Columns below the autotile area */
	const int underAt = atlasH - atAreaH;

	for (int i = 0; i < underAtLanes; ++i)
		cols.push_back(Column(i*tsLaneW, atAreaH, underAt));

	const int remCols = atlasW / tsLaneW - underAtLanes;

	if (remCols > 0)
		for (int i = 0; i < remCols; ++i)
			cols.push_back(Column((underAtLanes+i)*tsLaneW, 0, atlasH));

	return cols;
}

static BlitVec calcBlitsInt(ColumnVec &srcCols, ColumnVec &dstCols)
{
	BlitVec blits;

	/* Using signed indices here is safer, as we
	 * might decrement dstI while it is zero. */
	int dstI = 0;

	for (size_t srcI = 0; srcI < srcCols.size(); ++srcI)
	{
		Column &srcCol = srcCols[srcI];

		for (; dstI < (int) dstCols.size() && srcCol.h > 0; ++dstI)
		{
			Column &dstCol = dstCols[dstI];

			if (srcCol.h > dstCol.h)
			{
				/* srcCol doesn't fully fit into dstCol */
				blits.push_back(Blit(srcCol.x, srcCol.y,
				                     dstCol.x, dstCol.y, dstCol.h));

				srcCol.y += dstCol.h;
				srcCol.h -= dstCol.h;
			}
			else if (srcCol.h < dstCol.h)
			{
				/* srcCol fits into dstCol with space remaining */
				blits.push_back(Blit(srcCol.x, srcCol.y,
				                     dstCol.x, dstCol.y, srcCol.h));

				dstCol.y += srcCol.h;
				dstCol.h -= srcCol.h;

				/* Queue this column up again for processing */
				--dstI;

				srcCol.h = 0;
			}
			else
			{
				/* srcCol fits perfectly into dstCol */
				blits.push_back(Blit(srcCol.x, srcCol.y,
				                     dstCol.x, dstCol.y, dstCol.h));
			}
		}
	}

	return blits;
}

BlitVec calcBlits(int tilesetH, const Vec2i &atlasSize)
{
	ColumnVec srcCols = calcSrcCols(tilesetH);
	ColumnVec dstCols = calcDstCols(atlasSize.x, atlasSize.y);

	return calcBlitsInt(srcCols, dstCols);
}

Vec2i tileToAtlasCoor(int tileX, int tileY, int tilesetH, int atlasH)
{
	int laneX = tileX*32;
	int laneY = tileY*32;

	int longlaneH = atlasH;
	int shortlaneH = longlaneH - atAreaH;

	int longlaneOffset = shortlaneH * underAtLanes;

	int laneIdx = 0;
	int atlasY = 0;

	/* Check if we're inside the 2nd lane */
	// if (laneX >= tsLaneW)
	// {
	// 	laneY += tilesetH;
	// 	laneX -= tsLaneW;
	// }

	if (laneY < longlaneOffset)
	{
		/* Below autotile area */
		laneIdx = laneY / shortlaneH;
		atlasY  = laneY % shortlaneH + atAreaH;
	}
	else
	{
		/* Right of autotile area */
		int _y = laneY - longlaneOffset;
		laneIdx = underAtLanes + _y / longlaneH;
		atlasY = _y % longlaneH;
	}

	int atlasX = laneIdx * tsLaneW + laneX;

	return Vec2i(atlasX, atlasY);
}

}
