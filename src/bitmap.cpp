/*
** bitmap.cpp
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

#include "bitmap.h"

#include "SDL2/SDL.h"
#include "SDL2/SDL_image.h"
#include "SDL2/SDL_ttf.h"

#include "gl-util.h"
#include "quad.h"
#include "quadarray.h"
#include "exception.h"

#include "globalstate.h"
#include "glstate.h"
#include "texpool.h"
#include "shader.h"
#include "filesystem.h"
#include "font.h"
#include "eventthread.h"

#define DISP_CLASS_NAME "bitmap"

struct BitmapPrivate
{
	TexFBO tex;

	/* 'setPixel()' calls are cached and executed
	 * in batches on 'flush()' */
	PointArray pointArray;

	Font *font;

	BitmapPrivate()
	{
		font = &gState->defaultFont();
	}

	void bindTextureWithMatrix()
	{
		Tex::bind(tex.tex);
		Tex::bindMatrix(tex.width, tex.height);
	}

	void bindFBO()
	{
		FBO::bind(tex.fbo);
	}

	void pushSetViewport() const
	{
		glState.pushSetViewport(tex.width, tex.height);
	}

	void popViewport() const
	{
		glState.popViewport();
	}

	void blitQuad(Quad &quad)
	{
		glState.blendMode.pushSet(BlendNone);
		quad.draw();
		glState.blendMode.pop();
	}

	void flushPoints()
	{
		if (pointArray.count() == 0)
			return;

		Tex::unbind();
		bindFBO();
		pushSetViewport();
		glState.blendMode.pushSet(BlendNone);

		pointArray.commit();
		pointArray.draw();
		pointArray.reset();

		glState.blendMode.pop();
		popViewport();
	}

	void fillRect(const IntRect &rect,
	              const Vec4 &color)
	{
		flushPoints();

		bindFBO();

		glState.scissorTest.pushSet(true);
		glState.scissorBox.pushSet(rect);
		glState.clearColor.pushSet(color);

		glClear(GL_COLOR_BUFFER_BIT);

		glState.clearColor.pop();
		glState.scissorBox.pop();
		glState.scissorTest.pop();
	}

	static void ensureFormat(SDL_Surface *&surf, Uint32 format)
	{
		if (surf->format->format == format)
			return;

		SDL_Surface *surfConv = SDL_ConvertSurfaceFormat(surf, format, 0);
		SDL_FreeSurface(surf);
		surf = surfConv;
	}
};

Bitmap::Bitmap(const char *filename)
{
	SDL_RWops ops;
	gState->fileSystem().openRead(ops, filename, FileSystem::Image);
	SDL_Surface *imgSurf = IMG_Load_RW(&ops, 1);

	if (!imgSurf)
		throw Exception(Exception::SDLError, "SDL: %s", SDL_GetError());

	p = new BitmapPrivate;

	p->ensureFormat(imgSurf, SDL_PIXELFORMAT_ABGR8888);

	p->tex = gState->texPool().request(imgSurf->w, imgSurf->h);

	Tex::bind(p->tex.tex);
	Tex::uploadImage(p->tex.width, p->tex.height, imgSurf->pixels, GL_RGBA);

	SDL_FreeSurface(imgSurf);
}

Bitmap::Bitmap(int width, int height)
{
	p = new BitmapPrivate;

	p->tex = gState->texPool().request(width, height);

	clear();
}

Bitmap::Bitmap(const Bitmap &other)
{
	p = new BitmapPrivate;

	p->tex = gState->texPool().request(other.width(), other.height());

	other.flush();
	blt(0, 0, other, rect());
}

Bitmap::~Bitmap()
{
	dispose();
}

int Bitmap::width() const
{
	GUARD_DISPOSED

	return p->tex.width;
}

int Bitmap::height() const
{
	GUARD_DISPOSED

	return p->tex.height;
}

IntRect Bitmap::rect() const
{
	return IntRect(0, 0, width(), height());
}

void Bitmap::blt(int x, int y,
                  const Bitmap &source, const IntRect &rect,
                  int opacity)
{
	stretchBlt(IntRect(x, y, rect.w, rect.h),
	           source, rect, opacity);
}

