/*
** file.h
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

#ifndef BINDING_FILE_H
#define BINDING_FILE_H

#include "../binding-util.h"

#include <SDL_rwops.h>

struct FileImpl
{
	SDL_RWops *ops;
	bool closed;

	enum OpenMode
	{
		Read =  1 << 0,
		Write = 1 << 1,
		ReadWrite = Read | Write
	};

	int mode;

	FileImpl(SDL_RWops *ops, int mode)
	    : ops(ops),
	      closed(false),
	      mode(mode)
	{}

	void close()
	{
		if (closed)
			return;

		SDL_RWclose(ops);
		closed = true;
	}

	FILE *fp()
	{
		return ops->hidden.stdio.fp;
	}

	~FileImpl()
	{
		close();
	}
};

DECL_TYPE(File);

#endif // BINDING_FILE_H
