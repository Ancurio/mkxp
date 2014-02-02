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

#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>
#include <SDL_rect.h>

#include <pixman.h>

#include "gl-util.h"
#include "quad.h"
#include "quadarray.h"
#include "transform.h"
#include "exception.h"

#include "sharedstate.h"
#include "glstate.h"
#include "texpool.h"
#include "shader.h"
#include "filesystem.h"
#include "font.h"
#include "eventthread.h"

#define DISP_CLASS_NAME "bitmap"

#define GUARD_MEGA \
	{ \
		if (p->megaSurface) \
			throw Exception(Exception::MKXPError, \
                            "Operation not supported for mega surfaces"); \
	}

struct BitmapPrivate
{
	TEXFBO gl;

	Font *font;

	/* "Mega surfaces" are a hack to allow Tilesets to be used
	 * whose Bitmaps don't fit into a regular texture. They're
	 * kept in RAM and will throw an error if they're used in
	 * any context other than as Tilesets */
	SDL_Surface *megaSurface;

	/* The 'tainted' area describes which parts of the
	 * bitmap are not cleared, ie. don't have 0 opacity.
	 * If we're blitting / drawing text to a cleared part
	 * with full opacity, we can disregard any old contents
	 * in the texture and blit to it directly, saving
	 * ourselves the expensive blending calculation */
	pixman_region16_t tainted;

	BitmapPrivate()
	    : megaSurface(0)
	{
		font = &shState->defaultFont();
		pixman_region_init(&tainted);
	}

	~BitmapPrivate()
	{
		pixman_region_fini(&tainted);
	}

	void clearTaintedArea()
	{
		pixman_region_clear(&tainted);
	}

	void addTaintedArea(const IntRect &rect)
	{
		pixman_region_union_rect
		        (&tainted, &tainted, rect.x, rect.y, rect.w, rect.h);
	}

	void substractTaintedArea(const IntRect &rect)
	{
		if (!touchesTaintedArea(rect))
			return;

		pixman_region16_t m_reg;
		pixman_region_init_rect(&m_reg, rect.x, rect.y, rect.w, rect.h);

		pixman_region_subtract(&tainted, &m_reg, &tainted);

		pixman_region_fini(&m_reg);
	}

	bool touchesTaintedArea(const IntRect &rect)
	{
		pixman_box16_t box;
		box.x1 = rect.x;
		box.y1 = rect.y;
		box.x2 = rect.x + rect.w;
		box.y2 = rect.y + rect.h;

		pixman_region_overlap_t result =
		        pixman_region_contains_rectangle(&tainted, &box);

		return result != PIXMAN_REGION_OUT;
	}

	void bindTexture(ShaderBase &shader)
	{
		TEX::bind(gl.tex);
		shader.setTexSize(Vec2i(gl.width, gl.height));
	}

	void bindFBO()
	{
		FBO::bind(gl.fbo, FBO::Draw);
	}

	void pushSetViewport(ShaderBase &shader) const
	{
		glState.viewport.pushSet(IntRect(0, 0, gl.width, gl.height));
		shader.applyViewportProj();
	}

	void popViewport() const
	{
		glState.viewport.pop();
	}

	void blitQuad(Quad &quad)
	{
		glState.blendMode.pushSet(BlendNone);
		quad.draw();
		glState.blendMode.pop();
	}

