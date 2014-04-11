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

struct FileSystemPrivate;
struct SharedFontState;

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

	/* For extension supplementing */
	enum FileType
	{
		Image = 0,
		Audio,
		Font,
		Undefined
	};

	void openRead(SDL_RWops &ops,
	              const char *filename,
	              FileType type = Undefined,
	              bool freeOnClose = false,
	              const char **foundExt = 0);

	/* Circumvents extension supplementing */
	void openReadRaw(SDL_RWops &ops,
	                 const char *filename,
	                 bool freeOnClose = false);

	bool exists(const char *filename,
	            FileType type = Undefined);

private:
	FileSystemPrivate *p;
};

extern const Uint32 SDL_RWOPS_PHYSFS;

#endif // FILESYSTEM_H
