/*
** quad.h
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

#ifndef QUAD_H
#define QUAD_H

#include "GL/glew.h"
#include "etc-internal.h"
#include "gl-util.h"
#include "sharedstate.h"
#include "global-ibo.h"
#include "shader.h"

struct Quad
{
	Vertex vert[4];
	VBO::ID vbo;
	VAO::ID vao;
	bool vboDirty;

	static void setPosRect(CVertex *vert, const FloatRect &r)
	{
		int i = 0;
		vert[i++].pos = r.topLeft();
		vert[i++].pos = r.topRight();
		vert[i++].pos = r.bottomRight();
		vert[i++].pos = r.bottomLeft();
	}

	static void setColor(CVertex *vert, const Vec4 &c)
	{
		for (int i = 0; i < 4; ++i)
			vert[i].color = c;
	}

	static void setPosRect(SVertex *vert, const FloatRect &r)
	{
		int i = 0;
		vert[i++].pos = r.topLeft();
		vert[i++].pos = r.topRight();
		vert[i++].pos = r.bottomRight();
		vert[i++].pos = r.bottomLeft();
	}

	static void setTexRect(SVertex *vert, const FloatRect &r)
	{
		int i = 0;
		vert[i++].texPos = r.topLeft();
		vert[i++].texPos = r.topRight();
		vert[i++].texPos = r.bottomRight();
		vert[i++].texPos = r.bottomLeft();
	}

	static void setPosRect(Vertex *vert, const FloatRect &r)
	{
		int i = 0;
		vert[i++].pos = r.topLeft();
		vert[i++].pos = r.topRight();
		vert[i++].pos = r.bottomRight();
		vert[i++].pos = r.bottomLeft();
	}

	static void setTexRect(Vertex *vert, const FloatRect &r)
	{
		int i = 0;
		vert[i++].texPos = r.topLeft();
		vert[i++].texPos = r.topRight();
		vert[i++].texPos = r.bottomRight();
		vert[i++].texPos = r.bottomLeft();
	}

	static int setTexPosRect(SVertex *vert, const FloatRect &tex, const FloatRect &pos)
	{
		setPosRect(vert, pos);
		setTexRect(vert, tex);

		return 1;
	}

	static int setTexPosRect(Vertex *vert, const FloatRect &tex, const FloatRect &pos)
	{
		setPosRect(vert, pos);
		setTexRect(vert, tex);

		return 1;
	}

	Quad()
	    : vbo(VBO::gen()),
	      vao(VAO::gen()),
	      vboDirty(true)
	{
		VAO::bind(vao);
		VBO::bind(vbo);
		shState->bindQuadIBO();

		glEnableVertexAttribArray(Shader::Color);
		glEnableVertexAttribArray(Shader::Position);
		glEnableVertexAttribArray(Shader::TexCoord);

		glVertexAttribPointer(Shader::Color,    4, GL_FLOAT, GL_FALSE, sizeof(Vertex), Vertex::colorOffset());
		glVertexAttribPointer(Shader::Position, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), Vertex::posOffset());
		glVertexAttribPointer(Shader::TexCoord, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), Vertex::texPosOffset());

		VAO::unbind();
		VBO::unbind();
		IBO::unbind();

		setColor(Vec4(1, 1, 1, 1));
	}

	~Quad()
	{
		VAO::del(vao);
		VBO::del(vbo);
	}

	void updateBuffer()
	{
		VBO::bind(vbo);
		VBO::allocEmpty(sizeof(Vertex) * 4, GL_DYNAMIC_DRAW);
		VBO::uploadData(sizeof(Vertex) * 4, vert, GL_DYNAMIC_DRAW);
	}

	void setPosRect(const FloatRect &r)
	{
		setPosRect(vert, r);
		vboDirty = true;
	}

	void setTexRect(const FloatRect &r)
	{
		setTexRect(vert, r);
		vboDirty = true;
	}

	void setTexPosRect(const FloatRect &tex, const FloatRect &pos)
	{
		setTexPosRect(vert, tex, pos);
		vboDirty = true;
	}

	void setColor(const Vec4 &c)
	{
		for (int i = 0; i < 4; ++i)
			vert[i].color = c;

		vboDirty = true;
	}

	void draw()
	{
		if (vboDirty)
		{
			updateBuffer();
			vboDirty = false;
		}

		VAO::bind(vao);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
		VAO::unbind();
	}
};

#endif // QUAD_H
