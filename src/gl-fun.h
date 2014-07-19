/*
** gl-fun.h
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

#ifndef GLFUN_H
#define GLFUN_H

#include <SDL_opengl.h>

/* Etc */
typedef GLenum (APIENTRYP PFNGLGETERRORPROC) (void);
typedef void (APIENTRYP PFNGLCLEARCOLORPROC) (GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha);
typedef void (APIENTRYP PFNGLCLEARPROC) (GLbitfield mask);
typedef const GLubyte * (APIENTRYP PFNGLGETSTRINGPROC) (GLenum name);
typedef void (APIENTRYP PFNGLGETINTEGERVPROC) (GLenum pname, GLint *params);
typedef void (APIENTRYP PFNGLPIXELSTOREIPROC) (GLenum pname, GLint param);
typedef void (APIENTRYP PFNGLREADPIXELSPROC) (GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid *pixels);
typedef void (APIENTRYP PFNGLENABLEPROC) (GLenum cap);
typedef void (APIENTRYP PFNGLDISABLEPROC) (GLenum cap);
typedef void (APIENTRYP PFNGLSCISSORPROC) (GLint x, GLint y, GLsizei width, GLsizei height);
typedef void (APIENTRYP PFNGLVIEWPORTPROC) (GLint x, GLint y, GLsizei width, GLsizei height);
typedef void (APIENTRYP PFNGLBLENDFUNCPROC) (GLenum sfactor, GLenum dfactor);
typedef void (APIENTRYP PFNGLBLENDFUNCSEPARATEPROC) (GLenum sfactorRGB, GLenum dfactorRGB, GLenum sfactorAlpha, GLenum dfactorAlpha);
typedef void (APIENTRYP PFNGLBLENDEQUATIONPROC) (GLenum mode);
typedef void (APIENTRYP PFNGLDRAWELEMENTSPROC) (GLenum mode, GLsizei count, GLenum type, const GLvoid *indices);

/* Texture */
typedef void (APIENTRYP PFNGLGENTEXTURESPROC) (GLsizei n, GLuint *textures);
typedef void (APIENTRYP PFNGLDELETETEXTURESPROC) (GLsizei n, const GLuint *textures);
typedef void (APIENTRYP PFNGLBINDTEXTUREPROC) (GLenum target, GLuint texture);
typedef void (APIENTRYP PFNGLTEXIMAGE2DPROC) (GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels);
typedef void (APIENTRYP PFNGLTEXSUBIMAGE2DPROC) (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels);
typedef void (APIENTRYP PFNGLTEXPARAMETERIPROC) (GLenum target, GLenum pname, GLint param);
typedef void (APIENTRYP PFNGLACTIVETEXTUREPROC) (GLenum texture);

/* Debug callback */
typedef void (APIENTRY * GLDEBUGPROC) (GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void *userParam);
typedef void (APIENTRYP PFNGLDEBUGMESSAGECALLBACKPROC) (GLDEBUGPROC callback, const void *userParam);

