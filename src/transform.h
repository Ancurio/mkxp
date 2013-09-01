/*
 *  transform.h
 *
 *  This file is part of mkxp. It is based on parts of "SFML 2.0"
 */

////////////////////////////////////////////////////////////
//
// SFML - Simple and Fast Multimedia Library
// Copyright (C) 2007-2012 Laurent Gomila (laurent.gom@gmail.com)
//
// This software is provided 'as-is', without any express or implied warranty.
// In no event will the authors be held liable for any damages arising from the use of this software.
//
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it freely,
// subject to the following restrictions:
//
// 1. The origin of this software must not be misrepresented;
//    you must not claim that you wrote the original software.
//    If you use this software in a product, an acknowledgment
//    in the product documentation would be appreciated but is not required.
//
// 2. Altered source versions must be plainly marked as such,
//    and must not be misrepresented as being the original software.
//
// 3. This notice may not be removed or altered from any source distribution.
//
////////////////////////////////////////////////////////////


#ifndef TRANSFORM_H
#define TRANSFORM_H

#include "etc-internal.h"
#include "math.h"
#include "string.h"

class Transform
{
public:
	Transform()
	    : scale(1, 1),
	      rotation(0),
	      xOffset(0), yOffset(0),
	      dirty(true)
	{
		memset(matrix, 0, sizeof(matrix));

		matrix[10] = 1;
		matrix[15] = 1;
	}

	Vec2 &getPosition() { return position; }
	Vec2 &getScale()    { return scale;    }
	Vec2 &getOrigin()   { return origin;   }
	float getRotation() { return rotation; }

	void setPosition(const Vec2 &value)
	{
		position = value;
		dirty = true;
	}

	void setScale(const Vec2 &value)
	{
		scale = value;
		dirty = true;
	}

	void setOrigin(const Vec2 &value)
	{
		origin = value;
		dirty = true;
	}

	void setRotation(float value)
	{
		rotation = value;
		dirty = true;
	}

	void setGlobalOffset(int x, int y)
	{
		xOffset = x;
		yOffset = y;
		dirty = true;
	}

	const float *getMatrix()
	{
		if (dirty)
		{
			updateMatrix();
			dirty = false;
		}

		return matrix;
	}

private:
	void updateMatrix()
	{
		if (rotation >= 360 || rotation < -360)
			rotation = (float) fmod(rotation, 360);
		if (rotation < 0)
			rotation += 360;

		float angle  = rotation * 3.141592654f / 180.f;
		float cosine = (float) cos(angle);
		float sine   = (float) sin(angle);
		float sxc    = scale.x * cosine;
		float syc    = scale.y * cosine;
		float sxs    = scale.x * sine;
		float sys    = scale.y * sine;
		float tx     = -origin.x * sxc - origin.y * sys + position.x + xOffset;
		float ty     =  origin.x * sxs - origin.y * syc + position.y + yOffset;

		matrix[0]  =  sxc;
		matrix[1]  = -sxs;
		matrix[4]  =  sys;
		matrix[5]  =  syc;
		matrix[12] =  tx;
		matrix[13] =  ty;
	}

	Vec2 position;
	Vec2 origin;
	Vec2 scale;
	float rotation;

	/* Silently added to position */
	int xOffset, yOffset;

	float matrix[16];

	bool dirty;
};

#endif // TRANSFORM_H
