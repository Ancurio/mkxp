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

#include <vector>
#include <string>

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

	bool fontPresent(std::string family) const;

	static _TTF_Font *openBundled(int size);

private:
	SharedFontStatePrivate *p;
};

struct FontPrivate;

class Font
{
public:
	static bool doesExist(const char *name);

	Font(const std::vector<std::string> *names = 0,
	     int size = 0);

	/* Clone constructor */
	Font(const Font &other);

	~Font();

	const Font &operator=(const Font &o);

	DECL_ATTR( Size,     int    )
	DECL_ATTR( Bold,     bool   )
	DECL_ATTR( Italic,   bool   )
	DECL_ATTR( Color,    Color& )
	DECL_ATTR( Shadow,   bool   )
	DECL_ATTR( Outline,  bool   )
	DECL_ATTR( OutColor, Color& )

	DECL_ATTR_STATIC( DefaultSize,     int    )
	DECL_ATTR_STATIC( DefaultBold,     bool   )
	DECL_ATTR_STATIC( DefaultItalic,   bool   )
	DECL_ATTR_STATIC( DefaultColor,    Color& )
	DECL_ATTR_STATIC( DefaultShadow,   bool   )
	DECL_ATTR_STATIC( DefaultOutline,  bool   )
	DECL_ATTR_STATIC( DefaultOutColor, Color& )

	/* There is no point in providing getters for these,
	 * as the bindings will always return the stored native
	 * string/array object anyway. It's impossible to mirror
	 * in the C++ core.
	 * The core object picks the first existing name from the
	 * passed array and stores it internally (same for default). */
	void setName(const std::vector<std::string> &names);
	static void setDefaultName(const std::vector<std::string> &names,
	                           const SharedFontState &sfs);

	static const std::vector<std::string> &getInitialDefaultNames();

	/* Assigns heap allocated objects to object properties;
	 * using this in pure C++ will cause memory leaks
	 * (ie. only to be used in GCed language bindings) */
	void initDynAttribs();
	static void initDefaultDynAttribs();

	static void initDefaults(const SharedFontState &sfs);

	/* internal */
	_TTF_Font *getSdlFont();

private:
	FontPrivate *p;
};

#endif // FONT_H
