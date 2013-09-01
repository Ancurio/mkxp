/*
** font.h
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

#ifndef FONT_H
#define FONT_H

#include "etc.h"
#include "util.h"

struct _TTF_Font;
struct FontPoolPrivate;

class FontPool
{
public:
	FontPool();
	~FontPool();

	_TTF_Font *request(const char *filename,
	                  int size);

private:
	FontPoolPrivate *p;
};

struct FontPrivate;

class Font
{
public:
	static bool doesExist(const char *name);

	Font(const char *name = 0,
	     int size = 0);
	~Font();

	const char *getName() const;
	void setName(const char *value);

	DECL_ATTR( Size,   int )
	DECL_ATTR( Bold,   bool )
	DECL_ATTR( Italic, bool )
	DECL_ATTR( Color,  Color* )

	DECL_ATTR_STATIC( DefaultName,   const char* )
	DECL_ATTR_STATIC( DefaultSize,   int         )
	DECL_ATTR_STATIC( DefaultBold,   bool        )
	DECL_ATTR_STATIC( DefaultItalic, bool        )
	DECL_ATTR_STATIC( DefaultColor,  Color*      )

	/* internal */
	_TTF_Font *getSdlFont();

private:
	FontPrivate *p;
};

#endif // FONT_H
