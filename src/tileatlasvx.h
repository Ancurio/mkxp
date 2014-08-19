/*
** tileatlasvx.h
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

#ifndef TILEATLASVX_H
#define TILEATLASVX_H

#include <stdlib.h>

struct FloatRect;
struct TEXFBO;
class Bitmap;
class Table;

#define ATLASVX_W 1024
#define ATLASVX_H 2048

/* Bitmap indices */
enum
{
	BM_A1 = 0,
	BM_A2 = 1,
	BM_A3 = 2,
	BM_A4 = 3,
	BM_A5 = 4,
	BM_B  = 5,
	BM_C  = 6,
	BM_D  = 7,
	BM_E  = 8,

	BM_COUNT
};

namespace TileAtlasVX
{
struct Reader
{
	virtual void onQuads(const FloatRect *t, const FloatRect *p,
	                     size_t n, bool overPlayer) = 0;
};

void build(TEXFBO &tf, Bitmap *bitmaps[BM_COUNT]);

void readTiles(Reader &reader, const Table &data,
               const Table *flags, int ox, int oy, int w, int h);
}

#endif // TILEATLASVX_H
