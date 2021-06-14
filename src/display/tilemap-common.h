/*
** tilemap-common.h
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

#ifndef TILEMAPCOMMON_H
#define TILEMAPCOMMON_H

#include "table.h"
#include "gl-util.h"
#include "gl-meta.h"
#include "sharedstate.h"
#include "global-ibo.h"
#include "glstate.h"
#include "shader.h"
#include "vertex.h"
#include "quad.h"
#include "etc-internal.h"

#include <stdint.h>
#include <assert.h>
#include <vector>

#include "sigslot/signal.hpp"

static inline int
wrap(int value, int range)
{
	int res = value % range;
	return res < 0 ? res + range : res;
}

static inline Vec2i
wrap(const Vec2i &value, int range)
{
	return Vec2i(wrap(value.x, range),
	             wrap(value.y, range));
}

static inline int16_t
tableGetWrapped(const Table &t, int x, int y, int z = 0)
{
	return t.get(wrap(x, t.xSize()),
	             wrap(y, t.ySize()),
	             z);
}

/* Calculate the tile x/y on which this pixel x/y lies */
static inline Vec2i
getTilePos(const Vec2i &pixelPos)
{
	/* Round the pixel position down to the nearest top left
	 * tile boundary, by masking off the lower 5 bits (2^5 = 32).
	 * Then divide by 32 to convert into tile units. */
	return (pixelPos & ~(32-1)) / 32;
}

enum AtSubPos
{
	TopLeft          = 0,
	TopRight         = 1,
	BottomLeft       = 2,
	BottomRight      = 3,
	BottomLeftTable  = 4,
	BottomRightTable = 5
};

static inline void
atSelectSubPos(FloatRect &pos, int i)
{
	switch (i)
	{
	case TopLeft:
		return;
	case TopRight:
		pos.x += 16;
		return;
	case BottomLeft:
		pos.y += 16;
		return;
	case BottomRight:
		pos.x += 16;
		pos.y += 16;
		return;
	case BottomLeftTable:
		pos.y += 24;
		return;
	case BottomRightTable:
		pos.x += 16;
		pos.y += 24;
		return;
	default:
		assert(!"Unreachable");
	}
}

struct FlashMap
{
	FlashMap()
		: dirty(false),
	      data(0),
	      allocQuads(0)
	{
		vao.vbo = VBO::gen();
		vao.ibo = shState->globalIBO().ibo;
		GLMeta::vaoFillInVertexData<CVertex>(vao);

		GLMeta::vaoInit(vao);
	}

	~FlashMap()
	{
		GLMeta::vaoFini(vao);
		VBO::del(vao.vbo);
		dataCon.disconnect();
	}

	Table *getData() const
	{
		return data;
	}

	void setData(Table *value)
	{
		if (data == value)
			return;

		data = value;
		dataCon.disconnect();
		dirty = true;

		if (!data)
			return;

        
        
        dataCon = data->modified.connect(&FlashMap::setDirty, this);
	}

	void setViewport(const IntRect &value)
	{
		viewp = value;
		dirty = true;
	}

	void prepare()
	{
		if (!dirty)
			return;

		rebuildBuffer();
		dirty = false;
	}

	void draw(float alpha, const Vec2i &trans)
	{
		const size_t count = quadCount();

		if (count == 0)
			return;

		GLMeta::vaoBind(vao);
		glState.blendMode.pushSet(BlendAddition);

		FlashMapShader &shader = shState->shaders().flashMap;
		shader.bind();
		shader.applyViewportProj();
		shader.setAlpha(alpha);
		shader.setTranslation(trans);

		gl.DrawElements(GL_TRIANGLES, count * 6, _GL_INDEX_TYPE, 0);

		glState.blendMode.pop();

		GLMeta::vaoUnbind(vao);
	}

private:
	void setDirty()
	{
		dirty = true;
	}

	size_t quadCount() const
	{
		return vertices.size() / 4;
	}

	bool sampleFlashColor(Vec4 &out, int x, int y) const
	{
		int16_t packed = tableGetWrapped(*data, x, y);

		if (packed == 0)
			return false;

		const float max = 0xF;

		float b = ((packed & 0x000F) >> 0) / max;
		float g = ((packed & 0x00F0) >> 4) / max;
		float r = ((packed & 0x0F00) >> 8) / max;

		out = Vec4(r, g, b, 1);

		return true;
	}

	void rebuildBuffer()
	{
		vertices.clear();

		if (!data)
			return;

		for (int x = 0; x < viewp.w; ++x)
			for (int y = 0; y < viewp.h; ++y)
			{
				Vec4 color;

				if (!sampleFlashColor(color, x+viewp.x, y+viewp.y))
					continue;

				FloatRect posRect(x*32, y*32, 32, 32);

				CVertex v[4];
				Quad::setPosRect(v, posRect);
				Quad::setColor(v, color);

				for (size_t i = 0; i < 4; ++i)
					vertices.push_back(v[i]);
			}

		if (vertices.size() == 0)
			return;

		VBO::bind(vao.vbo);

		if (quadCount() > allocQuads)
		{
			allocQuads = quadCount();
			VBO::allocEmpty(sizeof(CVertex) * vertices.size());
		}

		VBO::uploadSubData(0, sizeof(CVertex) * vertices.size(), dataPtr(vertices));

		VBO::unbind();

		/* Ensure global IBO size */
		shState->ensureQuadIBO(quadCount());
	}

	bool dirty;

	Table *data;
	sigslot::connection dataCon;

	IntRect viewp;

	GLMeta::VAO vao;
	size_t allocQuads;
	std::vector<CVertex> vertices;
};

#endif // TILEMAPCOMMON_H
