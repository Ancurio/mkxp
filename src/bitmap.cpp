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
#include <SDL_surface.h>

#include <pixman.h>

#include "gl-util.h"
#include "gl-meta.h"
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

#define GUARD_MEGA \
	{ \
		if (p->megaSurface) \
			throw Exception(Exception::MKXPError, \
                            "Operation not supported for mega surfaces"); \
	}

#define OUTLINE_SIZE 1

/* Normalize (= ensure width and
 * height are positive) */
static IntRect normalizedRect(const IntRect &rect)
{
	IntRect norm = rect;

	if (norm.w < 0)
	{
		norm.w = -norm.w;
		norm.x -= norm.w;
	}

	if (norm.h < 0)
	{
		norm.h = -norm.h;
		norm.y -= norm.h;
	}

	return norm;
}

struct BitmapPrivate
{
	Bitmap *self;

	TEXFBO gl;

	Font *font;

	/* "Mega surfaces" are a hack to allow Tilesets to be used
	 * whose Bitmaps don't fit into a regular texture. They're
	 * kept in RAM and will throw an error if they're used in
	 * any context other than as Tilesets */
	SDL_Surface *megaSurface;

	/* A cached version of the bitmap in client memory, for
	 * getPixel calls. Is invalidated any time the bitmap
	 * is modified */
	SDL_Surface *surface;
	SDL_PixelFormat *format;

	/* The 'tainted' area describes which parts of the
	 * bitmap are not cleared, ie. don't have 0 opacity.
	 * If we're blitting / drawing text to a cleared part
	 * with full opacity, we can disregard any old contents
	 * in the texture and blit to it directly, saving
	 * ourselves the expensive blending calculation */
	pixman_region16_t tainted;

	BitmapPrivate(Bitmap *self)
	    : self(self),
	      megaSurface(0),
	      surface(0)
	{
		format = SDL_AllocFormat(SDL_PIXELFORMAT_ABGR8888);

		font = &shState->defaultFont();
		pixman_region_init(&tainted);
	}

	~BitmapPrivate()
	{
		SDL_FreeFormat(format);
		pixman_region_fini(&tainted);
	}

	void allocSurface()
	{
		surface = SDL_CreateRGBSurface(0, gl.width, gl.height, format->BitsPerPixel,
		                               format->Rmask, format->Gmask,
		                               format->Bmask, format->Amask);
	}

	void clearTaintedArea()
	{
		pixman_region_fini(&tainted);
		pixman_region_init(&tainted);
	}

	void addTaintedArea(const IntRect &rect)
	{	
		IntRect norm = normalizedRect(rect);
		pixman_region_union_rect
		        (&tainted, &tainted, norm.x, norm.y, norm.w, norm.h);
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
		FBO::bind(gl.fbo);
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
		glState.blend.pushSet(false);
		quad.draw();
		glState.blend.pop();
	}

	void fillRect(const IntRect &rect,
	              const Vec4 &color)
	{
		bindFBO();

		glState.scissorTest.pushSet(true);
		glState.scissorBox.pushSet(normalizedRect(rect));
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

	void onModified()
	{
		if (surface)
		{
			SDL_FreeSurface(surface);
			surface = 0;
		}

		self->modified();
	}
};

struct BitmapOpenHandler : FileSystem::OpenHandler
{
	SDL_Surface *surf;

	BitmapOpenHandler()
	    : surf(0)
	{}