	void fillRect(const IntRect &rect,
	              const Vec4 &color)
	{
		bindFBO();

		glState.scissorTest.pushSet(true);
		glState.scissorBox.pushSet(rect);
		glState.clearColor.pushSet(color);

		FBO::clear();

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
	const char *extension;
	shState->fileSystem().openRead(ops, filename, FileSystem::Image, false, &extension);
	SDL_Surface *imgSurf = IMG_LoadTyped_RW(&ops, 1, extension);

	if (!imgSurf)
		throw Exception(Exception::SDLError, "Error loading image '%s': %s",
		                filename, SDL_GetError());

	p->ensureFormat(imgSurf, SDL_PIXELFORMAT_ABGR8888);

	if (imgSurf->w > glState.caps.maxTexSize || imgSurf->h > glState.caps.maxTexSize)
	{
		/* Mega surface */
		p = new BitmapPrivate;
		p->megaSurface = imgSurf;
	}
	else
	{
		/* Regular surface */
		TEXFBO tex;

		try
		{
			tex = shState->texPool().request(imgSurf->w, imgSurf->h);
		}
		catch (const Exception &e)
		{
			SDL_FreeSurface(imgSurf);
			throw e;
		}

		p = new BitmapPrivate;
		p->gl = tex;

		TEX::bind(p->gl.tex);
		TEX::uploadImage(p->gl.width, p->gl.height, imgSurf->pixels, GL_RGBA);

		SDL_FreeSurface(imgSurf);
	}

	p->addTaintedArea(rect());
}

Bitmap::Bitmap(int width, int height)
{
	if (width <= 0 || height <= 0)
		throw Exception(Exception::RGSSError, "failed to create bitmap");

	TEXFBO tex = shState->texPool().request(width, height);

	p = new BitmapPrivate;
	p->gl = tex;

	clear();
}

Bitmap::Bitmap(const Bitmap &other)
{
	other.ensureNonMega();

	p = new BitmapPrivate;

	p->gl = shState->texPool().request(other.width(), other.height());

	blt(0, 0, other, rect());
}

Bitmap::~Bitmap()
{
	dispose();
}

int Bitmap::width() const
{
	GUARD_DISPOSED;

	if (p->megaSurface)
		return p->megaSurface->w;

	return p->gl.width;
}

int Bitmap::height() const
{
	GUARD_DISPOSED;

	if (p->megaSurface)
		return p->megaSurface->h;

	return p->gl.height;
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

	GUARD_MEGA;

	opacity = clamp(opacity, 0, 255);

	if (opacity == 0)
		return;

	if (source.megaSurface())
	{
		/* Don't do transparent blits for now */
		if (opacity < 255)
			source.ensureNonMega();

		SDL_Surface *srcSurf = source.megaSurface();

		int bpp;
		Uint32 rMask, gMask, bMask, aMask;
		SDL_PixelFormatEnumToMasks(SDL_PIXELFORMAT_ABGR8888,
		                           &bpp, &rMask, &gMask, &bMask, &aMask);
		SDL_Surface *blitTemp =
		        SDL_CreateRGBSurface(0, destRect.w, destRect.h, bpp, rMask, gMask, bMask, aMask);

		SDL_Rect srcRect;
		srcRect.x = sourceRect.x;
		srcRect.y = sourceRect.y;
		srcRect.w = sourceRect.w;
		srcRect.h = sourceRect.h;

		SDL_Rect dstRect;
		dstRect.x = dstRect.y = 0;
		dstRect.w = blitTemp->w;
		dstRect.h = blitTemp->h;

		// FXIME: This is supposed to be a scaled blit, put SDL2 for some reason
		// makes the source surface unusable after BlitScaled() is called. Investigate!
		SDL_BlitSurface(srcSurf, &srcRect, blitTemp, &dstRect);

		TEX::bind(p->gl.tex);
		TEX::uploadSubImage(destRect.x, destRect.y, destRect.w, destRect.h, blitTemp->pixels, GL_RGBA);

		SDL_FreeSurface(blitTemp);

		modified();
		return;
	}

	if (opacity == 255 && !p->touchesTaintedArea(destRect))
	{
		/* Fast blit */
		FBO::bind(source.p->gl.fbo, FBO::Read);
		FBO::bind(p->gl.fbo, FBO::Draw);

		FBO::blit(sourceRect.x, sourceRect.y, sourceRect.w, sourceRect.h,
		          destRect.x,   destRect.y,   destRect.w,   destRect.h);
	}
	else
	{
		/* Fragment pipeline */
		float normOpacity = (float) opacity / 255.0f;

		TEXFBO &gpTex = shState->gpTexFBO(destRect.w, destRect.h);

		FBO::bind(gpTex.fbo, FBO::Draw);
		FBO::bind(p->gl.fbo, FBO::Read);
		FBO::blit(destRect.x, destRect.y, 0, 0, destRect.w, destRect.h);

		FloatRect bltSubRect((float) sourceRect.x / source.width(),
		                     (float) sourceRect.y / source.height(),
		                     ((float) source.width() / sourceRect.w) * ((float) destRect.w / gpTex.width),
		                     ((float) source.height() / sourceRect.h) * ((float) destRect.h / gpTex.height));

		BltShader &shader = shState->shaders().blt;
		shader.bind();
		shader.setDestination(gpTex.tex);
		shader.setSubRect(bltSubRect);
		shader.setOpacity(normOpacity);

		Quad &quad = shState->gpQuad();
		quad.setTexPosRect(sourceRect, destRect);
		quad.setColor(Vec4(1, 1, 1, normOpacity));

		source.p->bindTexture(shader);
		p->bindFBO();
		p->pushSetViewport(shader);

		p->blitQuad(quad);

		p->popViewport();
	}

	p->addTaintedArea(destRect);

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
	GUARD_DISPOSED;

	GUARD_MEGA;

	p->fillRect(rect, color);

	if (color.w == 0)
		/* Clear op */
		p->substractTaintedArea(rect);
	else
		/* Fill op */
		p->addTaintedArea(rect);

	modified();
}

#ifdef RGSS2

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
	GUARD_DISPOSED;

