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

#include "GL/glew.h"

#include "etc-internal.h"
#include <QDebug>

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
};

/* 2D Texture */
namespace TEX
{
	DEF_GL_ID

	inline ID gen()
	{
		ID id;
		glGenTextures(1, &id.gl);

		return id;
	}

	inline void del(ID id)
	{
		glDeleteTextures(1, &id.gl);
	}

	inline void bind(ID id)
	{
		glBindTexture(GL_TEXTURE_2D, id.gl);
	}

	inline void unbind()
	{
		bind(ID(0));
	}

	inline void uploadImage(GLsizei width, GLsizei height, const void *data, GLenum format)
	{
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, format, GL_UNSIGNED_BYTE, data);
	}

	inline void uploadSubImage(GLint x, GLint y, GLsizei width, GLsizei height, const void *data, GLenum format)
	{
		glTexSubImage2D(GL_TEXTURE_2D, 0, x, y, width, height, format, GL_UNSIGNED_BYTE, data);
	}

	inline void allocEmpty(GLsizei width, GLsizei height)
	{
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
	}

	inline void setRepeat(bool mode)
	{
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, mode ? GL_REPEAT : GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, mode ? GL_REPEAT : GL_CLAMP_TO_EDGE);
	}

	inline void setSmooth(bool mode)
	{
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, mode ? GL_LINEAR : GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, mode ? GL_LINEAR : GL_NEAREST);
	}
}

/* Renderbuffer Object */
namespace RBO
{
	DEF_GL_ID

	inline ID gen()
	{
		ID id;
		glGenRenderbuffersEXT(1, &id.gl);

		return id;
	}

	inline void del(ID id)
	{
		glDeleteRenderbuffersEXT(1, &id.gl);
	}

	inline void bind(ID id)
	{
		glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, id.gl);
	}

	inline void unbind()
	{
		bind(ID(0));
	}

	inline void allocEmpty(GLsizei width, GLsizei height)
	{
		glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_RGBA8, width, height);
	}
}

/* Framebuffer Object */
namespace FBO
{
	DEF_GL_ID

	enum Mode
	{
		Draw = 0,
		Read = 1
	};

	enum BlitMode
	{
		Nearest = 0,
		Linear  = 1
	};

	inline ID gen()
	{
		ID id;
		glGenFramebuffersEXT(1, &id.gl);

		return id;
	}

	inline void del(ID id)
	{
		glDeleteFramebuffersEXT(1, &id.gl);
	}

	inline void bind(ID id, Mode mode)
	{
		static const GLenum modes[] =
		{
			GL_DRAW_FRAMEBUFFER_EXT,
			GL_READ_FRAMEBUFFER_EXT
		};

		glBindFramebufferEXT(modes[mode], id.gl);
	}

	inline void unbind(Mode mode)
	{
		bind(ID(0), mode);
	}

	inline void setTarget(TEX::ID target, unsigned colorAttach = 0)
	{
		glFramebufferTexture2DEXT(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + colorAttach, GL_TEXTURE_2D, target.gl, 0);
	}

	inline void setTarget(RBO::ID target, unsigned colorAttach = 0)
	{
		glFramebufferRenderbufferEXT(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + colorAttach, GL_RENDERBUFFER, target.gl);
	}

	inline void blit(int srcX, int srcY,
	                 int srcW, int srcH,
	                 int dstX, int dstY,
	                 int dstW, int dstH,
	                 BlitMode mode = Nearest)
	{
		static const GLenum modes[] =
		{
			GL_NEAREST,
			GL_LINEAR
		};

		glBlitFramebufferEXT(srcX, srcY, srcX+srcW, srcY+srcH,
		                     dstX, dstY, dstX+dstW, dstY+dstH,
		                     GL_COLOR_BUFFER_BIT, modes[mode]);
	}

	inline void blit(int srcX, int srcY,
					 int dstX, int dstY,
					 int srcW, int srcH,
	                 BlitMode mode = Nearest)
	{
		blit(srcX, srcY, srcW, srcH, dstX, dstY, srcW, srcH, mode);
	}

	inline Vec4 getPixel(int x, int y)
	{
		Vec4 pixel;

		glReadPixels(x, y, 1, 1, GL_RGBA, GL_FLOAT, &pixel.x);

		return pixel;
	}
}

/* Vertex Array Object */
namespace VAO
{
	DEF_GL_ID

	inline ID gen()
	{
		ID id;
		glGenVertexArrays(1, &id.gl);

		return id;
	}

	inline void del(ID id)
	{
		glDeleteVertexArrays(1, &id.gl);
	}

	inline void bind(ID id)
	{
		glBindVertexArray(id.gl);
	}

	inline void unbind()
	{
		bind(ID(0));
	}
}

template<GLenum target>
struct GenericBO
{
	DEF_GL_ID

	inline static ID gen()
	{
		ID id;
		glGenBuffers(1, &id.gl);

		return id;
	}

	inline static void del(ID id)
	{
		glDeleteBuffers(1, &id.gl);
	}

	inline static void bind(ID id)
	{
		glBindBuffer(target, id.gl);
	}

	inline static void unbind()
	{
		bind(ID(0));
	}

	inline static void uploadData(GLsizeiptr size, const GLvoid *data, GLenum usage = GL_STATIC_DRAW)
	{
		glBufferData(target, size, data, usage);
	}

	inline static void uploadSubData(GLintptr offset, GLsizeiptr size, const GLvoid *data)
	{
		glBufferSubData(target, offset, size, data);
	}

	inline static void allocEmpty(GLsizeiptr size, GLenum usage = GL_STATIC_DRAW)
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
		FBO::bind(obj.fbo, FBO::Draw);
		FBO::setTarget(obj.tex);
	}

	static inline void fini(TEXFBO &obj)
	{
		FBO::del(obj.fbo);
		TEX::del(obj.tex);
	}
};

/* Convenience struct wrapping a framebuffer
 * and a renderbuffer as its target */
struct RBOFBO
{
	RBO::ID rbo;
	FBO::ID fbo;
	int width, height;

	RBOFBO()
	    : rbo(0), fbo(0), width(0), height(0)
	{}

	static inline void init(RBOFBO &obj)
	{
		obj.rbo = RBO::gen();
		obj.fbo = FBO::gen();
	}

	static inline void allocEmpty(RBOFBO &obj, int width, int height)
	{
		RBO::bind(obj.rbo);
		RBO::allocEmpty(width, height);
		obj.width = width;
		obj.height = height;
	}

	static inline void linkFBO(RBOFBO &obj)
	{
		FBO::bind(obj.fbo, FBO::Draw);
		FBO::setTarget(obj.rbo);
	}

	static inline void fini(RBOFBO &obj)
	{
		FBO::del(obj.fbo);
		RBO::del(obj.rbo);
	}
};

#endif // GLUTIL_H
