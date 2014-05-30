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

/* A small hack to get mutable QuadArray constructors */
inline void initBufferBindings(Vertex *)
{
	gl.EnableVertexAttribArray(Shader::Color);
	gl.EnableVertexAttribArray(Shader::Position);
	gl.EnableVertexAttribArray(Shader::TexCoord);

	gl.VertexAttribPointer(Shader::Color,    4, GL_FLOAT, GL_FALSE, sizeof(Vertex), Vertex::colorOffset());
	gl.VertexAttribPointer(Shader::Position, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), Vertex::posOffset());
	gl.VertexAttribPointer(Shader::TexCoord, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), Vertex::texPosOffset());
}

inline void initBufferBindings(SVertex *)
{
	gl.EnableVertexAttribArray(Shader::Position);
	gl.EnableVertexAttribArray(Shader::TexCoord);

	gl.VertexAttribPointer(Shader::Position, 2, GL_FLOAT, GL_FALSE, sizeof(SVertex), SVertex::posOffset());
	gl.VertexAttribPointer(Shader::TexCoord, 2, GL_FLOAT, GL_FALSE, sizeof(SVertex), SVertex::texPosOffset());
}

template<class VertexType>
struct QuadArray
{
	std::vector<VertexType> vertices;

	VBO::ID vbo;
	VAO::ID vao;

	int quadCount;
	GLsizeiptr vboSize;

	QuadArray()
	    : quadCount(0),
	      vboSize(-1)
	{
		vbo = VBO::gen();
		vao = VAO::gen();

		VAO::bind(vao);
		VBO::bind(vbo);
		shState->bindQuadIBO();

		/* Call correct implementation here via overloading */
		VertexType *dummy = 0;
		initBufferBindings(dummy);

		VAO::unbind();
		IBO::unbind();
		VBO::unbind();
	}

	~QuadArray()
	{
		VBO::del(vbo);
		VAO::del(vao);
	}

	void resize(int size)
	{
		vertices.resize(size * 4);
		quadCount = size;
	}

	void clear()
	{
		vertices.clear();
		quadCount = 0;
	}

	/* This needs to be called after the final 'append()' call
	 * and previous to the first 'draw()' call. */
	void commit()
	{
		VBO::bind(vbo);

		GLsizeiptr size = vertices.size() * sizeof(VertexType);

		if (size > vboSize)
		{
			/* New data exceeds already allocated size.
			 * Reallocate VBO. */
			VBO::uploadData(size, &vertices[0], GL_DYNAMIC_DRAW);
			vboSize = size;

			shState->ensureQuadIBO(quadCount);
		}
		else
		{
			/* New data fits in allocated size */
			VBO::uploadSubData(0, size, &vertices[0]);
		}

		VBO::unbind();
	}

	void draw(size_t offset, size_t count)
	{
		VAO::bind(vao);

		const char *_offset = (const char*) 0 + offset * 6 * sizeof(index_t);
		gl.DrawElements(GL_TRIANGLES, count * 6, _GL_INDEX_TYPE, _offset);

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

typedef QuadArray<Vertex> ColorQuadArray;
typedef QuadArray<SVertex> SimpleQuadArray;

#endif // QUADARRAY_H
