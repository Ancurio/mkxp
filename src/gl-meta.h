/*
** gl-meta.h
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

#ifndef GLMETA_H
#define GLMETA_H

#include "gl-fun.h"
#include "gl-util.h"

#include <SDL_surface.h>

namespace GLMeta
{

inline void subRectImageUpload(GLint srcW, GLint srcX, GLint srcY,
                               GLint dstX, GLint dstY, GLsizei dstW, GLsizei dstH,
                               SDL_Surface *src, GLenum format)
{
	if (gl.unpack_subimage)
	{
		PixelStore::setupSubImage(srcW, srcX, srcY);
		TEX::uploadSubImage(dstX, dstY, dstW, dstH, src->pixels, format);
	}
	else
	{
		SDL_PixelFormat *form = src->format;
		SDL_Surface *tmp = SDL_CreateRGBSurface(0, dstW, dstH, form->BitsPerPixel,
		                                        form->Rmask, form->Gmask, form->Bmask, form->Amask);
		SDL_Rect srcRect = { srcX, srcY, dstW, dstH };

		SDL_BlitSurface(src, &srcRect, tmp, 0);

		TEX::uploadSubImage(dstX, dstY, dstW, dstH, tmp->pixels, format);

		SDL_FreeSurface(tmp);
	}
}

inline void subRectImageFinish()
{
	if (gl.unpack_subimage)
		PixelStore::reset();
}

}

#endif // GLMETA_H
