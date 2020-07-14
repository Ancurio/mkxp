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

#ifdef GLES2_HEADER
#include <SDL_opengles2.h>
#define APIENTRYP GL_APIENTRYP
#else
#include <SDL_opengl.h>
#endif

/* Etc */
typedef GLenum (APIENTRYP _PFNGLGETERRORPROC) (void);
typedef void (APIENTRYP _PFNGLCLEARCOLORPROC) (GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha);
typedef void (APIENTRYP _PFNGLCLEARPROC) (GLbitfield mask);
typedef const GLubyte * (APIENTRYP _PFNGLGETSTRINGPROC) (GLenum name);
typedef void (APIENTRYP _PFNGLGETINTEGERVPROC) (GLenum pname, GLint *params);
typedef void (APIENTRYP _PFNGLPIXELSTOREIPROC) (GLenum pname, GLint param);
typedef void (APIENTRYP _PFNGLREADPIXELSPROC) (GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid *pixels);
typedef void (APIENTRYP _PFNGLENABLEPROC) (GLenum cap);
typedef void (APIENTRYP _PFNGLDISABLEPROC) (GLenum cap);
typedef void (APIENTRYP _PFNGLSCISSORPROC) (GLint x, GLint y, GLsizei width, GLsizei height);
typedef void (APIENTRYP _PFNGLVIEWPORTPROC) (GLint x, GLint y, GLsizei width, GLsizei height);
typedef void (APIENTRYP _PFNGLBLENDFUNCPROC) (GLenum sfactor, GLenum dfactor);
typedef void (APIENTRYP _PFNGLBLENDFUNCSEPARATEPROC) (GLenum sfactorRGB, GLenum dfactorRGB, GLenum sfactorAlpha, GLenum dfactorAlpha);
typedef void (APIENTRYP _PFNGLBLENDEQUATIONPROC) (GLenum mode);
typedef void (APIENTRYP _PFNGLDRAWELEMENTSPROC) (GLenum mode, GLsizei count, GLenum type, const GLvoid *indices);

/* Texture */
typedef void (APIENTRYP _PFNGLGENTEXTURESPROC) (GLsizei n, GLuint *textures);
typedef void (APIENTRYP _PFNGLDELETETEXTURESPROC) (GLsizei n, const GLuint *textures);
typedef void (APIENTRYP _PFNGLBINDTEXTUREPROC) (GLenum target, GLuint texture);
typedef void (APIENTRYP _PFNGLTEXIMAGE2DPROC) (GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels);
typedef void (APIENTRYP _PFNGLTEXSUBIMAGE2DPROC) (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels);
typedef void (APIENTRYP _PFNGLTEXPARAMETERIPROC) (GLenum target, GLenum pname, GLint param);
typedef void (APIENTRYP _PFNGLACTIVETEXTUREPROC) (GLenum texture);

/* Debugging */
typedef void (APIENTRY * _GLDEBUGPROC) (GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void *userParam);
typedef void (APIENTRYP _PFNGLDEBUGMESSAGECALLBACKPROC) (_GLDEBUGPROC callback, const void *userParam);
typedef void (APIENTRYP _PFNGLSTRINGMARKERPROC) (GLsizei len, const GLvoid *string);

/* Buffer object */
typedef void (APIENTRYP _PFNGLGENBUFFERSPROC) (GLsizei n, GLuint* buffers);
typedef void (APIENTRYP _PFNGLDELETEBUFFERSPROC) (GLsizei n, const GLuint* buffers);
typedef void (APIENTRYP _PFNGLBINDBUFFERPROC) (GLenum target, GLuint buffer);
typedef void (APIENTRYP _PFNGLBUFFERDATAPROC) (GLenum target, GLsizeiptr size, const GLvoid* data, GLenum usage);
typedef void (APIENTRYP _PFNGLBUFFERSUBDATAPROC) (GLenum target, GLintptr offset, GLsizeiptr size, const GLvoid* data);

