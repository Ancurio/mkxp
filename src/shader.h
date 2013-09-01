/*
** shader.h
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

#ifndef SHADER_H
#define SHADER_H

#include "etc-internal.h"
#include "gl-util.h"

class FragShader
{
public:
	void bind();
	static void unbind();

protected:
	~FragShader();

	void init(const unsigned char *source, int length);
	void initFromFile(const char *filename);

	void setVec4Uniform(GLint location, const Vec4 &vec);
	void setTexUniform(GLint location, unsigned unitIndex, Tex::ID texture);

	GLuint shader;
	GLuint program;
};

class TransShader : public FragShader
{
public:
	TransShader();

	void setCurrentScene(Tex::ID tex);
	void setFrozenScene(Tex::ID tex);
	void setTransMap(Tex::ID tex);
	void setProg(float value);
	void setVague(float value);

private:
	GLint u_currentScene, u_frozenScene, u_transMap, u_prog, u_vague;
};

class SimpleTransShader : public FragShader
{
public:
	SimpleTransShader();

	void setCurrentScene(Tex::ID tex);
	void setFrozenScene(Tex::ID tex);
	void setProg(float value);

private:
	GLint u_currentScene, u_frozenScene, u_prog;
};

class SpriteShader : public FragShader
{
public:
	SpriteShader();

	void resetUniforms();
	void setTone(const Vec4 &value);
	void setColor(const Vec4 &value);
	void setFlash(const Vec4 &value);
	void setOpacity(float value);
	void setBushDepth(float value);
	void setBushOpacity(float value);

private:
	GLint u_tone, u_opacity, u_color, u_flash, u_bushDepth, u_bushOpacity;
};

class HueShader : public FragShader
{
public:
	HueShader();

	void setHueAdjust(float value);
	void setInputTexture(Tex::ID tex);

private:
	GLint u_hueAdjust, u_inputTexture;
};

/* Bitmap blit */
class BltShader : public FragShader
{
public:
	BltShader();

	void setSource();
	void setDestination(const Tex::ID value);
	void setDestCoorF(const Vec2 &value);
	void setSubRect(const FloatRect &value);
	void setOpacity(float value);

private:
	GLint u_source, u_destination, u_subRect, u_opacity;
};

#endif // SHADER_H
