/*
** filesystem.h
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

#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include <SDL_rwops.h>
#include <string>

struct FileSystemPrivate;
class SharedFontState;

class FileSystem
{
public:
	FileSystem(const char *argv0,
	           bool allowSymlinks);
	~FileSystem();

	void addPath(const char *path);

	/* Call these after the last 'addPath()' */
	void createPathCache();

	/* Scans "Fonts/" and creates inventory of
	 * available font assets */
	void initFontSets(SharedFontState &sfs);

	struct OpenHandler
	{
		/* Try to read and interpret data provided from ops.
		 * If data cannot be parsed, return false, otherwise true.
		 * Can be called multiple times until a parseable file is found.
		 * It's the handler's responsibility to close every passed
		 * ops structure, even when data could not be parsed.
		 * After this function returns, ops becomes invalid, so don't take
		 * references to it. Instead, copy the structure without closing
		 * if you need to further read from it later. */
		virtual bool tryRead(SDL_RWops &ops, const char *ext) = 0;
	};

	void openRead(OpenHandler &handler,
	              const char *filename);

	/* Circumvents extension supplementing */
	void openReadRaw(SDL_RWops &ops,
	                 const char *filename,
	                 bool freeOnClose = false);

	char *normalize(const char *pathname, bool preferred, bool absolute);

	/* Does not perform extension supplementing */
	bool exists(const char *filename);

	const char *desensitize(const char *filename);

private:
	FileSystemPrivate *p;
};

extern const Uint32 SDL_RWOPS_PHYSFS;

#endif // FILESYSTEM_H