void Bitmap::stretchBlt(const IntRect &destRect,
                         const Bitmap &source, const IntRect &sourceRect,
                         int opacity)
{
	GUARD_DISPOSED;

	opacity = bound(opacity, 0, 255);

	if (opacity == 0)
	{
		return;
	}
//	else if (opacity == 255) /* Fast blit */
//	{
//		flush();

//		FBO::bind(source.p->tex.fbo, FBO::Read);
//		FBO::bind(p->tex.fbo, FBO::Draw);

//		FBO::blit(sourceRect.x, sourceRect.y, sourceRect.w, sourceRect.h,
//		          destRect.x,   destRect.y,   destRect.w,   destRect.h);
//	}
	else /* Fragment pipeline */
	{
		flush();

		float normOpacity = (float) opacity / 255.0f;

		TexFBO &gpTex = gState->gpTexFBO(destRect.w, destRect.h);

		FBO::bind(gpTex.fbo, FBO::Draw);
		FBO::bind(p->tex.fbo, FBO::Read);
		FBO::blit(destRect.x, destRect.y, 0, 0, destRect.w, destRect.h);

		FloatRect bltSubRect((float) sourceRect.x / source.width(),
		                     (float) sourceRect.y / source.height(),
		                     ((float) source.width() / sourceRect.w) * ((float) destRect.w / gpTex.width),
		                     ((float) source.height() / sourceRect.h) * ((float) destRect.h / gpTex.height));

		BltShader &shader = gState->bltShader();
		shader.bind();
		shader.setDestination(gpTex.tex);
		shader.setSubRect(bltSubRect);
		shader.setOpacity(normOpacity);

		Quad quad;
		quad.setTexPosRect(sourceRect, destRect);
		quad.setColor(Vec4(1, 1, 1, normOpacity));

		source.p->bindTextureWithMatrix();
		p->bindFBO();
		p->pushSetViewport();

		p->blitQuad(quad);

		p->popViewport();
		shader.unbind();
	}

	modified();
}

void Bitmap::fillRect(int x, int y,
                      int width, int height,
                      const Vec4 &color)
{
	fillRect(IntRect(x, y, width, height), color);
}

void Bitmap::fillRect(const IntRect &rect, const Vec4 &color)
{
	GUARD_DISPOSED

	p->fillRect(rect, color);

	modified();
}

void Bitmap::gradientFillRect(int x, int y,
                              int width, int height,
                              const Vec4 &color1, const Vec4 &color2,
                              bool vertical)
{
	gradientFillRect(IntRect(x, y, width, height), color1, color2, vertical);
}

void Bitmap::gradientFillRect(const IntRect &rect,
                              const Vec4 &color1, const Vec4 &color2,
                              bool vertical)
{
	GUARD_DISPOSED

	flush();

	Quad quad;

	if (vertical)
	{
		quad.vert[0].color = color2;
		quad.vert[1].color = color2;
		quad.vert[2].color = color1;
		quad.vert[3].color = color1;
	}
	else
	{
		quad.vert[0].color = color1;
		quad.vert[3].color = color1;
		quad.vert[1].color = color2;
		quad.vert[2].color = color2;
	}

	quad.setPosRect(rect);

	Tex::unbind();
	p->bindFBO();
	p->pushSetViewport();

	p->blitQuad(quad);

	p->popViewport();

	modified();
}

void Bitmap::clearRect(int x, int y, int width, int height)
{
	clearRect(IntRect(x, y, width, height));
}

void Bitmap::clearRect(const IntRect &rect)
{
	GUARD_DISPOSED

	p->fillRect(rect, Vec4());

	modified();
}

void Bitmap::clear()
{
	GUARD_DISPOSED

	/* Any queued points won't be visible after this anyway */
	p->pointArray.reset();

	p->bindFBO();

	glState.clearColor.pushSet(Vec4());

	glClear(GL_COLOR_BUFFER_BIT);

	glState.clearColor.pop();

	modified();
}

Vec4 Bitmap::getPixel(int x, int y) const
{
	GUARD_DISPOSED;

	if (x < 0 || y < 0 || x >= width() || y >= height())
		return Vec4();

	flush();

	p->bindFBO();

	glState.viewport.push();
	Vec4 pixel = FBO::getPixel(x, y, width(), height());
	glState.viewport.pop();

	return pixel;
}

void Bitmap::setPixel(int x, int y, const Vec4 &color)
{
	GUARD_DISPOSED

	p->pointArray.append(Vec2(x+.5, y+.5), color);

	modified();
}