/* Shader */
typedef GLuint (APIENTRYP _PFNGLCREATESHADERPROC) (GLenum type);
typedef void (APIENTRYP _PFNGLDELETESHADERPROC) (GLuint shader);
typedef void (APIENTRYP _PFNGLSHADERSOURCEPROC) (GLuint shader, GLsizei count, const GLchar* const* strings, const GLint* lengths);
typedef void (APIENTRYP _PFNGLCOMPILESHADERPROC) (GLuint shader);
typedef void (APIENTRYP _PFNGLATTACHSHADERPROC) (GLuint program, GLuint shader);
typedef void (APIENTRYP _PFNGLGETSHADERIVPROC) (GLuint shader, GLenum pname, GLint* param);
typedef void (APIENTRYP _PFNGLGETSHADERINFOLOGPROC) (GLuint shader, GLsizei bufSize, GLsizei* length, GLchar* infoLog);

/* Program */
typedef GLuint (APIENTRYP _PFNGLCREATEPROGRAMPROC) (void);
typedef void (APIENTRYP _PFNGLDELETEPROGRAMPROC) (GLuint program);
typedef void (APIENTRYP _PFNGLUSEPROGRAMPROC) (GLuint program);
typedef void (APIENTRYP _PFNGLLINKPROGRAMPROC) (GLuint program);
typedef void (APIENTRYP _PFNGLGETPROGRAMIVPROC) (GLuint program, GLenum pname, GLint* param);
typedef void (APIENTRYP _PFNGLGETPROGRAMINFOLOGPROC) (GLuint program, GLsizei bufSize, GLsizei* length, GLchar* infoLog);

/* Uniform */
typedef GLint (APIENTRYP _PFNGLGETUNIFORMLOCATIONPROC) (GLuint program, const GLchar* name);
typedef void (APIENTRYP _PFNGLUNIFORM1FPROC) (GLint location, GLfloat v0);
typedef void (APIENTRYP _PFNGLUNIFORM2FPROC) (GLint location, GLfloat v0, GLfloat v1);
typedef void (APIENTRYP _PFNGLUNIFORM4FPROC) (GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3);
typedef void (APIENTRYP _PFNGLUNIFORM1IPROC) (GLint location, GLint v0);
typedef void (APIENTRYP _PFNGLUNIFORM1IVPROC) (GLint location, GLsizei count, const GLint *value);
typedef void (APIENTRYP _PFNGLUNIFORMMATRIX4FVPROC) (GLint location, GLsizei count, GLboolean transpose, const GLfloat* value);

/* Vertex attribute */
typedef void (APIENTRYP _PFNGLBINDATTRIBLOCATIONPROC) (GLuint program, GLuint index, const GLchar* name);
typedef void (APIENTRYP _PFNGLENABLEVERTEXATTRIBARRAYPROC) (GLuint);
typedef void (APIENTRYP _PFNGLDISABLEVERTEXATTRIBARRAYPROC) (GLuint);
typedef void (APIENTRYP _PFNGLVERTEXATTRIBPOINTERPROC) (GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid* pointer);

