/*
** gl-fun.cpp
**
** This file is part of mkxp.
**
** Copyright (C) 2014 Jonas Kulla <Nyocurio@gmail.com>
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

#include "gl-fun.h"

#include "boost-hash.h"
#include "exception.h"

#include <SDL_video.h>
#include <string>

GLFunctions gl;

typedef const GLubyte* (APIENTRYP _PFNGLGETSTRINGIPROC) (GLenum, GLuint);

static void parseExtensionsCore(_PFNGLGETINTEGERVPROC GetIntegerv, BoostSet<std::string> &out)
{
	_PFNGLGETSTRINGIPROC GetStringi =
		(_PFNGLGETSTRINGIPROC) SDL_GL_GetProcAddress("glGetStringi");

	GLint extCount = 0;
	GetIntegerv(GL_NUM_EXTENSIONS, &extCount);

	for (GLint i = 0; i < extCount; ++i)
		out.insert((const char*) GetStringi(GL_EXTENSIONS, i));
}

static void parseExtensionsCompat(_PFNGLGETSTRINGPROC GetString, BoostSet<std::string> &out)
{
	const char *ext = (const char*) GetString(GL_EXTENSIONS);

	if (!ext)
		return;

	char buffer[0x100];
	size_t bufferI;

	while (*ext)
	{
		bufferI = 0;
		while (*ext && *ext != ' ')
			buffer[bufferI++] = *ext++;

		buffer[bufferI] = '\0';

		out.insert(buffer);

		if (*ext == ' ')
			++ext;
	}
}

#define GL_FUN(name, type) \
	gl.name = (type) SDL_GL_GetProcAddress("gl" #name EXT_SUFFIX);

#define EXC(msg) \
	Exception(Exception::MKXPError, "%s", msg)

void initGLFunctions()
{
#define EXT_SUFFIX ""
	GL_20_FUN;

	/* Determine GL version */
	const char *ver = (const char*) gl.GetString(GL_VERSION);

	const char glesPrefix[] = "OpenGL ES ";
	const size_t glesPrefixN = sizeof(glesPrefix)-1;

	bool gles = false;

	if (!strncmp(ver, glesPrefix, glesPrefixN))
	{
		gles = true;
		gl.glsles = true;

		ver += glesPrefixN;
	}

	/* Assume single digit */
	int glMajor = *ver - '0';

	if (glMajor < 2)
		throw EXC("OpenGL (ES) version >= 2 required");

	if (gles)
	{
		GL_ES_FUN;
	}

	BoostSet<std::string> ext;

	if (glMajor >= 3)
		parseExtensionsCore(gl.GetIntegerv, ext);
	else
		parseExtensionsCompat(gl.GetString, ext);

#define HAVE_EXT(_ext) ext.contains("GL_" #_ext)

	/* FBO entrypoints */
	if (glMajor >= 3 || HAVE_EXT(ARB_framebuffer_object))
	{
#undef EXT_SUFFIX
#define EXT_SUFFIX ""
		GL_FBO_FUN;
		GL_FBO_BLIT_FUN;
	}
	else if (gles && glMajor == 2)
	{
		GL_FBO_FUN;
	}
	else if (HAVE_EXT(EXT_framebuffer_object))
	{
#undef EXT_SUFFIX
#define EXT_SUFFIX "EXT"
		GL_FBO_FUN;

		if (HAVE_EXT(EXT_framebuffer_blit))
		{
			GL_FBO_BLIT_FUN;
		}
	}
	else
	{
		throw EXC("No FBO support available");
	}

	/* VAO entrypoints */
	if (HAVE_EXT(ARB_vertex_array_object) || glMajor >= 3)
	{
#undef EXT_SUFFIX
#define EXT_SUFFIX ""
		GL_VAO_FUN;
	}
	else if (HAVE_EXT(APPLE_vertex_array_object))
	{
#undef EXT_SUFFIX
#define EXT_SUFFIX "APPLE"
		GL_VAO_FUN;
	}
	else if (HAVE_EXT(OES_vertex_array_object))
	{
#undef EXT_SUFFIX
#define EXT_SUFFIX "OES"
		GL_VAO_FUN;
	}

	/* Debug callback entrypoints */
	if (HAVE_EXT(KHR_debug))
	{
#undef EXT_SUFFIX
#define EXT_SUFFIX ""
		GL_DEBUG_KHR_FUN;
	}
	else if (HAVE_EXT(ARB_debug_output))
	{
#undef EXT_SUFFIX
#define EXT_SUFFIX "ARB"
		GL_DEBUG_KHR_FUN;
	}

	if (HAVE_EXT(GREMEDY_string_marker))
	{
#undef EXT_SUFFIX
#define EXT_SUFFIX "GREMEDY"
		GL_GREMEMDY_FUN;
	}

	/* Misc caps */
	if (!gles || glMajor >= 3 || HAVE_EXT(EXT_unpack_subimage))
		gl.unpack_subimage = true;

	if (!gles || glMajor >= 3 || HAVE_EXT(OES_texture_npot))
		gl.npot_repeat = true;
}