	bool tryRead(SDL_RWops &ops, const char *ext)
	{
		surf = IMG_LoadTyped_RW(&ops, 1, ext);
		return surf != 0;
	}
};

Bitmap::Bitmap(const char *filename)
{
	BitmapOpenHandler handler;
	shState->fileSystem().openRead(handler, filename);
	SDL_Surface *imgSurf = handler.surf;

	if (!imgSurf)
		throw Exception(Exception::SDLError, "Error loading image '%s': %s",
		                filename, SDL_GetError());

	p->ensureFormat(imgSurf, SDL_PIXELFORMAT_ABGR8888);

	if (imgSurf->w > glState.caps.maxTexSize || imgSurf->h > glState.caps.maxTexSize)
	{
		/* Mega surface */
		p = new BitmapPrivate(this);
		p->megaSurface = imgSurf;
		SDL_SetSurfaceBlendMode(p->megaSurface, SDL_BLENDMODE_NONE);
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

		p = new BitmapPrivate(this);
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

	p = new BitmapPrivate(this);
	p->gl = tex;

	clear();
}

Bitmap::Bitmap(const Bitmap &other)
{
	other.ensureNonMega();

	p = new BitmapPrivate(this);

	p->gl = shState->texPool().request(other.width(), other.height());

	blt(0, 0, other, rect());
}

Bitmap::~Bitmap()
{
	dispose();
}

int Bitmap::width() const
{
	guardDisposed();

	if (p->megaSurface)
		return p->megaSurface->w;

	return p->gl.width;
}

int Bitmap::height() const
{
	guardDisposed();

	if (p->megaSurface)
		return p->megaSurface->h;

	return p->gl.height;
}

IntRect Bitmap::rect() const
{
	guardDisposed();

	return IntRect(0, 0, width(), height());
}

void Bitmap::blt(int x, int y,
                  const Bitmap &source, IntRect rect,
                  int opacity)
{
	if (source.isDisposed())
		return;

	// FIXME: RGSS allows the source rect to both lie outside
	// the bitmap rect and be inverted in both directions;
	// clamping only covers a subset of these cases (and
	// doesn't fix anything for a direct stretch_blt call).

	/* Clamp rect to source bitmap size */
	if (rect.x + rect.w > source.width())
		rect.w = source.width() - rect.x;

	if (rect.y + rect.h > source.height())
		rect.h = source.height() - rect.y;

	stretchBlt(IntRect(x, y, rect.w, rect.h),
	           source, rect, opacity);
}

void Bitmap::stretchBlt(const IntRect &destRect,
                        const Bitmap &source, const IntRect &sourceRect,
                        int opacity)
{
	guardDisposed();

	GUARD_MEGA;

	if (source.isDisposed())
		return;

	opacity = clamp(opacity, 0, 255);

	if (opacity == 0)
		return;

	SDL_Surface *srcSurf = source.megaSurface();

	if (srcSurf && shState->config().subImageFix)
	{
		/* Blit from software surface, for broken GL drivers */
		Vec2i gpTexSize;
		shState->ensureTexSize(sourceRect.w, sourceRect.h, gpTexSize);
		shState->bindTex();

		GLMeta::subRectImageUpload(srcSurf->w, sourceRect.x, sourceRect.y, 0, 0,
		                           sourceRect.w, sourceRect.h, srcSurf, GL_RGBA);
		GLMeta::subRectImageEnd();

		SimpleShader &shader = shState->shaders().simple;
		shader.bind();
		shader.setTranslation(Vec2i());
		shader.setTexSize(gpTexSize);

		p->pushSetViewport(shader);
		p->bindFBO();

		Quad &quad = shState->gpQuad();
		quad.setTexRect(FloatRect(0, 0, sourceRect.w, sourceRect.h));
		quad.setPosRect(destRect);

		p->blitQuad(quad);
		p->popViewport();

		p->addTaintedArea(destRect);
		p->onModified();

		return;
	}
	else if (srcSurf)
	{
		/* Blit from software surface */
		/* Don't do transparent blits for now */
		if (opacity < 255)
			source.ensureNonMega();

		SDL_Rect srcRect = sourceRect;
		SDL_Rect dstRect = destRect;
		SDL_Rect btmRect = { 0, 0, width(), height() };
		SDL_Rect bltRect;

		if (SDL_IntersectRect(&btmRect, &dstRect, &bltRect) != SDL_TRUE)
			return;

		int bpp;
		Uint32 rMask, gMask, bMask, aMask;
		SDL_PixelFormatEnumToMasks(SDL_PIXELFORMAT_ABGR8888,
		                           &bpp, &rMask, &gMask, &bMask, &aMask);
		SDL_Surface *blitTemp =
			SDL_CreateRGBSurface(0, destRect.w, destRect.h, bpp, rMask, gMask, bMask, aMask);

		SDL_BlitScaled(srcSurf, &srcRect, blitTemp, 0);

		TEX::bind(p->gl.tex);

		if (bltRect.w == dstRect.w && bltRect.h == dstRect.h)
		{
			/* Dest rectangle lies within bounding box */
			TEX::uploadSubImage(destRect.x, destRect.y,
			                    destRect.w, destRect.h,
			                    blitTemp->pixels, GL_RGBA);
		}
		else
		{
			/* Clipped blit */
			GLMeta::subRectImageUpload(blitTemp->w, bltRect.x - dstRect.x, bltRect.y - dstRect.y,
			                           bltRect.x, bltRect.y, bltRect.w, bltRect.h, blitTemp, GL_RGBA);
			GLMeta::subRectImageEnd();
		}

		SDL_FreeSurface(blitTemp);

		p->onModified();
		return;
	}

	if (opacity == 255 && !p->touchesTaintedArea(destRect))
	{
		/* Fast blit */
		GLMeta::blitBegin(p->gl);
		GLMeta::blitSource(source.p->gl);
		GLMeta::blitRectangle(sourceRect, destRect);
		GLMeta::blitEnd();
	}
	else
	{
		/* Fragment pipeline */
		float normOpacity = (float) opacity / 255.0f;

		TEXFBO &gpTex = shState->gpTexFBO(destRect.w, destRect.h);

		GLMeta::blitBegin(gpTex);
		GLMeta::blitSource(p->gl);
		GLMeta::blitRectangle(destRect, Vec2i());
		GLMeta::blitEnd();

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
	p->onModified();
}

void Bitmap::fillRect(int x, int y,
                      int width, int height,
                      const Vec4 &color)
{
	fillRect(IntRect(x, y, width, height), color);
}

void Bitmap::fillRect(const IntRect &rect, const Vec4 &color)
{
	guardDisposed();

	GUARD_MEGA;

	p->fillRect(rect, color);

	if (color.w == 0)
		/* Clear op */
		p->substractTaintedArea(rect);
	else
		/* Fill op */
		p->addTaintedArea(rect);

	p->onModified();
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
	guardDisposed();

	GUARD_MEGA;

	SimpleColorShader &shader = shState->shaders().simpleColor;
	shader.bind();
	shader.setTranslation(Vec2i());

	Quad &quad = shState->gpQuad();

	if (vertical)
	{
		quad.vert[0].color = color1;
		quad.vert[1].color = color1;
		quad.vert[2].color = color2;
		quad.vert[3].color = color2;
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

	p->onModified();
}

void Bitmap::clearRect(int x, int y, int width, int height)
{
	clearRect(IntRect(x, y, width, height));
}

void Bitmap::clearRect(const IntRect &rect)
{
	guardDisposed();

	GUARD_MEGA;

	p->fillRect(rect, Vec4());

	p->onModified();
}

void Bitmap::blur()
{
	guardDisposed();

	GUARD_MEGA;

	Quad &quad = shState->gpQuad();
	FloatRect rect(0, 0, width(), height());
	quad.setTexPosRect(rect, rect);

	TEXFBO auxTex = shState->texPool().request(width(), height());

	BlurShader &shader = shState->shaders().blur;
	BlurShader::HPass &pass1 = shader.pass1;
	BlurShader::VPass &pass2 = shader.pass2;

	glState.blend.pushSet(false);
	glState.viewport.pushSet(IntRect(0, 0, width(), height()));

	TEX::bind(p->gl.tex);
	FBO::bind(auxTex.fbo);

	pass1.bind();
	pass1.setTexSize(Vec2i(width(), height()));
	pass1.applyViewportProj();

	quad.draw();

	TEX::bind(auxTex.tex);
	p->bindFBO();

	pass2.bind();
	pass2.setTexSize(Vec2i(width(), height()));
	pass2.applyViewportProj();

	quad.draw();

	glState.viewport.pop();
	glState.blend.pop();

	shState->texPool().release(auxTex);

	p->onModified();
}

void Bitmap::radialBlur(int angle, int divisions)
{
	guardDisposed();

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

	FBO::bind(newTex.fbo);

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

	p->onModified();
}

void Bitmap::clear()
{
	guardDisposed();

	GUARD_MEGA;

	p->bindFBO();

	glState.clearColor.pushSet(Vec4());

	FBO::clear();

	glState.clearColor.pop();

	p->clearTaintedArea();

	p->onModified();
}

Color Bitmap::getPixel(int x, int y) const
{
	guardDisposed();

	GUARD_MEGA;

	if (x < 0 || y < 0 || x >= width() || y >= height())
		return Vec4();

	if (!p->surface)
	{
		p->allocSurface();

		FBO::bind(p->gl.fbo);

		glState.viewport.pushSet(IntRect(0, 0, width(), height()));

		gl.ReadPixels(0, 0, width(), height(), GL_RGBA, GL_UNSIGNED_BYTE, p->surface->pixels);

		glState.viewport.pop();
	}

	size_t offset = x*p->format->BytesPerPixel + y*p->surface->pitch;
	uint8_t *bytes = (uint8_t*) p->surface->pixels + offset;
	uint32_t pixel = *((uint32_t*) bytes);

	return Color((pixel >> p->format->Rshift) & 0xFF,
	             (pixel >> p->format->Gshift) & 0xFF,
	             (pixel >> p->format->Bshift) & 0xFF,
	             (pixel >> p->format->Ashift) & 0xFF);
}

void Bitmap::setPixel(int x, int y, const Color &color)
{
	guardDisposed();

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

	p->onModified();
}

void Bitmap::hueChange(int hue)
{
	guardDisposed();

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

	FBO::bind(newTex.fbo);
	p->pushSetViewport(shader);
	p->bindTexture(shader);

	p->blitQuad(quad);

	p->popViewport();

	TEX::unbind();

	shState->texPool().release(p->gl);
	p->gl = newTex;

	p->onModified();
}

void Bitmap::drawText(int x, int y,
                      int width, int height,
                      const char *str, int align)
{
	drawText(IntRect(x, y, width, height), str, align);
}

static std::string fixupString(const char *str)
{
	std::string s(str);

	/* RMXP actually draws LF as a "missing gylph" box,
	 * but since we might have accidentally converted CRs
	 * to LFs when editing scripts on a Unix OS, treat them
	 * as white space too */
	for (size_t i = 0; i < s.size(); ++i)
		if (s[i] == '\r' || s[i] == '\n')
			s[i] = ' ';

	return s;
}

static void applyShadow(SDL_Surface *&in, const SDL_PixelFormat &fm, const SDL_Color &c)
{
	SDL_Surface *out = SDL_CreateRGBSurface
		(0, in->w+1, in->h+1, fm.BitsPerPixel, fm.Rmask, fm.Gmask, fm.Bmask, fm.Amask);

	float fr = c.r / 255.0f;
	float fg = c.g / 255.0f;
	float fb = c.b / 255.0f;

	/* We allocate an output surface one pixel wider and higher than the input,
	 * (implicitly) blit a copy of the input with RGB values set to black into
	 * it with x/y offset by 1, then blend the input surface over it at origin
	 * (0,0) using the bitmap blit equation (see shader/bitmapBlit.frag) */

	for (int y = 0; y < in->h+1; ++y)
		for (int x = 0; x < in->w+1; ++x)
		{
			/* src: input pixel, shd: shadow pixel */
			uint32_t src = 0, shd = 0;

			/* Output pixel location */
			uint32_t *outP = ((uint32_t*) ((uint8_t*) out->pixels + y*out->pitch)) + x;

			if (y < in->h && x < in->w)
				src = ((uint32_t*) ((uint8_t*) in->pixels + y*in->pitch))[x];

			if (y > 0 && x > 0)
				shd = ((uint32_t*) ((uint8_t*) in->pixels + (y-1)*in->pitch))[x-1];

			/* Set shadow pixel RGB values to 0 (black) */
			shd &= fm.Amask;

			if (x == 0 || y == 0)
			{
				*outP = src;
				continue;
			}

			if (x == in->w || y == in->h)
			{
				*outP = shd;
				continue;
			}

			/* Input and shadow alpha values */
			uint8_t srcA, shdA;
			srcA = (src & fm.Amask) >> fm.Ashift;
			shdA = (shd & fm.Amask) >> fm.Ashift;

			if (srcA == 255 || shdA == 0)
			{
				*outP = src;
				continue;
			}

			if (srcA == 0 && shdA == 0)
			{
				*outP = 0;
				continue;
			}

			float fSrcA = srcA / 255.0f;
			float fShdA = shdA / 255.0f;

			/* Because opacity == 1, co1 == fSrcA */
			float co2 = fShdA * (1.0f - fSrcA);
			/* Result alpha */
			float fa = fSrcA + co2;
			/* Temp value to simplify arithmetic below */
			float co3 = fSrcA / fa;

			/* Result colors */
			uint8_t r, g, b, a;

			r = clamp<float>(fr * co3, 0, 1) * 255.0f;
			g = clamp<float>(fg * co3, 0, 1) * 255.0f;
			b = clamp<float>(fb * co3, 0, 1) * 255.0f;
			a = clamp<float>(fa, 0, 1) * 255.0f;

			*outP = SDL_MapRGBA(&fm, r, g, b, a);
		}

	/* Store new surface in the input pointer */
	SDL_FreeSurface(in);
	in = out;
}

void Bitmap::drawText(const IntRect &rect, const char *str, int align)
{
	guardDisposed();

	GUARD_MEGA;

	std::string fixed = fixupString(str);
	str = fixed.c_str();

	if (*str == '\0')
		return;

	if (str[0] == ' ' && str[1] == '\0')
		return;

	TTF_Font *font = p->font->getSdlFont();
	const Color &fontColor = p->font->getColor();
	const Color &outColor = p->font->getOutColor();

	SDL_Color c = fontColor.toSDLColor();
	c.a = 255;

	float txtAlpha = fontColor.norm.w;

	SDL_Surface *txtSurf;

	if (shState->rtData().config.solidFonts)
		txtSurf = TTF_RenderUTF8_Solid(font, str, c);
	else
		txtSurf = TTF_RenderUTF8_Blended(font, str, c);

	p->ensureFormat(txtSurf, SDL_PIXELFORMAT_ABGR8888);

	int rawTxtSurfH = txtSurf->h;

	if (p->font->getShadow())
		applyShadow(txtSurf, *p->format, c);

	/* outline using TTF_Outline and blending it together with SDL_BlitSurface
	 * FIXME: outline is forced to have the same opacity as the font color */
	if (p->font->getOutline())
	{
		SDL_Color co = outColor.toSDLColor();
		co.a = 255;
		SDL_Surface *outline;
		/* set the next font render to render the outline */
		TTF_SetFontOutline(font, OUTLINE_SIZE);
		if (shState->rtData().config.solidFonts)
			outline = TTF_RenderUTF8_Solid(font, str, co);
		else
			outline = TTF_RenderUTF8_Blended(font, str, co);

		p->ensureFormat(outline, SDL_PIXELFORMAT_ABGR8888);
		SDL_Rect outRect = {OUTLINE_SIZE, OUTLINE_SIZE, txtSurf->w, txtSurf->h}; 

		SDL_SetSurfaceBlendMode(txtSurf, SDL_BLENDMODE_BLEND);
		SDL_BlitSurface(txtSurf, NULL, outline, &outRect);
		SDL_FreeSurface(txtSurf);
		txtSurf = outline;
		/* reset outline to 0 */
		TTF_SetFontOutline(font, 0);
	}

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

	int alignY = rect.y + (rect.h - rawTxtSurfH) / 2;

	float squeeze = (float) rect.w / txtSurf->w;

	if (squeeze > 1)
		squeeze = 1;

	FloatRect posRect(alignX, alignY, txtSurf->w * squeeze, txtSurf->h);

	Vec2i gpTexSize;
	shState->ensureTexSize(txtSurf->w, txtSurf->h, gpTexSize);

	bool fastBlit = false; //!p->touchesTaintedArea(posRect) && txtAlpha == 1.0f;

	if (fastBlit)
	{
		if (squeeze == 1.0f && !shState->config().subImageFix)
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
				bool subImage = false;
				int subSrcX = 0, subSrcY = 0;

				if (inters.w != txtRect.w || inters.h != txtRect.h)
				{
					/* Clip the text surface */
					subSrcX = inters.x - txtRect.x;
					subSrcY = inters.y - txtRect.y;
					subImage = true;

					posRect.x = inters.x;
					posRect.y = inters.y;
					posRect.w = inters.w;
					posRect.h = inters.h;
				}

				TEX::bind(p->gl.tex);

				if (!subImage)
				{
					TEX::uploadSubImage(posRect.x, posRect.y,
					                    posRect.w, posRect.h,
					                    txtSurf->pixels, GL_RGBA);
				}
				else
				{
					GLMeta::subRectImageUpload(txtSurf->w, subSrcX, subSrcY,
					                           posRect.x, posRect.y,
					                           posRect.w, posRect.h,
					                           txtSurf, GL_RGBA);
					GLMeta::subRectImageEnd();
				}
			}
		}
		else
		{
			/* Squeezing involved: need to use intermediary TexFBO */
			TEXFBO &gpTF = shState->gpTexFBO(txtSurf->w, txtSurf->h);

			TEX::bind(gpTF.tex);
			TEX::uploadSubImage(0, 0, txtSurf->w, txtSurf->h, txtSurf->pixels, GL_RGBA);

			GLMeta::blitBegin(p->gl);
			GLMeta::blitSource(gpTF);
			GLMeta::blitRectangle(IntRect(0, 0, txtSurf->w, txtSurf->h),
			                      posRect, true);
			GLMeta::blitEnd();
		}
	}
	else
	{
		/* Aquire a partial copy of the destination
		 * buffer we're about to render to */
		TEXFBO &gpTex2 = shState->gpTexFBO(posRect.w, posRect.h);

		GLMeta::blitBegin(gpTex2);
		GLMeta::blitSource(p->gl);
		GLMeta::blitRectangle(posRect, Vec2i());
		GLMeta::blitEnd();

		FloatRect bltRect(0, 0,
		                  (float) (gpTexSize.x * squeeze) / gpTex2.width,
		                  (float) gpTexSize.y / gpTex2.height);

		BltShader &shader = shState->shaders().blt;
		shader.bind();
		shader.setTexSize(gpTexSize);
		shader.setSource();
		shader.setDestination(gpTex2.tex);
		shader.setSubRect(bltRect);
		shader.setOpacity(txtAlpha);

		shState->bindTex();
		TEX::uploadSubImage(0, 0, txtSurf->w, txtSurf->h, txtSurf->pixels, GL_RGBA);
		TEX::setSmooth(true);

		Quad &quad = shState->gpQuad();
		quad.setTexRect(FloatRect(0, 0, txtSurf->w, txtSurf->h));
		quad.setPosRect(posRect);

		p->bindFBO();
		p->pushSetViewport(shader);

		p->blitQuad(quad);

		p->popViewport();
	}

	SDL_FreeSurface(txtSurf);
	p->addTaintedArea(posRect);

	p->onModified();
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
	guardDisposed();

	GUARD_MEGA;

	TTF_Font *font = p->font->getSdlFont();

	std::string fixed = fixupString(str);
	str = fixed.c_str();

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

DEF_ATTR_RD_SIMPLE(Bitmap, Font, Font&, *p->font)

void Bitmap::setFont(Font &value)
{
	*p->font = value;
}

void Bitmap::setInitFont(Font *value)
{
	p->font = value;
}

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

void Bitmap::taintArea(const IntRect &rect)
{
	p->addTaintedArea(rect);
}

void Bitmap::releaseResources()
{
	if (p->megaSurface)
		SDL_FreeSurface(p->megaSurface);
	else
		shState->texPool().release(p->gl);

	delete p;
}
