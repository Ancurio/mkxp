/*
** gl-debug.h
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

#ifndef DEBUGLOGGER_H
#define DEBUGLOGGER_H

#include "gl-fun.h"

#include <stdio.h>
#include <algorithm>

struct GLDebugLoggerPrivate;

class GLDebugLogger
{
public:
	GLDebugLogger(const char *filename = 0);
	~GLDebugLogger();

private:
	GLDebugLoggerPrivate *p;
};

#define GL_MARKER(format, ...) \
	if (gl.StringMarker) \
	{ \
		char buf[128]; \
		int len = snprintf(buf, sizeof(buf), format, ##__VA_ARGS__); \
		gl.StringMarker(std::min<size_t>(len, sizeof(buf)), buf); \
	}


#endif // DEBUGLOGGER_H
