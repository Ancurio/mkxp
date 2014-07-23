/*
** vertex.cpp
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

#include "vertex.h"
#include "util.h"

#include <cstddef>

CVertex::CVertex()
    : color(1, 1, 1, 1)
{}

Vertex::Vertex()
    : color(1, 1, 1, 1)
{}

#define o(type, mem) ((const GLvoid*) offsetof(type, mem))

static const VertexAttribute SVertexAttribs[] =
{
	{ Shader::Position, 2, GL_FLOAT, o(SVertex, pos)    },
	{ Shader::TexCoord, 2, GL_FLOAT, o(SVertex, texPos) }
};

static const VertexAttribute CVertexAttribs[] =
{
	{ Shader::Color,    4, GL_FLOAT, o(CVertex, color) },
	{ Shader::Position, 2, GL_FLOAT, o(CVertex, pos)   }
};

static const VertexAttribute VertexAttribs[] =
{
	{ Shader::Color,    4, GL_FLOAT, o(Vertex, color)  },
	{ Shader::Position, 2, GL_FLOAT, o(Vertex, pos)    },
	{ Shader::TexCoord, 2, GL_FLOAT, o(Vertex, texPos) }
};

#define DEF_TRAITS(VertType) \
	template<> \
	const VertexAttribute *VertexTraits<VertType>::attr = VertType##Attribs; \
	template<> \
	const GLsizei VertexTraits<VertType>::attrCount = ARRAY_SIZE(VertType##Attribs)

DEF_TRAITS(SVertex);
DEF_TRAITS(CVertex);
DEF_TRAITS(Vertex);