	GUARD_MEGA;

	SimpleColorShader &shader = shState->shaders().simpleColor;
	shader.bind();
	shader.setTranslation(Vec2i());

	Quad &quad = shState->gpQuad();

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

	p->bindFBO();
	p->pushSetViewport(shader);

	p->blitQuad(quad);

	p->popViewport();

	p->addTaintedArea(rect);

	modified();
}

void Bitmap::clearRect(int x, int y, int width, int height)
{
	clearRect(IntRect(x, y, width, height));
}

void Bitmap::clearRect(const IntRect &rect)
{
	GUARD_DISPOSED;

	GUARD_MEGA;

	p->fillRect(rect, Vec4());

	modified();
}

void Bitmap::blur()
{
	GUARD_DISPOSED;

	GUARD_MEGA;

	Quad &quad = shState->gpQuad();
	FloatRect rect(0, 0, width(), height());
	quad.setTexPosRect(rect, rect);

	TEXFBO auxTex = shState->texPool().request(width(), height());

	BlurShader &shader = shState->shaders().blur;
	BlurShader::HPass &pass1 = shader.pass1;
	BlurShader::VPass &pass2 = shader.pass2;

	glState.blendMode.pushSet(BlendNone);
	glState.viewport.pushSet(IntRect(0, 0, width(), height()));

	TEX::bind(p->gl.tex);
	FBO::bind(auxTex.fbo, FBO::Draw);

	pass1.bind();
	pass1.setTexSize(Vec2i(width(), height()));
	pass1.applyViewportProj();

	quad.draw();

	TEX::bind(auxTex.tex);
	FBO::bind(p->gl.fbo, FBO::Draw);

	pass2.bind();
	pass2.setTexSize(Vec2i(width(), height()));
	pass2.applyViewportProj();

	quad.draw();

	glState.viewport.pop();
	glState.blendMode.pop();

	shState->texPool().release(auxTex);

	modified();
}

