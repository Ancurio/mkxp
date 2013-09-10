/*
** font.cpp
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

#include "font.h"

#include "globalstate.h"
#include "filesystem.h"
#include "exception.h"

#include "../assets/liberation.ttf.xxd"

#include "SDL2/SDL_ttf.h"

#include <QHash>
#include <QByteArray>
#include <QPair>

#include <QDebug>

#define BUNDLED_FONT liberation

#define BUNDLED_FONT_D(f) assets_## f ##_ttf
#define BUNDLED_FONT_L(f) assets_## f ##_ttf_len

// Go fuck yourself CPP
#define BNDL_F_D(f) BUNDLED_FONT_D(f)
#define BNDL_F_L(f) BUNDLED_FONT_L(f)

typedef QPair<QByteArray, int> FontKey;

struct FontPoolPrivate
{
	QHash<FontKey, TTF_Font*> hash;
};

FontPool::FontPool()
{
	p = new FontPoolPrivate;
}

FontPool::~FontPool()
{
	QHash<FontKey, TTF_Font*>::const_iterator iter;
	for (iter = p->hash.begin(); iter != p->hash.end(); ++iter)
		TTF_CloseFont(iter.value());

	delete p;
}

static SDL_RWops *openBundledFont()
{
	return SDL_RWFromConstMem(BNDL_F_D(BUNDLED_FONT), BNDL_F_L(BUNDLED_FONT));
}

_TTF_Font *FontPool::request(const char *filename,
                            int size)
{
	// FIXME Find out how font path resolution is done in VX/Ace
	QByteArray nameKey = QByteArray(filename).toLower();
	nameKey.replace(' ', '_');

	bool useBundled = false;
	QByteArray path = QByteArray("Fonts/") + nameKey;
	if (!gState->fileSystem().exists(path.constData(), FileSystem::Font))
	{
		useBundled = true;
		nameKey = " bundled";
	}

	FontKey key(nameKey, size);

	TTF_Font *font = p->hash.value(key, 0);

	if (font)
	{
//		static int i=0;qDebug() << "FontPool: <?+>" << i++;
		return font;
	}

//	qDebug() << "FontPool: <?->";

	/* Not in hash, create */
	SDL_RWops *ops;

	if (useBundled)
	{
		ops = openBundledFont();
	}
	else
	{
		ops = SDL_AllocRW();
		gState->fileSystem().openRead(*ops, path.constData(), FileSystem::Font, true);
	}

	// FIXME 0.9 is guesswork at this point
	font = TTF_OpenFontRW(ops, 1, (float) size * .90);

	if (!font)
		throw Exception(Exception::SDLError, "SDL: %s", SDL_GetError());

	p->hash.insert(key, font);

	return font;
}


struct FontPrivate
{
	QByteArray name;
	int size;
	bool bold;
	bool italic;
	Color *color;

	Color colorTmp;

	static QByteArray defaultName;
	static int defaultSize;
	static bool defaultBold;
	static bool defaultItalic;
	static Color *defaultColor;

	static Color defaultColorTmp;

	TTF_Font *sdlFont;

	FontPrivate(const char *name = 0,
	            int size = 0)
	    : name(name ? QByteArray(name) : defaultName),
	      size(size ? size : defaultSize),
	      bold(defaultBold),
	      italic(defaultItalic),
	      color(&colorTmp),
	      colorTmp(*defaultColor)
	{
		sdlFont = gState->fontPool().request(this->name.constData(),
		                                     this->size);
	}
};

QByteArray FontPrivate::defaultName   = "Arial";
int        FontPrivate::defaultSize   = 22;
bool       FontPrivate::defaultBold   = false;
bool       FontPrivate::defaultItalic = false;
Color     *FontPrivate::defaultColor  = &FontPrivate::defaultColorTmp;

Color FontPrivate::defaultColorTmp(255, 255, 255, 255);

bool Font::doesExist(const char *name)
{
	QByteArray path = QByteArray("fonts/") + QByteArray(name);

	return gState->fileSystem().exists(path.constData(), FileSystem::Font);
}

Font::Font(const char *name,
           int size)
{
	p = new FontPrivate(name, size);
}

Font::~Font()
{
	delete p;
}

const char *Font::getName() const
{
	return p->name.constData();
}

void Font::setName(const char *value)
{
	p->name = value;
}

void Font::setSize(int value)
{
	if (p->size == value)
		return;

	p->size = value;
	p->sdlFont = gState->fontPool().request(p->name.constData(), value);
}

#undef CHK_DISP
#define CHK_DISP

DEF_ATTR_RD_SIMPLE(Font, Size, int, p->size)
DEF_ATTR_SIMPLE(Font, Bold, bool, p->bold)
DEF_ATTR_SIMPLE(Font, Italic, bool, p->italic)
DEF_ATTR_SIMPLE(Font, Color, Color*, p->color)

DEF_ATTR_SIMPLE_STATIC(Font, DefaultSize, int, FontPrivate::defaultSize)
DEF_ATTR_SIMPLE_STATIC(Font, DefaultBold, bool, FontPrivate::defaultBold)
DEF_ATTR_SIMPLE_STATIC(Font, DefaultItalic, bool, FontPrivate::defaultItalic)
DEF_ATTR_SIMPLE_STATIC(Font, DefaultColor, Color*, FontPrivate::defaultColor)

const char *Font::getDefaultName()
{
	return FontPrivate::defaultName.constData();
}

void Font::setDefaultName(const char *value)
{
	FontPrivate::defaultName = value;
}

_TTF_Font *Font::getSdlFont()
{
	int style = TTF_STYLE_NORMAL;

	if (p->bold)
		style |= TTF_STYLE_BOLD;

	if (p->italic)
		style |= TTF_STYLE_ITALIC;

	TTF_SetFontStyle(p->sdlFont, style);

	return p->sdlFont;
}
