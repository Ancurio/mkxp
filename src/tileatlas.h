#ifndef TILEATLAS_H
#define TILEATLAS_H

#include "etc-internal.h"

#include <QList>

namespace TileAtlas
{

/* Abstract definition of a blit
 * operation with undefined rect width */
struct Blit
{
	Vec2i src;
	Vec2i dst;
	int h;

	Blit(int sx, int sy,
	     int dx, int dy,
	     int h)
	    : src(sx, sy),
	      dst(dx, dy),
	      h(h)
	{}
};

typedef QList<Blit> BlitList;

/* Calculates the minimum atlas size required to hold
 * a tileset of height 'tilesetH'. If the required dimensions
 * exceed 'maxAtlasSize', Vec2i(-1, -1) is returned. */
Vec2i minSize(int tilesetH, int maxAtlasSize);

/* Calculates a series of blits necessary to fill dstRows
 * with srcRows without wasting any space */
BlitList calcBlits(int tilesetH, const Vec2i &atlasSize);

/* Translates a tile coordinate (not pixel!) to a physical
 * pixel coordinate in the atlas */
Vec2i tileToAtlasCoor(int tileX, int tileY, int tilesetH, int atlasH);

}

#endif // TILEATLAS_H
