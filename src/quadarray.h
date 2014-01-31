/*
** quadarray.h
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

#ifndef QUADARRAY_H
#define QUADARRAY_H

#include "gl-util.h"
#include "sharedstate.h"
#include "global-ibo.h"
#include "shader.h"

#include <vector>

#include <stdint.h>

typedef uint32_t index_t;
#define _GL_INDEX_TYPE GL_UNSIGNED_INT

struct ColorQuadArray
{
	std::vector<Vertex> vertices;

	VBO::ID vbo;
	VAO::ID vao;

	int quadCount;

	ColorQuadArray()
	    : quadCount(0)
	{
		vbo = VBO::gen();
		vao = VAO::gen();

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
		IBO::unbind();
		VBO::unbind();
	}

	~ColorQuadArray()
	{
		VBO::del(vbo);
		VAO::del(vao);
	}

	void resize(int size)
	{
		vertices.resize(size * 4);
		quadCount = size;
	}

	/* This needs to be called after the final 'append()' call
	 * and previous to the first 'draw()' call. */
	void commit()
	{
		VBO::bind(vbo);
		VBO::uploadData(vertices.size() * sizeof(Vertex), &vertices[0], GL_DYNAMIC_DRAW);
		VBO::unbind();

		shState->ensureQuadIBO(quadCount);
	}

	void draw(size_t offset, size_t count)
	{
		VAO::bind(vao);

		const char *_offset = (const char*) 0 + offset * 6 * sizeof(index_t);
		glDrawElements(GL_TRIANGLES, count * 6, _GL_INDEX_TYPE, _offset);

		VAO::unbind();
	}

	void draw()
	{
		draw(0, quadCount);
	}

	int count() const
	{
		return quadCount;
	}
};

#endif // QUADARRAY_H