void Bitmap::hueChange(int hue)
{
	GUARD_DISPOSED

	if ((hue % 360) == 0)
		return;

	flush();

	TexFBO newTex = gState->texPool().request(width(), height());

	FloatRect texRect(rect());

	Quad quad;
	quad.setTexPosRect(texRect, texRect);

	/* Calculate hue parameter */
	hue = wrapRange(hue, 0, 359);
	float hueAdj = -((M_PI * 2) / 360) * hue;

	HueShader &shader = gState->hueShader();
	shader.bind();
	shader.setHueAdjust(hueAdj);
	shader.setInputTexture(p->tex.tex);

	FBO::bind(newTex.fbo);
	Tex::bindMatrix(width(), height());
	p->pushSetViewport();

	p->blitQuad(quad);

	shader.unbind();

	p->popViewport();

	gState->texPool().release(p->tex);
	p->tex = newTex;

	modified();
}

void Bitmap::drawText(int x, int y,
                      int width, int height,
                      const char *str, int align)
{
	drawText(IntRect(x, y, width, height), str, align);
}

void Bitmap::drawText(const IntRect &rect, const char *str, int align)
{
	GUARD_DISPOSED

	if (*str == '\0')
		return;

	flush();

	TTF_Font *font = p->font->getSdlFont();

	SDL_Color c;
	p->font->getColor()->toSDLColor(c);

	SDL_Surface *txtSurf;

	if (gState->rtData().config.solidFonts)
		txtSurf = TTF_RenderUTF8_Solid(font, str, c);
	else
		txtSurf = TTF_RenderUTF8_Blended(font, str, c);

	p->ensureFormat(txtSurf, SDL_PIXELFORMAT_ARGB8888);

	int alignX = rect.x;

	switch (align)
	{
	default:
	case Left :
		break;

	case Center :
		alignX += (rect.w - txtSurf->w) / 2;
		break;

	case Right :
		alignX += rect.w - txtSurf->w;
		break;
	}

	if (alignX < rect.x)
		alignX = rect.x;

	int alignY = rect.y + (rect.h - txtSurf->h) / 2;

	float squeeze = (float) rect.w / txtSurf->w;

	if (squeeze > 1)
		squeeze = 1;

	FloatRect posRect(alignX, alignY, txtSurf->w * squeeze, txtSurf->h);

	Vec2 gpTexSize;
	gState->ensureTexSize(txtSurf->w, txtSurf->h, gpTexSize);

//	if (str[1] != '\0')
	{
		/* Aquire a partial copy of the destination
		 * buffer we're about to render to */
		TexFBO &gpTex2 = gState->gpTexFBO(posRect.w, posRect.h);

		FBO::bind(gpTex2.fbo, FBO::Draw);
		FBO::bind(p->tex.fbo, FBO::Read);
		FBO::blit(posRect.x, posRect.y, 0, 0, posRect.w, posRect.h);

		FloatRect bltRect(0, 0,
		                  gpTexSize.x / gpTex2.width,
		                  gpTexSize.y / gpTex2.height);

		BltShader &shader = gState->bltShader();
		shader.bind();
		shader.setSource();
		shader.setDestination(gpTex2.tex);
		shader.setSubRect(bltRect);
		shader.setOpacity(p->font->getColor()->norm.w);
	}

	gState->bindTex();
	Tex::uploadSubImage(0, 0, txtSurf->w, txtSurf->h, txtSurf->pixels, GL_BGRA);
	Tex::setSmooth(true);

	Quad quad;
	quad.setTexRect(FloatRect(0, 0, txtSurf->w, txtSurf->h));
	quad.setPosRect(posRect);
	SDL_FreeSurface(txtSurf);

	p->bindFBO();
	p->pushSetViewport();
	glState.blendMode.pushSet(BlendNone);

	quad.draw();

	glState.blendMode.pop();
	p->popViewport();

	FragShader::unbind();

	modified();
}

IntRect Bitmap::textSize(const char *str)
{
	GUARD_DISPOSED

	TTF_Font *font = p->font->getSdlFont();

	int w, h;
	TTF_SizeUTF8(font, str, &w, &h);

//	if (strlen(str) == 1)
//		TTF_GlyphMetrics(font, *str, 0, 0, 0, 0, &w);

	return IntRect(0, 0, w, h);
}

DEF_ATTR_SIMPLE(Bitmap, Font, Font*, p->font)

void Bitmap::flush() const
{
	p->flushPoints();
}

TexFBO &Bitmap::getGLTypes()
{
	return p->tex;
}

void Bitmap::bindTexWithMatrix()
{
	p->bindTextureWithMatrix();
}


void Bitmap::releaseResources()
{
	gState->texPool().release(p->tex);
	delete p;
}
