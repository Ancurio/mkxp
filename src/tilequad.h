/*
** tilequad.h
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

#ifndef TILEQUAD_H
#define TILEQUAD_H

#include "etc-internal.h"

/* Tiled Quads
 *
 * These functions enable drawing a tiled subrectangle
 * of a texture,
 * but no advanced stuff like rotation, scaling etc.
 */

#include "quadarray.h"

namespace TileQuads
{
	/* Calculate needed quad counts */
	int oneDimCount(int tileDimension,
	                int destDimension);
	int twoDimCount(int tileW, int tileH,
	                int destW, int destH);

	/* Build tiling quads */
	int buildH(const IntRect &sourceRect,
	           int width, int x, int y,
	           Vertex *verts);

	int buildV(const IntRect &sourceRect,
	           int height, int ox, int oy,
	           Vertex *verts);

	int build(const IntRect &sourceRect,
	          const IntRect &destRect,
	          Vertex *verts);

	/* Build a quad "frame" (see Window cursor_rect) */
	int buildFrame(const IntRect &rect,
	               Vertex vert[36]);

	int buildFrameSource(const IntRect &rect,
	                     Vertex vert[36]);
}

#endif // TILEQUAD_H