void Bitmap::radialBlur(int angle, int divisions)
{
	GUARD_DISPOSED;

	GUARD_MEGA;

	angle     = clamp<int>(angle, 0, 359);
	divisions = clamp<int>(divisions, 2, 100);

	const int _width = width();
	const int _height = height();

	float angleStep = (float) angle / (divisions-1);
	float opacity   = 1.0f / divisions;
	float baseAngle = -((float) angle / 2);

	ColorQuadArray qArray;
	qArray.resize(5);

	std::vector<Vertex> &vert = qArray.vertices;

	int i = 0;

	/* Center */
	FloatRect texRect(0, 0, _width, _height);
	FloatRect posRect(0, 0, _width, _height);

	i += Quad::setTexPosRect(&vert[i*4], texRect, posRect);

	/* Upper */
	posRect = FloatRect(0, 0, _width, -_height);

	i += Quad::setTexPosRect(&vert[i*4], texRect, posRect);

	/* Lower */
	posRect = FloatRect(0, _height*2, _width, -_height);

	i += Quad::setTexPosRect(&vert[i*4], texRect, posRect);

	/* Left */
	posRect = FloatRect(0, 0, -_width, _height);

	i += Quad::setTexPosRect(&vert[i*4], texRect, posRect);

	/* Right */
	posRect = FloatRect(_width*2, 0, -_width, _height);

	i += Quad::setTexPosRect(&vert[i*4], texRect, posRect);

	for (int i = 0; i < 4*5; ++i)
		vert[i].color = Vec4(1, 1, 1, opacity);

	qArray.commit();

	TEXFBO newTex = shState->texPool().request(_width, _height);

	FBO::bind(newTex.fbo, FBO::Draw);

	glState.clearColor.pushSet(Vec4());
	FBO::clear();

	Transform trans;
	trans.setOrigin(Vec2(_width / 2.0f, _height / 2.0f));
	trans.setPosition(Vec2(_width / 2.0f, _height / 2.0f));

	glState.blendMode.pushSet(BlendAddition);

	SimpleMatrixShader &shader = shState->shaders().simpleMatrix;
	shader.bind();

	p->bindTexture(shader);
	TEX::setSmooth(true);

	p->pushSetViewport(shader);

	for (int i = 0; i < divisions; ++i)
	{
		trans.setRotation(baseAngle + i*angleStep);
		shader.setMatrix(trans.getMatrix());
		qArray.draw();
	}

	p->popViewport();

	TEX::setSmooth(false);

	glState.blendMode.pop();
	glState.clearColor.pop();

	shState->texPool().release(p->gl);
	p->gl = newTex;

	modified();
}

#endif

void Bitmap::clear()
{
	GUARD_DISPOSED;

	GUARD_MEGA;

	p->bindFBO();

	glState.clearColor.pushSet(Vec4());

	FBO::clear();

	glState.clearColor.pop();

	p->clearTaintedArea();

	modified();
}

Color Bitmap::getPixel(int x, int y) const
{
	GUARD_DISPOSED;

	GUARD_MEGA;

	if (x < 0 || y < 0 || x >= width() || y >= height())
		return Vec4();

	FBO::bind(p->gl.fbo, FBO::Read);

	glState.viewport.pushSet(IntRect(0, 0, width(), height()));

	uint8_t pixel[4];
	glReadPixels(x, y, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, &pixel);

	glState.viewport.pop();

	return Color(pixel[0], pixel[1], pixel[2], pixel[3]);
}

void Bitmap::setPixel(int x, int y, const Color &color)
{
	GUARD_DISPOSED;

	GUARD_MEGA;

	uint8_t pixel[] =
	{
		(uint8_t) clamp<double>(color.red,   0, 255),
		(uint8_t) clamp<double>(color.green, 0, 255),
		(uint8_t) clamp<double>(color.blue,  0, 255),
		(uint8_t) clamp<double>(color.alpha, 0, 255)
	};

	TEX::bind(p->gl.tex);
	TEX::uploadSubImage(x, y, 1, 1, &pixel, GL_RGBA);

	p->addTaintedArea(IntRect(x, y, 1, 1));

	modified();
}

