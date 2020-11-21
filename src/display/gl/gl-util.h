/*
** gl-util.h
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

#ifndef GLUTIL_H
#define GLUTIL_H

#include "gl-fun.h"
#include "etc-internal.h"

/* Struct wrapping GLuint for some light type safety */
#define DEF_GL_ID \
struct ID \
{ \
	GLuint gl; \
	explicit ID(GLuint gl = 0)  \
	    : gl(gl)  \
	{}  \
	ID &operator=(const ID &o)  \
	{  \
		gl = o.gl;  \
		return *this; \
	}  \
	bool operator==(const ID &o) const  \
	{  \
		return gl == o.gl;  \
	}  \
	bool operator!=(const ID &o) const \
	{ \
		return !(*this == o); \
	} \
};

/* 2D Texture */
namespace TEX
{
	DEF_GL_ID

	inline ID gen()
	{
		ID id;
		gl.GenTextures(1, &id.gl);

		return id;
	}

	static inline void del(ID id)
	{
		gl.DeleteTextures(1, &id.gl);
	}

	static inline void bind(ID id)
	{
		gl.BindTexture(GL_TEXTURE_2D, id.gl);
	}

	static inline void unbind()
	{
		bind(ID(0));
	}

	static inline void uploadImage(GLsizei width, GLsizei height, const void *data, GLenum format)
	{
		gl.TexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, format, GL_UNSIGNED_BYTE, data);
	}

	static inline void uploadSubImage(GLint x, GLint y, GLsizei width, GLsizei height, const void *data, GLenum format)
	{
		gl.TexSubImage2D(GL_TEXTURE_2D, 0, x, y, width, height, format, GL_UNSIGNED_BYTE, data);
	}

	static inline void allocEmpty(GLsizei width, GLsizei height)
	{
		gl.TexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
	}

	static inline void setRepeat(bool mode)
	{
		gl.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, mode ? GL_REPEAT : GL_CLAMP_TO_EDGE);
		gl.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, mode ? GL_REPEAT : GL_CLAMP_TO_EDGE);
	}

	static inline void setSmooth(bool mode)
	{
		gl.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, mode ? GL_LINEAR : GL_NEAREST);
		gl.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, mode ? GL_LINEAR : GL_NEAREST);
	}
}

/* Framebuffer Object */
namespace FBO
{
	DEF_GL_ID

	inline ID gen()
	{
		ID id;
		gl.GenFramebuffers(1, &id.gl);

		return id;
	}

	static inline void del(ID id)
	{
		gl.DeleteFramebuffers(1, &id.gl);
	}

	static inline void bind(ID id)
	{
		gl.BindFramebuffer(GL_FRAMEBUFFER, id.gl);
	}

	static inline void unbind()
	{
		bind(ID(0));
	}

	static inline void setTarget(TEX::ID target, unsigned colorAttach = 0)
	{
		gl.FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + colorAttach, GL_TEXTURE_2D, target.gl, 0);
	}

	static inline void clear()
	{
		gl.Clear(GL_COLOR_BUFFER_BIT);
	}
}

template<GLenum target>
struct GenericBO
{
	DEF_GL_ID

	static inline ID gen()
	{
		ID id;
		gl.GenBuffers(1, &id.gl);

		return id;
	}

	static inline void del(ID id)
	{
		gl.DeleteBuffers(1, &id.gl);
	}

	static inline void bind(ID id)
	{
		gl.BindBuffer(target, id.gl);
	}

	static inline void unbind()
	{
		bind(ID(0));
	}

	static inline void uploadData(GLsizeiptr size, const GLvoid *data, GLenum usage = GL_STATIC_DRAW)
	{
		gl.BufferData(target, size, data, usage);
	}

	static inline void uploadSubData(GLintptr offset, GLsizeiptr size, const GLvoid *data)
	{
		gl.BufferSubData(target, offset, size, data);
	}

	static inline void allocEmpty(GLsizeiptr size, GLenum usage = GL_STATIC_DRAW)
	{
		uploadData(size, 0, usage);
	}
};

/* Vertex Buffer Object */
typedef struct GenericBO<GL_ARRAY_BUFFER> VBO;

/* Index Buffer Object */
typedef struct GenericBO<GL_ELEMENT_ARRAY_BUFFER> IBO;

#undef DEF_GL_ID

/* Convenience struct wrapping a framebuffer
 * and a 2D texture as its target */
struct TEXFBO
{
	TEX::ID tex;
	FBO::ID fbo;
	int width, height;

	TEXFBO()
	    : tex(0), fbo(0), width(0), height(0)
	{}

	bool operator==(const TEXFBO &other) const
	{
		return (tex == other.tex) && (fbo == other.fbo);
	}

	static inline void init(TEXFBO &obj)
	{
		obj.tex = TEX::gen();
		obj.fbo = FBO::gen();
		TEX::bind(obj.tex);
		TEX::setRepeat(false);
		TEX::setSmooth(false);
	}

	static inline void allocEmpty(TEXFBO &obj, int width, int height)
	{
		TEX::bind(obj.tex);
		TEX::allocEmpty(width, height);
		obj.width = width;
		obj.height = height;
	}

	static inline void linkFBO(TEXFBO &obj)
	{
		FBO::bind(obj.fbo);
		FBO::setTarget(obj.tex);
	}

	static inline void fini(TEXFBO &obj)
	{
		FBO::del(obj.fbo);
		TEX::del(obj.tex);
	}

	static inline void clear(TEXFBO &obj)
	{
		obj.tex = TEX::ID(0);
		obj.fbo = FBO::ID(0);
		obj.width = obj.height = 0;
	}
};

#endif // GLUTIL_H
