/*
** rwmem.cpp
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

#include "rwmem.h"
#include "util.h"

#include <SDL_rwops.h>

#include <vector>

typedef std::vector<char> ByteVec;

static inline ByteVec *
getRWPrivate(SDL_RWops *ops)
{
	return static_cast<ByteVec*>(ops->hidden.unknown.data1);
}

static Sint64 SDL_RWopsSize(SDL_RWops *ops)
{
	ByteVec *v = getRWPrivate(ops);

	return v->size();
}

static Sint64 SDL_RWopsSeek(SDL_RWops *, Sint64, int)
{
	/* No need for implementation */

	return -1;
}

static size_t SDL_RWopsRead(SDL_RWops *, void *, size_t, size_t)
{
	/* No need for implementation */

	return 0;
}

static size_t SDL_RWopsWrite(SDL_RWops *ops, const void *buffer, size_t size, size_t num)
{
	ByteVec *v = getRWPrivate(ops);

	size_t writeBytes = size * num;

	if (writeBytes == 1)
	{
		char byte = *static_cast<const char*>(buffer);
		v->push_back(byte);
		return 1;
	}

	size_t writeInd = v->size();
	v->resize(writeInd + writeBytes);

	memcpy(&(*v)[writeInd], buffer, writeBytes);

	return num;
}

static int SDL_RWopsClose(SDL_RWops *ops)
{
	ByteVec *v = getRWPrivate(ops);

	delete v;

	return 0;
}

void RWMemOpen(SDL_RWops *ops)
{
	ByteVec *v = new ByteVec;
	ops->hidden.unknown.data1 = v;

	ops->size  = SDL_RWopsSize;
	ops->seek  = SDL_RWopsSeek;
	ops->read  = SDL_RWopsRead;
	ops->write = SDL_RWopsWrite;
	ops->close = SDL_RWopsClose;
}

int RWMemGetData(SDL_RWops *ops, void *buffer)
{
	ByteVec *v = getRWPrivate(ops);

	if (buffer)
		memcpy(buffer, dataPtr(*v), v->size());

	return v->size();
}