#define GL_20_FUN \
	/* Etc */ \
	GL_FUN(GetError, PFNGLGETERRORPROC) \
	GL_FUN(ClearColor, PFNGLCLEARCOLORPROC) \
	GL_FUN(Clear, PFNGLCLEARPROC) \
	GL_FUN(GetString, PFNGLGETSTRINGPROC) \
	GL_FUN(GetIntegerv, PFNGLGETINTEGERVPROC) \
	GL_FUN(PixelStorei, PFNGLPIXELSTOREIPROC) \
	GL_FUN(ReadPixels, PFNGLREADPIXELSPROC) \
	GL_FUN(Enable, PFNGLENABLEPROC) \
	GL_FUN(Disable, PFNGLDISABLEPROC) \
	GL_FUN(Scissor, PFNGLSCISSORPROC) \
	GL_FUN(Viewport, PFNGLVIEWPORTPROC) \
	GL_FUN(BlendFunc, PFNGLBLENDFUNCPROC) \
	GL_FUN(BlendFuncSeparate, PFNGLBLENDFUNCSEPARATEPROC) \
	GL_FUN(BlendEquation, PFNGLBLENDEQUATIONPROC) \
	GL_FUN(DrawElements, PFNGLDRAWELEMENTSPROC) \
	/* Texture */ \
	GL_FUN(GenTextures, PFNGLGENTEXTURESPROC) \
	GL_FUN(DeleteTextures, PFNGLDELETETEXTURESPROC) \
	GL_FUN(BindTexture, PFNGLBINDTEXTUREPROC) \
	GL_FUN(TexImage2D, PFNGLTEXIMAGE2DPROC) \
	GL_FUN(TexSubImage2D, PFNGLTEXSUBIMAGE2DPROC) \
	GL_FUN(TexParameteri, PFNGLTEXPARAMETERIPROC) \
	GL_FUN(ActiveTexture, PFNGLACTIVETEXTUREPROC) \
	/* Buffer objcet */ \
	GL_FUN(GenBuffers, PFNGLGENBUFFERSPROC) \
	GL_FUN(DeleteBuffers, PFNGLDELETEBUFFERSPROC) \
	GL_FUN(BindBuffer, PFNGLBINDBUFFERPROC) \
	GL_FUN(BufferData, PFNGLBUFFERDATAPROC) \
	GL_FUN(BufferSubData, PFNGLBUFFERSUBDATAPROC) \
	/* Shader */ \
	GL_FUN(CreateShader, PFNGLCREATESHADERPROC) \
	GL_FUN(DeleteShader, PFNGLDELETESHADERPROC) \
	GL_FUN(ShaderSource, PFNGLSHADERSOURCEPROC) \
	GL_FUN(CompileShader, PFNGLCOMPILESHADERPROC) \
	GL_FUN(AttachShader, PFNGLATTACHSHADERPROC) \
	GL_FUN(GetShaderiv, PFNGLGETSHADERIVPROC) \
	GL_FUN(GetShaderInfoLog, PFNGLGETSHADERINFOLOGPROC) \
	/* Program */ \
	GL_FUN(CreateProgram, PFNGLCREATEPROGRAMPROC) \
	GL_FUN(DeleteProgram, PFNGLDELETEPROGRAMPROC) \
	GL_FUN(UseProgram, PFNGLUSEPROGRAMPROC) \
	GL_FUN(LinkProgram, PFNGLLINKPROGRAMPROC) \
	GL_FUN(GetProgramiv, PFNGLGETPROGRAMIVPROC) \
	GL_FUN(GetProgramInfoLog, PFNGLGETPROGRAMINFOLOGPROC) \
	/* Uniform */ \
	GL_FUN(GetUniformLocation, PFNGLGETUNIFORMLOCATIONPROC) \
	GL_FUN(Uniform1f, PFNGLUNIFORM1FPROC) \
	GL_FUN(Uniform2f, PFNGLUNIFORM2FPROC) \
	GL_FUN(Uniform4f, PFNGLUNIFORM4FPROC) \
	GL_FUN(Uniform1i, PFNGLUNIFORM1IPROC) \
	GL_FUN(UniformMatrix4fv, PFNGLUNIFORMMATRIX4FVPROC) \
	/* Vertex attribute */ \
	GL_FUN(BindAttribLocation, PFNGLBINDATTRIBLOCATIONPROC) \
	GL_FUN(EnableVertexAttribArray, PFNGLENABLEVERTEXATTRIBARRAYPROC) \
	GL_FUN(DisableVertexAttribArray, PFNGLDISABLEVERTEXATTRIBARRAYPROC) \
	GL_FUN(VertexAttribPointer, PFNGLVERTEXATTRIBPOINTERPROC)

#define GL_FBO_FUN \
	/* Framebuffer object */ \
	GL_FUN(GenFramebuffers, PFNGLGENFRAMEBUFFERSPROC) \
	GL_FUN(DeleteFramebuffers, PFNGLDELETEFRAMEBUFFERSPROC) \
	GL_FUN(BindFramebuffer, PFNGLBINDFRAMEBUFFERPROC) \
	GL_FUN(FramebufferTexture2D, PFNGLFRAMEBUFFERTEXTURE2DPROC) \
	GL_FUN(FramebufferRenderbuffer, PFNGLFRAMEBUFFERRENDERBUFFERPROC)

#define GL_FBO_BLIT_FUN \
	GL_FUN(BlitFramebuffer, PFNGLBLITFRAMEBUFFERPROC)

#define GL_VAO_FUN \
	/* Vertex array object */ \
	GL_FUN(GenVertexArrays, PFNGLGENVERTEXARRAYSPROC) \
	GL_FUN(DeleteVertexArrays, PFNGLDELETEVERTEXARRAYSPROC) \
	GL_FUN(BindVertexArray, PFNGLBINDVERTEXARRAYPROC)

#define GL_DEBUG_KHR_FUN \
	GL_FUN(DebugMessageCallback, PFNGLDEBUGMESSAGECALLBACKPROC)


struct GLFunctions
{
#define GL_FUN(name, type) type name;

	GL_20_FUN
	GL_FBO_FUN
	GL_FBO_BLIT_FUN
	GL_VAO_FUN
	GL_DEBUG_KHR_FUN

	bool glsles;
	bool unpack_subimage;
	bool npot_repeat;

#undef GL_FUN
};

extern GLFunctions gl;
void initGLFunctions();

#endif // GLFUN_H