void Bitmap::hueChange(int hue)
{
	GUARD_DISPOSED;

	GUARD_MEGA;

	if ((hue % 360) == 0)
		return;

	TEXFBO newTex = shState->texPool().request(width(), height());

	FloatRect texRect(rect());

	Quad &quad = shState->gpQuad();
	quad.setTexPosRect(texRect, texRect);
	quad.setColor(Vec4(1, 1, 1, 1));

	/* Calculate hue parameter */
	hue = wrapRange(hue, 0, 359);
	float hueAdj = -((M_PI * 2) / 360) * hue;

	HueShader &shader = shState->shaders().hue;
	shader.bind();
	shader.setHueAdjust(hueAdj);

	FBO::bind(newTex.fbo, FBO::Draw);
	p->pushSetViewport(shader);
	p->bindTexture(shader);

	p->blitQuad(quad);

	p->popViewport();

	TEX::unbind();

	shState->texPool().release(p->gl);
	p->gl = newTex;

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
	GUARD_DISPOSED;

	GUARD_MEGA;

	if (*str == '\0')
		return;

	if (str[0] == ' ' && str[1] == '\0')
		return;

	TTF_Font *font = p->font->getSdlFont();
	Color *fontColor = p->font->getColor();

	SDL_Color c;
	fontColor->toSDLColor(c);

	float txtAlpha = fontColor->norm.w;

	SDL_Surface *txtSurf;

	if (shState->rtData().config.solidFonts)
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

	Vec2i gpTexSize;
	shState->ensureTexSize(txtSurf->w, txtSurf->h, gpTexSize);

	bool fastBlit = !p->touchesTaintedArea(posRect) && txtAlpha == 1.0;

	if (fastBlit)
	{
		if (squeeze == 1.0)
		{
			/* Even faster: upload directly to bitmap texture.
			 * We have to make sure the posRect lies within the texture
			 * boundaries or texSubImage will generate errors.
			 * If it partly lies outside bounds we have to upload
			 * the clipped visible part of it. */
			SDL_Rect btmRect;
			btmRect.x = btmRect.y = 0;
			btmRect.w = width();
			btmRect.h = height();

			SDL_Rect txtRect;
			txtRect.x = posRect.x;
			txtRect.y = posRect.y;
			txtRect.w = posRect.w;
			txtRect.h = posRect.h;

			SDL_Rect inters;

			/* If we have no intersection at all,
			 * there's nothing to upload to begin with */
			if (SDL_IntersectRect(&btmRect, &txtRect, &inters))
			{
				if (inters.w != txtRect.w || inters.h != txtRect.h)
				{
					/* Clip the text surface */
					SDL_Rect clipSrc = inters;
					clipSrc.x -= txtRect.x;
					clipSrc.y -= txtRect.y;

					posRect.x = inters.x;
					posRect.y = inters.y;
					posRect.w = inters.w;
					posRect.h = inters.h;

					PixelStore::setupSubImage(txtSurf->w, clipSrc.x, clipSrc.y);
				}

				TEX::bind(p->gl.tex);
				TEX::uploadSubImage(posRect.x, posRect.y, posRect.w, posRect.h, txtSurf->pixels, GL_BGRA);

				PixelStore::reset();
			}
		}
		else
		{
			/* Squeezing involved: need to use intermediary TexFBO */
			TEXFBO &gpTF = shState->gpTexFBO(txtSurf->w, txtSurf->h);

			TEX::bind(gpTF.tex);
			TEX::uploadSubImage(0, 0, txtSurf->w, txtSurf->h, txtSurf->pixels, GL_BGRA);

			FBO::bind(gpTF.fbo, FBO::Read);
			p->bindFBO();

			FBO::blit(0, 0, txtSurf->w, txtSurf->h,
			          posRect.x, posRect.y, posRect.w, posRect.h,
			          FBO::Linear);
		}
	}
	else
	{
		/* Aquire a partial copy of the destination
		 * buffer we're about to render to */
		TEXFBO &gpTex2 = shState->gpTexFBO(posRect.w, posRect.h);

		FBO::bind(gpTex2.fbo, FBO::Draw);
		FBO::bind(p->gl.fbo, FBO::Read);
		FBO::blit(posRect.x, posRect.y, 0, 0, posRect.w, posRect.h);

		FloatRect bltRect(0, 0,
		                  (float) gpTexSize.x / gpTex2.width,
		                  (float) gpTexSize.y / gpTex2.height);

		BltShader &shader = shState->shaders().blt;
		shader.bind();
		shader.setTexSize(gpTexSize);
		shader.setSource();
		shader.setDestination(gpTex2.tex);
		shader.setSubRect(bltRect);
		shader.setOpacity(txtAlpha);

		shState->bindTex();
		TEX::uploadSubImage(0, 0, txtSurf->w, txtSurf->h, txtSurf->pixels, GL_BGRA);
		TEX::setSmooth(true);

		Quad &quad = shState->gpQuad();
		quad.setTexRect(FloatRect(0, 0, txtSurf->w, txtSurf->h));
		quad.setPosRect(posRect);

		p->bindFBO();
		p->pushSetViewport(shader);

		glState.blendMode.pushSet(BlendNone);

		quad.draw();

		glState.blendMode.pop();
		p->popViewport();
	}

	SDL_FreeSurface(txtSurf);
	p->addTaintedArea(posRect);

	modified();
}

