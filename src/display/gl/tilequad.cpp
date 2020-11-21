/*
** tilequad.cpp
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

#include "tilequad.h"

#include "gl-util.h"
#include "quad.h"

namespace TileQuads
{

int oneDimCount(int tileDimension,
                int destDimension)
{
	if (tileDimension <= 0)
		return 0;

	int fullCount = destDimension / tileDimension;
	int partSize  = destDimension % tileDimension;

	return fullCount + (partSize ? 1 : 0);
}

int twoDimCount(int tileW, int tileH,
                int destW, int destH)
{
	return oneDimCount(tileW, destW) *
	       oneDimCount(tileH, destH);
}

int buildH(const IntRect &sourceRect,
           int width, int x, int y,
           Vertex *verts)
{
	if (width <= 0)
		return 0;

	int fullCount = width / sourceRect.w;
	int partSize  = width % sourceRect.w;

	FloatRect _sourceRect(sourceRect);
	FloatRect destRect(x, y, sourceRect.w, sourceRect.h);

	/* Full size quads */
	for (int x = 0; x < fullCount; ++x)
	{
		Vertex *vert = &verts[x*4];

		Quad::setTexRect(vert, _sourceRect);
		Quad::setPosRect(vert, destRect);

		destRect.x += sourceRect.w;
	}

	if (partSize)
	{
		Vertex *vert = &verts[fullCount*4];

		_sourceRect.w = partSize;
		destRect.w = partSize;

		Quad::setTexRect(vert, _sourceRect);
		Quad::setPosRect(vert, destRect);
	}

	return fullCount + (partSize ? 1 : 0);
}

int buildV(const IntRect &sourceRect,
           int height, int ox, int oy,
           Vertex *verts)
{
	if (height <= 0)
		return 0;

	int fullCount = height / sourceRect.h;
	int partSize  = height % sourceRect.h;

	FloatRect _sourceRect(sourceRect);
	FloatRect destRect(ox, oy, sourceRect.w, sourceRect.h);

	/* Full size quads */
	for (int y = 0; y < fullCount; ++y)
	{
		Vertex *vert = &verts[y*4];

		Quad::setTexRect(vert, _sourceRect);
		Quad::setPosRect(vert, destRect);

		destRect.y += sourceRect.h;
	}

	if (partSize)
	{
		Vertex *vert = &verts[fullCount*4];

		_sourceRect.h = partSize;
		destRect.h = partSize;

		Quad::setTexRect(vert, _sourceRect);
		Quad::setPosRect(vert, destRect);
	}

	return fullCount + (partSize ? 1 : 0);
}

int build(const IntRect &sourceRect,
          const IntRect &destRect,
          Vertex *verts)
{
	int ox = destRect.x;
	int oy = destRect.y;
	int width = destRect.w;
	int height = destRect.h;

	if (width <= 0 || height <= 0)
		return 0;

	int fullCount = height / sourceRect.h;
	int partSize  = height % sourceRect.h;

	int rowTileCount = oneDimCount(sourceRect.w, width);

	int qCount = 0;

	int v = 0;
	for (int i = 0; i < fullCount; ++i)
	{
		qCount += buildH(sourceRect, width, ox, oy, &verts[v]);

		v += rowTileCount*4;
		oy += sourceRect.h;
	}

	if (partSize)
	{
		IntRect partSourceRect = sourceRect;
		partSourceRect.h = partSize;

		qCount += buildH(partSourceRect, width, ox, oy, &verts[v]);
	}

	return qCount;
}

static void buildFrameInt(const IntRect &rect,
                          FloatRect quadRects[9])
{
	int w  = rect.w; int h  = rect.h;
	int x1 = rect.x; int x2 = x1 + w;
	int y1 = rect.y; int y2 = y1 + h;

	int i = 0;
	/* Corners - tl, tr, br, bl */
	quadRects[i++] = FloatRect(x1,   y1,   2, 2);
	quadRects[i++] = FloatRect(x2-2, y1,   2, 2);
	quadRects[i++] = FloatRect(x2-2, y2-2, 2, 2);
	quadRects[i++] = FloatRect(x1,   y2-2, 2, 2);

	/* Sides - l, r, t, b */
	quadRects[i++] = FloatRect(x1,   y1+2, 2,   h-4);
	quadRects[i++] = FloatRect(x2-2, y1+2, 2,   h-4);
	quadRects[i++] = FloatRect(x1+2, y1,   w-4, 2);
	quadRects[i++] = FloatRect(x1+2, y2-2, w-4, 2);

	/* Center */
	quadRects[i++] = FloatRect(x1+2, y1+2, w-4, h-4);
}

int buildFrameSource(const IntRect &rect,
                     Vertex vert[36])
{
	FloatRect quadRects[9];

	buildFrameInt(rect, quadRects);

	for (int i = 0; i < 9; ++i)
		Quad::setTexRect(&vert[i*4], quadRects[i]);

	return 9;
}

int buildFrame(const IntRect &rect,
               Vertex vert[36])
{
	FloatRect quadRects[9];

	buildFrameInt(rect, quadRects);

	for (int i = 0; i < 9; ++i)
		Quad::setPosRect(&vert[i*4], quadRects[i]);

	return 9;
}

}
