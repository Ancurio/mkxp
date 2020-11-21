/*
** gl-meta.h
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

#ifndef GLMETA_H
#define GLMETA_H

#include "gl-fun.h"
#include "gl-util.h"
#include "vertex.h"

#include <SDL_surface.h>

namespace GLMeta
{

/* EXT_unpack_subimage */
void subRectImageUpload(GLint srcW, GLint srcX, GLint srcY,
                        GLint dstX, GLint dstY, GLsizei dstW, GLsizei dstH,
                        SDL_Surface *src, GLenum format);
void subRectImageEnd();

/* ARB_vertex_array_object */
struct VAO
{
	/* Set manually, then call vaoInit() */
	const VertexAttribute *attr;
	size_t attrCount;
	GLsizei vertSize;
	VBO::ID vbo;
	IBO::ID ibo;

	/* Don't touch */
	GLuint nativeVAO;
};

template<class VertexType>
inline void vaoFillInVertexData(VAO &vao)
{
	vao.attr      = VertexTraits<VertexType>::attr;
	vao.attrCount = VertexTraits<VertexType>::attrCount;
	vao.vertSize  = sizeof(VertexType);
}

void vaoInit(VAO &vao, bool keepBound = false);
void vaoFini(VAO &vao);
void vaoBind(VAO &vao);
void vaoUnbind(VAO &vao);

/* EXT_framebuffer_blit */
void blitBegin(TEXFBO &target);
void blitBeginScreen(const Vec2i &size);
void blitSource(TEXFBO &source);
void blitRectangle(const IntRect &src, const Vec2i &dstPos);
void blitRectangle(const IntRect &src, const IntRect &dst,
                   bool smooth = false);
void blitEnd();

}

#endif // GLMETA_H
