/*
** texpool.h
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

#ifndef TEXPOOL_H
#define TEXPOOL_H

#include "gl-util.h"

struct TexPoolPrivate;

class TexPool
{
public:
	TexPool(uint32_t maxMemSize = 20000000 /* 20 MB */);
	~TexPool();

	TEXFBO request(int width, int height);
	void release(TEXFBO &obj);

	void disable();

private:
	TexPoolPrivate *p;
};

#endif // TEXPOOL_H
