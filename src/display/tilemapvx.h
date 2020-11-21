/*
** tilemapvx.h
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

#ifndef TILEMAPVX_H
#define TILEMAPVX_H

#include "disposable.h"
#include "util.h"

class Viewport;
class Bitmap;
class Table;

struct TilemapVXPrivate;

class TilemapVX : public Disposable
{
public:
	class BitmapArray
	{
	public:
		void set(int i, Bitmap *bitmap);
		Bitmap *get(int i) const;

	private:
		BitmapArray() {}
		~BitmapArray() {}

		TilemapVXPrivate *p;
		friend class TilemapVX;
		friend struct TilemapVXPrivate;
	};

	TilemapVX(Viewport *viewport = 0);
	~TilemapVX();

	void update();

	BitmapArray &getBitmapArray();

	DECL_ATTR( Viewport,   Viewport* )
	DECL_ATTR( MapData,    Table*    )
	DECL_ATTR( FlashData,  Table*    )
	DECL_ATTR( Flags,      Table*    )
	DECL_ATTR( Visible,    bool      )
	DECL_ATTR( OX,         int       )
	DECL_ATTR( OY,         int       )

private:
	TilemapVXPrivate *p;
	BitmapArray bmProxy;

	void releaseResources();
	const char *klassName() const { return "tilemap"; }
};

#endif // TILEMAPVX_H