/* Framebuffer object */
typedef void (APIENTRYP _PFNGLGENFRAMEBUFFERSPROC) (GLsizei n, GLuint* framebuffers);
typedef void (APIENTRYP _PFNGLDELETEFRAMEBUFFERSPROC) (GLsizei n, const GLuint* framebuffers);
typedef void (APIENTRYP _PFNGLBINDFRAMEBUFFERPROC) (GLenum target, GLuint framebuffer);
typedef void (APIENTRYP _PFNGLFRAMEBUFFERTEXTURE2DPROC) (GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
typedef void (APIENTRYP _PFNGLBLITFRAMEBUFFERPROC) (GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter);

/* Vertex array object */
typedef void (APIENTRYP _PFNGLGENVERTEXARRAYSPROC) (GLsizei n, GLuint* arrays);
typedef void (APIENTRYP _PFNGLDELETEVERTEXARRAYSPROC) (GLsizei n, const GLuint* arrays);
typedef void (APIENTRYP _PFNGLBINDVERTEXARRAYPROC) (GLuint array);

/* GLES only */
typedef void (APIENTRYP _PFNGLRELEASESHADERCOMPILERPROC) (void);

#ifdef GLES2_HEADER
#define GL_NUM_EXTENSIONS 0x821D
#define GL_READ_FRAMEBUFFER 0x8CA8
#define GL_DRAW_FRAMEBUFFER 0x8CA9
#define GL_UNPACK_ROW_LENGTH 0x0CF2
#define GL_UNPACK_SKIP_PIXELS 0x0CF4
#define GL_UNPACK_SKIP_ROWS 0x0CF3
#endif

#define GL_20_FUN \
	/* Etc */ \
	GL_FUN(GetError, _PFNGLGETERRORPROC) \
	GL_FUN(ClearColor, _PFNGLCLEARCOLORPROC) \
	GL_FUN(Clear, _PFNGLCLEARPROC) \
	GL_FUN(GetString, _PFNGLGETSTRINGPROC) \
	GL_FUN(GetIntegerv, _PFNGLGETINTEGERVPROC) \
	GL_FUN(PixelStorei, _PFNGLPIXELSTOREIPROC) \
	GL_FUN(ReadPixels, _PFNGLREADPIXELSPROC) \
	GL_FUN(Enable, _PFNGLENABLEPROC) \
	GL_FUN(Disable, _PFNGLDISABLEPROC) \
	GL_FUN(Scissor, _PFNGLSCISSORPROC) \
	GL_FUN(Viewport, _PFNGLVIEWPORTPROC) \
	GL_FUN(BlendFunc, _PFNGLBLENDFUNCPROC) \
	GL_FUN(BlendFuncSeparate, _PFNGLBLENDFUNCSEPARATEPROC) \
	GL_FUN(BlendEquation, _PFNGLBLENDEQUATIONPROC) \
	GL_FUN(DrawElements, _PFNGLDRAWELEMENTSPROC) \
	/* Texture */ \
	GL_FUN(GenTextures, _PFNGLGENTEXTURESPROC) \
	GL_FUN(DeleteTextures, _PFNGLDELETETEXTURESPROC) \
	GL_FUN(BindTexture, _PFNGLBINDTEXTUREPROC) \
	GL_FUN(TexImage2D, _PFNGLTEXIMAGE2DPROC) \
	GL_FUN(TexSubImage2D, _PFNGLTEXSUBIMAGE2DPROC) \
	GL_FUN(TexParameteri, _PFNGLTEXPARAMETERIPROC) \
	GL_FUN(ActiveTexture, _PFNGLACTIVETEXTUREPROC) \
	/* Buffer object */ \
	GL_FUN(GenBuffers, _PFNGLGENBUFFERSPROC) \
	GL_FUN(DeleteBuffers, _PFNGLDELETEBUFFERSPROC) \
	GL_FUN(BindBuffer, _PFNGLBINDBUFFERPROC) \
	GL_FUN(BufferData, _PFNGLBUFFERDATAPROC) \
	GL_FUN(BufferSubData, _PFNGLBUFFERSUBDATAPROC) \
	/* Shader */ \
	GL_FUN(CreateShader, _PFNGLCREATESHADERPROC) \
	GL_FUN(DeleteShader, _PFNGLDELETESHADERPROC) \
	GL_FUN(ShaderSource, _PFNGLSHADERSOURCEPROC) \
	GL_FUN(CompileShader, _PFNGLCOMPILESHADERPROC) \
	GL_FUN(AttachShader, _PFNGLATTACHSHADERPROC) \
	GL_FUN(GetShaderiv, _PFNGLGETSHADERIVPROC) \
	GL_FUN(GetShaderInfoLog, _PFNGLGETSHADERINFOLOGPROC) \
	/* Program */ \
	GL_FUN(CreateProgram, _PFNGLCREATEPROGRAMPROC) \
	GL_FUN(DeleteProgram, _PFNGLDELETEPROGRAMPROC) \
	GL_FUN(UseProgram, _PFNGLUSEPROGRAMPROC) \
	GL_FUN(LinkProgram, _PFNGLLINKPROGRAMPROC) \
	GL_FUN(GetProgramiv, _PFNGLGETPROGRAMIVPROC) \
	GL_FUN(GetProgramInfoLog, _PFNGLGETPROGRAMINFOLOGPROC) \
	/* Uniform */ \
	GL_FUN(GetUniformLocation, _PFNGLGETUNIFORMLOCATIONPROC) \
	GL_FUN(Uniform1f, _PFNGLUNIFORM1FPROC) \
	GL_FUN(Uniform2f, _PFNGLUNIFORM2FPROC) \
	GL_FUN(Uniform4f, _PFNGLUNIFORM4FPROC) \
	GL_FUN(Uniform1i, _PFNGLUNIFORM1IPROC) \
	GL_FUN(Uniform1iv, _PFNGLUNIFORM1IVPROC) \
	GL_FUN(UniformMatrix4fv, _PFNGLUNIFORMMATRIX4FVPROC) \
	/* Vertex attribute */ \
	GL_FUN(BindAttribLocation, _PFNGLBINDATTRIBLOCATIONPROC) \
	GL_FUN(EnableVertexAttribArray, _PFNGLENABLEVERTEXATTRIBARRAYPROC) \
	GL_FUN(DisableVertexAttribArray, _PFNGLDISABLEVERTEXATTRIBARRAYPROC) \
	GL_FUN(VertexAttribPointer, _PFNGLVERTEXATTRIBPOINTERPROC)

#define GL_ES_FUN \
	GL_FUN(ReleaseShaderCompiler, _PFNGLRELEASESHADERCOMPILERPROC)

#define GL_FBO_FUN \
	/* Framebuffer object */ \
	GL_FUN(GenFramebuffers, _PFNGLGENFRAMEBUFFERSPROC) \
	GL_FUN(DeleteFramebuffers, _PFNGLDELETEFRAMEBUFFERSPROC) \
	GL_FUN(BindFramebuffer, _PFNGLBINDFRAMEBUFFERPROC) \
	GL_FUN(FramebufferTexture2D, _PFNGLFRAMEBUFFERTEXTURE2DPROC)

#define GL_FBO_BLIT_FUN \
	GL_FUN(BlitFramebuffer, _PFNGLBLITFRAMEBUFFERPROC)

#define GL_VAO_FUN \
	/* Vertex array object */ \
	GL_FUN(GenVertexArrays, _PFNGLGENVERTEXARRAYSPROC) \
	GL_FUN(DeleteVertexArrays, _PFNGLDELETEVERTEXARRAYSPROC) \
	GL_FUN(BindVertexArray, _PFNGLBINDVERTEXARRAYPROC)

#define GL_DEBUG_KHR_FUN \
	GL_FUN(DebugMessageCallback, _PFNGLDEBUGMESSAGECALLBACKPROC)

#define GL_GREMEMDY_FUN \
	GL_FUN(StringMarker, _PFNGLSTRINGMARKERPROC)


struct GLFunctions
{
#define GL_FUN(name, type) type name;

	GL_20_FUN
	GL_ES_FUN
	GL_FBO_FUN
	GL_FBO_BLIT_FUN
	GL_VAO_FUN
	GL_DEBUG_KHR_FUN
	GL_GREMEMDY_FUN

	bool glsles;
	bool unpack_subimage;
	bool npot_repeat;

#undef GL_FUN
};

extern GLFunctions gl;
void initGLFunctions();

#endif // GLFUN_H
