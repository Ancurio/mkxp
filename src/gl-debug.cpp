/*
** gl-debug.cpp
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

#include "gl-debug.h"
#include "debugwriter.h"

#include <iostream>

#include "gl-fun.h"

struct GLDebugLoggerPrivate
{
	std::ostream *stream;

	GLDebugLoggerPrivate(const char *logFilename)
	{
		(void) logFilename;

		stream = &std::clog;
	}

	~GLDebugLoggerPrivate()
	{
	}

	void writeTimestamp()
	{
		// FIXME reintroduce proper time stamps (is this even necessary??)
		*stream << "[GLDEBUG] ";
	}

	void writeLine(const char *line)
	{
		*stream << line << "\n";
		stream->flush();
	}
};

static void APIENTRY arbDebugFunc(GLenum source,
                                  GLenum type,
                                  GLuint id,
                                  GLenum severity,
                                  GLsizei length,
                                  const GLchar* message,
                                  const void* userParam)
{
	GLDebugLoggerPrivate *p =
		static_cast<GLDebugLoggerPrivate*>(const_cast<void*>(userParam));

	(void) source;
	(void) type;
	(void) id;
	(void) severity;
	(void) length;

	p->writeTimestamp();
	p->writeLine(message);
}

GLDebugLogger::GLDebugLogger(const char *filename)
{
	p = new GLDebugLoggerPrivate(filename);

	if (gl.DebugMessageCallback)
		gl.DebugMessageCallback(arbDebugFunc, p);
	else
		Debug() << "DebugLogger: no debug extensions found";
}

GLDebugLogger::~GLDebugLogger()
{
	delete p;
}
