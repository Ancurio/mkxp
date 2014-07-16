/*
** gl-meta.cpp
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

#include "gl-meta.h"
#include "gl-fun.h"
#include "sharedstate.h"
#include "glstate.h"
#include "quad.h"

namespace GLMeta
{

void subRectImageUpload(GLint srcW, GLint srcX, GLint srcY,
                        GLint dstX, GLint dstY, GLsizei dstW, GLsizei dstH,
                        SDL_Surface *src, GLenum format)
{
	if (gl.unpack_subimage)
	{
		PixelStore::setupSubImage(srcW, srcX, srcY);
		TEX::uploadSubImage(dstX, dstY, dstW, dstH, src->pixels, format);
	}
	else
	{
		SDL_PixelFormat *form = src->format;
		SDL_Surface *tmp = SDL_CreateRGBSurface(0, dstW, dstH, form->BitsPerPixel,
		                                        form->Rmask, form->Gmask, form->Bmask, form->Amask);
		SDL_Rect srcRect = { srcX, srcY, dstW, dstH };

		SDL_BlitSurface(src, &srcRect, tmp, 0);

		TEX::uploadSubImage(dstX, dstY, dstW, dstH, tmp->pixels, format);

		SDL_FreeSurface(tmp);
	}
}

void subRectImageFinish()
{
	if (gl.unpack_subimage)
		PixelStore::reset();
}

#define HAVE_NATIVE_VAO gl.GenVertexArrays

static void vaoBindRes(VAO &vao)
{
	VBO::bind(vao.vbo);
	IBO::bind(vao.ibo);

	for (size_t i = 0; i < vao.attrCount; ++i)
	{
		const VertexAttribute &va = vao.attr[i];

		gl.EnableVertexAttribArray(va.index);
		gl.VertexAttribPointer(va.index, va.size, va.type, GL_FALSE, vao.vertSize, va.offset);
	}
}

void vaoInit(VAO &vao, bool keepBound)
{
	if (HAVE_NATIVE_VAO)
	{
		vao.vao = ::VAO::gen();
		::VAO::bind(vao.vao);
		vaoBindRes(vao);
		if (!keepBound)
			::VAO::unbind();
	}
	else
	{
		if (keepBound)
		{
			VBO::bind(vao.vbo);
			IBO::bind(vao.ibo);
		}
	}
}

void vaoFini(VAO &vao)
{
	if (HAVE_NATIVE_VAO)
		::VAO::del(vao.vao);
}

void vaoBind(VAO &vao)
{
	if (HAVE_NATIVE_VAO)
		::VAO::bind(vao.vao);
	else
		vaoBindRes(vao);
}

void vaoUnbind(VAO &vao)
{
	if (HAVE_NATIVE_VAO)
	{
		::VAO::unbind();
	}
	else
	{
		for (size_t i = 0; i < vao.attrCount; ++i)
			gl.DisableVertexAttribArray(vao.attr[i].index);

		VBO::unbind();
		IBO::unbind();
	}
}

#define HAVE_NATIVE_BLIT gl.BlitFramebuffer

static void _blitBegin(FBO::ID fbo, const Vec2i &size)
{
	if (HAVE_NATIVE_BLIT)
	{
		FBO::bind(fbo, FBO::Draw);
	}
	else
	{
		FBO::bind(fbo, FBO::Generic);
		glState.viewport.pushSet(IntRect(0, 0, size.x, size.y));

		SimpleShader &shader = shState->shaders().simple;
		shader.bind();
		shader.applyViewportProj();
		shader.setTranslation(Vec2i());
	}
}

void blitBegin(TEXFBO &target)
{
	_blitBegin(target.fbo, Vec2i(target.width, target.height));
}

void blitBeginScreen(const Vec2i &size)
{
	_blitBegin(FBO::ID(0), size);
}

void blitSource(TEXFBO &source)
{
	if (HAVE_NATIVE_BLIT)
	{
		FBO::bind(source.fbo, FBO::Read);
	}
	else
	{
		SimpleShader &shader = shState->shaders().simple;
		shader.setTexSize(Vec2i(source.width, source.height));
		TEX::bind(source.tex);
	}
}

void blitRectangle(const IntRect &src, const Vec2i &dstPos, FBO::BlitMode mode)
{
	blitRectangle(src, IntRect(dstPos.x, dstPos.y, src.w, src.h), mode);
}

void blitRectangle(const IntRect &src, const IntRect &dst, FBO::BlitMode mode)
{
	if (HAVE_NATIVE_BLIT)
	{
		FBO::blit(src.x, src.y, src.w, src.h,
		          dst.x, dst.y, dst.w, dst.h,
		          mode);
	}
	else
	{
		if (mode == FBO::Linear)
			TEX::setSmooth(true);

		Quad &quad = shState->gpQuad();
		quad.setTexPosRect(src, dst);
		quad.draw();

		if (mode == FBO::Linear)
			TEX::setSmooth(false);
	}
}

void blitFinish()
{
	if (!HAVE_NATIVE_BLIT)
		glState.viewport.pop();
}

}
