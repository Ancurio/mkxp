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

struct SDL_RWops;
struct _TTF_Font;
struct Config;

struct SharedFontStatePrivate;

class SharedFontState
{
public:
	SharedFontState(const Config &conf);
	~SharedFontState();

	/* Called from FileSystem during font cache initialization
	 * (when "Fonts/" is scanned for available assets).
	 * 'ops' is an opened handle to a possible font file,
	 * 'filename' is the corresponding path */
	void initFontSetCB(SDL_RWops &ops,
	                   const std::string &filename);

	_TTF_Font *getFont(std::string family,
	                   int size);

	bool fontPresent(std::string family);

	static _TTF_Font *openBundled(int size);

private:
	SharedFontStatePrivate *p;
};

/* Concerning Font::name/defaultName :
 * In RGSS, this is not actually a string; any type of
 * object is accepted, however anything but strings and
 * arrays is ignored (and text drawing turns blank).
 * Single strings are interpreted as font family names,
 * and directly passed to the underlying C++ object;
 * arrays however are searched for the first string
 * object corresponding to a valid font family name,
 * and rendering is done with that. In mkxp, we pass
 * this first valid font family as the 'name' attribute
 * back to the C++ object on assignment and object
 * creation (in case Font.default_name is also an array).
 * Invalid parameters (things other than strings or
 * arrays not containing any valid family name) are
 * passed back as "". */

struct FontPrivate;

class Font
{
public:
	static bool doesExist(const char *name);

	Font(const char *name = 0,
	     int size = 0);
	/* Clone constructor */
	Font(const Font &other);
	~Font();

	const Font &operator=(const Font &o);

	DECL_ATTR( Name,     const char * )
	DECL_ATTR( Size,     int          )
	DECL_ATTR( Bold,     bool         )
	DECL_ATTR( Italic,   bool         )
	DECL_ATTR( Color,    Color&       )
	DECL_ATTR( Shadow,   bool         )
	DECL_ATTR( Outline,  bool         )
	DECL_ATTR( OutColor, Color&       )

	DECL_ATTR_STATIC( DefaultName,     const char* )
	DECL_ATTR_STATIC( DefaultSize,     int         )
	DECL_ATTR_STATIC( DefaultBold,     bool        )
	DECL_ATTR_STATIC( DefaultItalic,   bool        )
	DECL_ATTR_STATIC( DefaultColor,    Color&      )
	DECL_ATTR_STATIC( DefaultShadow,   bool        )
	DECL_ATTR_STATIC( DefaultOutline,  bool        )
	DECL_ATTR_STATIC( DefaultOutColor, Color&      )

	/* Assigns heap allocated objects to object properties;
	 * using this in pure C++ will cause memory leaks
	 * (ie. only to be used in GCed language bindings) */
	void initDynAttribs();
	static void initDefaultDynAttribs();

	static void initDefaults();

	/* internal */
	_TTF_Font *getSdlFont();

private:
	FontPrivate *p;
};

#endif // FONT_H
