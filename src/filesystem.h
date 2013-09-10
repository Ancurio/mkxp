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

#include "SFML/System/InputStream.hpp"

#include "SDL2/SDL_rwops.h"

struct PHYSFS_File;

class FileStream : public sf::InputStream
{
public:
	FileStream(PHYSFS_File *);
	~FileStream();

	void operator=(const FileStream &o);

	sf::Int64 read(void *data, sf::Int64 size);
	sf::Int64 seek(sf::Int64 position);
	sf::Int64 tell();
	sf::Int64 getSize();

	sf::Int64 write(const void *data, sf::Int64 size);

	void close();

private:
	PHYSFS_File *p; /* NULL denotes invalid stream */
};

class FileSystem
{
public:
	FileSystem(const char *argv0);
	~FileSystem();

	void addPath(const char *path);

	/* For extension supplementing */
	enum FileType
	{
		Image = 0,
		Audio,
		Font,
		Undefined
	};

	FileStream openRead(const char *filename,
	                    FileType type = Undefined);

	void openRead(SDL_RWops &ops,
	              const char *filename,
	              FileType type = Undefined,
	              bool freeOnClose = false);

	bool exists(const char *filename,
	            FileType type = Undefined);
};

extern const Uint32 SDL_RWOPS_PHYSFS;

#endif // FILESYSTEM_H
