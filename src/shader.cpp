/*
** shader.cpp
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

#include "shader.h"

#include "GL/glew.h"

#include <QFile>

#include "../shader/sprite.frag.xxd"
#include "../shader/hue.frag.xxd"
#include "../shader/trans.frag.xxd"
#include "../shader/transSimple.frag.xxd"
#include "../shader/bitmapBlit.frag.xxd"

#define GET_U(name) u_##name = glGetUniformLocation(program, #name)

FragShader::~FragShader()
{
	glUseProgram(0);
	glDeleteProgram(program);
	glDeleteShader(shader);
}

void FragShader::bind()
{
	glUseProgram(program);
}

void FragShader::unbind()
{
	glActiveTexture(GL_TEXTURE0);
	glUseProgram(0);
}

void FragShader::init(const unsigned char *source, int length)
{
	GLint success;

	shader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(shader, 1, (const GLchar**) &source, (const GLint*) &length);
	glCompileShader(shader);

	glGetObjectParameterivARB(shader, GL_COMPILE_STATUS, &success);
	Q_ASSERT(success);

	program = glCreateProgram();
	glAttachShader(program, shader);
	glLinkProgram(program);

	glGetObjectParameterivARB(program, GL_LINK_STATUS, &success);
	Q_ASSERT(success);
}

void FragShader::initFromFile(const char *filename)
{
	QFile shaderFile(filename);
	shaderFile.open(QFile::ReadOnly);
	QByteArray contents = shaderFile.readAll();
	shaderFile.close();

	init((const unsigned char*) contents.constData(), contents.size());
}

void FragShader::setVec4Uniform(GLint location, const Vec4 &vec)
{
	glUniform4f(location, vec.x, vec.y, vec.z, vec.w);
}

void FragShader::setTexUniform(GLint location, unsigned unitIndex, TEX::ID texture)
{
	GLenum texUnit = GL_TEXTURE0 + unitIndex;

	glActiveTexture(texUnit);
	glBindTexture(GL_TEXTURE_2D, texture.gl);
	glUniform1i(location, unitIndex);
	glActiveTexture(GL_TEXTURE0);
}


TransShader::TransShader()
{
	init(shader_trans_frag,
	     shader_trans_frag_len);

	GET_U(currentScene);
	GET_U(frozenScene);
	GET_U(transMap);
	GET_U(prog);
	GET_U(vague);
}

void TransShader::setCurrentScene(TEX::ID tex)
{
	setTexUniform(u_currentScene, 0, tex);
}

void TransShader::setFrozenScene(TEX::ID tex)
{
	setTexUniform(u_frozenScene, 1, tex);
}

void TransShader::setTransMap(TEX::ID tex)
{
	setTexUniform(u_transMap, 2, tex);
}

void TransShader::setProg(float value)
{
	glUniform1f(u_prog, value);
}

void TransShader::setVague(float value)
{
	glUniform1f(u_vague, value);
}


SimpleTransShader::SimpleTransShader()
{
	init(shader_transSimple_frag,
	     shader_transSimple_frag_len);

	GET_U(currentScene);
	GET_U(frozenScene);
	GET_U(prog);
}

void SimpleTransShader::setCurrentScene(TEX::ID tex)
{
	setTexUniform(u_currentScene, 0, tex);
}

void SimpleTransShader::setFrozenScene(TEX::ID tex)
{
	setTexUniform(u_frozenScene, 1, tex);
}

void SimpleTransShader::setProg(float value)
{
	glUniform1f(u_prog, value);
}


SpriteShader::SpriteShader()
{
	init(shader_sprite_frag,
	     shader_sprite_frag_len);

	GET_U(tone);
	GET_U(color);
	GET_U(flash);
	GET_U(opacity);
	GET_U(bushDepth);
	GET_U(bushOpacity);

	bind();
	resetUniforms();
	unbind();
}

void SpriteShader::resetUniforms()
{
	setTone(Vec4());
	setColor(Vec4());
	setFlash(Vec4());
	setOpacity(1);
	setBushDepth(0);
	setBushOpacity(0.5);
}

void SpriteShader::setTone(const Vec4 &tone)
{
	setVec4Uniform(u_tone, tone);
}

void SpriteShader::setColor(const Vec4 &color)
{
	setVec4Uniform(u_color, color);
}

void SpriteShader::setFlash(const Vec4 &flash)
{
	setVec4Uniform(u_flash, flash);
}

void SpriteShader::setOpacity(float value)
{
	glUniform1f(u_opacity, value);
}

void SpriteShader::setBushDepth(float value)
{
	glUniform1f(u_bushDepth, value);
}

void SpriteShader::setBushOpacity(float value)
{
	glUniform1f(u_bushOpacity, value);
}


HueShader::HueShader()
{
	init(shader_hue_frag,
	     shader_hue_frag_len);

	GET_U(hueAdjust);
	GET_U(inputTexture);
}

void HueShader::setHueAdjust(float value)
{
	glUniform1f(u_hueAdjust, value);
}

void HueShader::setInputTexture(TEX::ID tex)
{
	setTexUniform(u_inputTexture, 0, tex);
}


BltShader::BltShader()
{
	init(shader_bitmapBlit_frag,
	     shader_bitmapBlit_frag_len);

	GET_U(source);
	GET_U(destination);
	GET_U(subRect);
	GET_U(opacity);
}

void BltShader::setSource()
{
	glUniform1i(u_source, 0);
}

void BltShader::setDestination(const TEX::ID value)
{
	setTexUniform(u_destination, 1, value);
}

void BltShader::setSubRect(const FloatRect &value)
{
	glUniform4f(u_subRect, value.x, value.y, value.w, value.h);
}

void BltShader::setOpacity(float value)
{
	glUniform1f(u_opacity, value);
}
