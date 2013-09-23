#include "tileatlas.h"

namespace TileAtlas
{

/* A Row represents a Rect
 * with undefined width */
struct Row
{
	int x, y, h;

	Row(int x, int y, int h)
	    : x(x), y(y), h(h)
	{}
};

typedef QList<Row> RowList;

/* Autotile area width */
static const int atAreaW = 96*4;
/* Autotile area height */
static const int atAreaH = 128*7;
/* Autotile area */
static const int atArea = atAreaW * atAreaH;

static const int tilesetW = 256;
static const int tsLaneW = tilesetW / 2;

static int freeArea(int width, int height)
{
	return width * height - atArea;
}

Vec2i minSize(int tilesetH, int maxAtlasSize)
{
	int width = atAreaW;
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

static RowList calcSrcRows(int tilesetH)
{
	RowList rows;

	rows << Row(0, 0, tilesetH);
	rows << Row(tsLaneW, 0, tilesetH);

	return rows;
}

static RowList calcDstRows(int atlasW, int atlasH)
{
	RowList rows;

	/* Rows below the autotile area */
	const int underAt = atlasH - atAreaH;

	for (int i = 0; i < 3; ++i)
		rows << Row(i*tsLaneW, atAreaH, underAt);

	if (atlasW <= atAreaW)
		return rows;

	const int remRows = (atlasW - atAreaW) / tsLaneW;

	for (int i = 0; i < remRows; ++i)
		rows << Row(i*tsLaneW + atAreaW, 0, atlasH);

	return rows;
}

static BlitList calcBlitsInt(RowList &srcRows, RowList &dstRows)
{
	BlitList blits;

	while (!srcRows.empty())
	{
		Row srcRow = srcRows.takeFirst();
		Q_ASSERT(srcRow.h > 0);

		while (!dstRows.empty() && srcRow.h > 0)
		{
			Row dstRow = dstRows.takeFirst();

			if (srcRow.h > dstRow.h)
			{
				/* srcRow doesn't fully fit into dstRow */
				blits << Blit(srcRow.x, srcRow.y,
				              dstRow.x, dstRow.y, dstRow.h);

				srcRow.y += dstRow.h;
				srcRow.h -= dstRow.h;
			}
			else if (srcRow.h < dstRow.h)
			{
				/* srcRow fits into dstRow with space remaining */
				blits << Blit(srcRow.x, srcRow.y,
				              dstRow.x, dstRow.y, srcRow.h);

				dstRow.y += srcRow.h;
				dstRow.h -= srcRow.h;
				dstRows.prepend(dstRow);
				srcRow.h = 0;
			}
			else
			{
				/* srcRow fits perfectly into dstRow */
				blits << Blit(srcRow.x, srcRow.y,
				              dstRow.x, dstRow.y, dstRow.h);
			}
		}
	}

	return blits;
}

BlitList calcBlits(int tilesetH, const Vec2i &atlasSize)
{
	RowList srcRows = calcSrcRows(tilesetH);
	RowList dstRows = calcDstRows(atlasSize.x, atlasSize.y);

	return calcBlitsInt(srcRows, dstRows);
}

Vec2i tileToAtlasCoor(int tileX, int tileY, int tilesetH, int atlasH)
{
	int laneX = tileX*32;
	int laneY = tileY*32;

	int longlaneH = atlasH;
	int shortlaneH = longlaneH - atAreaH;

	int longlaneOffset = shortlaneH * 3;

	int laneIdx = 0;
	int atlasY = 0;

	/* Check if we're inside the 2nd lane */
	if (laneX >= tsLaneW)
	{
		laneY += tilesetH;
		laneX -= tsLaneW;
	}

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
		laneIdx = 3 + _y / longlaneH;
		atlasY = _y % longlaneH;
	}

	int atlasX = laneIdx * tsLaneW + laneX;

	return Vec2i(atlasX, atlasY);
}

}