/* http://www.lemoda.net/c/utf8-to-ucs2/index.html */
static uint16_t utf8_to_ucs2(const char *_input,
                             const char **end_ptr)
{
	const unsigned char *input =
	        reinterpret_cast<const unsigned char*>(_input);
	*end_ptr = _input;

	if (input[0] == 0)
		return -1;

	if (input[0] < 0x80)
	{
		*end_ptr = _input + 1;

		return input[0];
	}

	if ((input[0] & 0xE0) == 0xE0)
	{
		if (input[1] == 0 || input[2] == 0)
			return -1;

		*end_ptr = _input + 3;

		return (input[0] & 0x0F)<<12 |
		       (input[1] & 0x3F)<<6  |
		       (input[2] & 0x3F);
	}

	if ((input[0] & 0xC0) == 0xC0)
	{
		if (input[1] == 0)
			return -1;

		*end_ptr = _input + 2;

		return (input[0] & 0x1F)<<6  |
		       (input[1] & 0x3F);
	}

	return -1;
}

IntRect Bitmap::textSize(const char *str)
{
	GUARD_DISPOSED;

	GUARD_MEGA;

	TTF_Font *font = p->font->getSdlFont();

	int w, h;
	TTF_SizeUTF8(font, str, &w, &h);

	/* If str is one character long, *endPtr == 0 */
	const char *endPtr;
	uint16_t ucs2 = utf8_to_ucs2(str, &endPtr);

	/* For cursive characters, returning the advance
	 * as width yields better results */
	if (p->font->getItalic() && *endPtr == '\0')
		TTF_GlyphMetrics(font, ucs2, 0, 0, 0, 0, &w);

	return IntRect(0, 0, w, h);
}

DEF_ATTR_SIMPLE(Bitmap, Font, Font*, p->font)

TEXFBO &Bitmap::getGLTypes()
{
	return p->gl;
}

SDL_Surface *Bitmap::megaSurface() const
{
	return p->megaSurface;
}

void Bitmap::ensureNonMega() const
{
	if (isDisposed())
		return;

	GUARD_MEGA;
}

void Bitmap::bindTex(ShaderBase &shader)
{
	p->bindTexture(shader);
}

void Bitmap::releaseResources()
{
	if (p->megaSurface)
		SDL_FreeSurface(p->megaSurface);
	else
		shState->texPool().release(p->gl);

	delete p;
}
