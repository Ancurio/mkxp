/*
** graphics.cpp
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

#include "graphics.h"

#include "util.h"
#include "gl-util.h"
#include "globalstate.h"
#include "glstate.h"
#include "shader.h"
#include "scene.h"
#include "quad.h"
#include "eventthread.h"
#include "texpool.h"
#include "bitmap.h"
#include "etc-internal.h"
#include "binding.h"

#include "SDL2/SDL_video.h"
#include "SDL2/SDL_timer.h"

#include "stdint.h"

#include "SFML/System/Clock.hpp"

struct TimerQuery
{
	GLuint query;
	static bool queryActive;
	bool thisQueryActive;

	TimerQuery()
	    : thisQueryActive(false)
	{
		glGenQueries(1, &query);
	}

	void begin()
	{
		if (queryActive)
			return;

		if (thisQueryActive)
			return;

		glBeginQuery(GL_TIME_ELAPSED, query);
		queryActive = true;
		thisQueryActive = true;
	}

	void end()
	{
		if (!thisQueryActive)
			return;

		glEndQuery(GL_TIME_ELAPSED);
		queryActive = false;
		thisQueryActive = false;
	}

	bool getResult(GLuint64 *result)
	{
		if (thisQueryActive)
			return false;

		GLint isReady;
		glGetQueryObjectiv(query, GL_QUERY_RESULT_AVAILABLE, &isReady);

		if (isReady != GL_TRUE)
		{
//			qDebug() << "TimerQuery result not ready";
			return false;
		}

		glGetQueryObjectui64v(query, GL_QUERY_RESULT, result);

		if (glGetError() == GL_INVALID_OPERATION)
		{
			qDebug() << "Something went wrong with getting TimerQuery results";
			return false;
		}

		return true;
	}

	GLuint64 getResultSync()
	{
		if (thisQueryActive)
			return 0;

		GLuint64 result;
		GLint isReady = GL_FALSE;

		while (isReady == GL_FALSE)
			glGetQueryObjectiv(query, GL_QUERY_RESULT_AVAILABLE, &isReady);

		glGetQueryObjectui64v(query, GL_QUERY_RESULT, &result);

		return result;
	}

	~TimerQuery()
	{
		if (thisQueryActive)
			end();

		glDeleteQueries(1, &query);
	}
};

bool TimerQuery::queryActive = false;

struct GPUTimer
{
	TimerQuery queries[2];
	const int iter;

	uchar ind;
	uint64_t acc;
	int32_t counter;
	bool first;

	GPUTimer(int iter)
	    : iter(iter),
	      ind(0),
	      acc(0),
	      counter(0),
	      first(true)
	{}

	void startTiming()
	{
		queries[ind].begin();
	}

	void endTiming()
	{
		queries[ind].end();

		if (first)
		{
			first = false;
			return;
		}

		swapInd();

		GLuint64 result;
		if (!queries[ind].getResult(&result))
			return;

		acc += result;

		if (++counter < iter)
			return;

		qDebug() << "  Avg. GPU time:" << ((double) acc / (iter * 1000 * 1000)) << "ms";
		acc = counter = 0;
	}

	void swapInd()
	{
		ind = ind ? 0 : 1;
	}
};

struct CPUTimer
{
	const int iter;

	uint64_t acc;
	int32_t counter;
	sf::Clock clock;

	CPUTimer(int iter)
	    : iter(iter),
	      acc(0),
	      counter(0)
	{
	}

	void startTiming()
	{
		clock.restart();
	}

	void endTiming()
	{
		acc += clock.getElapsedTime().asMicroseconds();

		if (++counter < iter)
			return;

		qDebug() << "Avg. CPU time:" << ((double) acc / (iter * 1000)) << "ms";
		acc = counter = 0;
	}
};

struct PingPong
{
	TEXFBO rt[2];
	unsigned srcInd, dstInd;
	int screenW, screenH;

	PingPong(int screenW, int screenH)
	    : srcInd(0), dstInd(1),
	      screenW(screenW), screenH(screenH)
	{
		for (int i = 0; i < 2; ++i)
		{
			TEXFBO::init(rt[i]);
			TEXFBO::allocEmpty(rt[i], screenW, screenH);
			TEXFBO::linkFBO(rt[i]);
			glClearColor(0, 0, 0, 1);
			glClear(GL_COLOR_BUFFER_BIT);
		}
	}

	~PingPong()
	{
		for (int i = 0; i < 2; ++i)
			TEXFBO::fini(rt[i]);
	}

	/* Binds FBO of last good buffer for reading */
	void bindLastBuffer()
	{
		FBO::bind(rt[dstInd].fbo, FBO::Read);
	}

	/* Better not call this during render cycles */
	void resize(int width, int height)
	{
		screenW = width;
		screenH = height;
		for (int i = 0; i < 2; ++i)
		{
			TEX::bind(rt[i].tex);
			TEX::allocEmpty(width, height);
		}
	}

	void startRender()
	{
		bind();
	}

	void swapRender()
	{
		swapIndices();

		/* Discard dest buffer */
		TEX::bind(rt[dstInd].tex);
		TEX::allocEmpty(screenW, screenH);

		bind();
	}

	void blitFBOs()
	{
		FBO::blit(0, 0, 0, 0, screenW, screenH);
	}

	void finishRender()
	{
		FBO::unbind(FBO::Draw);
		FBO::bind(rt[dstInd].fbo, FBO::Read);
	}

private:
	void bind()
	{
		TEX::bind(rt[srcInd].tex);
		FBO::bind(rt[srcInd].fbo, FBO::Read);
		FBO::bind(rt[dstInd].fbo, FBO::Draw);
	}

	void swapIndices()
	{
		unsigned tmp = srcInd;
		srcInd = dstInd;
		dstInd = tmp;
	}
};

class ScreenScene : public Scene
{
public:
	ScreenScene(int width, int height)
	    : pp(width, height),
	      brightEffect(false),
	      actW(width), actH(height)
	{
		updateReso(width, height);
		brightnessQuad.setColor(Vec4());
	}

	void composite()
	{
		const int w = geometry.rect.w;
		const int h = geometry.rect.h;

		gState->prepareDraw();

		pp.startRender();

		glState.viewport.set(IntRect(0, 0, w, h));

		glClear(GL_COLOR_BUFFER_BIT);

		Scene::composite();

		if (brightEffect)
		{
			SimpleColorShader &shader = gState->simpleColorShader();
			shader.bind();
			shader.applyViewportProj();
			shader.setTranslation(Vec2i());

			brightnessQuad.draw();
		}

		pp.finishRender();
	}

	void requestViewportRender(Vec4 &c, Vec4 &f, Vec4 &t)
	{
		pp.swapRender();
		pp.blitFBOs();

		PlaneShader &shader = gState->planeShader();
		shader.bind();
		shader.setColor(c);
		shader.setFlash(f);
		shader.setTone(t);
		shader.applyViewportProj();
		shader.setTexSize(geometry.rect.size());

		glState.blendMode.pushSet(BlendNone);

		screenQuad.draw();

		glState.blendMode.pop();
		shader.unbind();
	}

	void setBrightness(float norm)
	{
		brightnessQuad.setColor(Vec4(0, 0, 0, 1.0 - norm));

		brightEffect = norm < 1.0;
	}

	void updateReso(int width, int height)
	{
		geometry.rect.w = width;
		geometry.rect.h = height;

		screenQuad.setTexPosRect(geometry.rect, geometry.rect);
		brightnessQuad.setTexPosRect(geometry.rect, geometry.rect);

		notifyGeometryChange();
	}

	void setResolution(int width, int height)
	{
		pp.resize(width, height);
		updateReso(width, height);
	}

	void setScreenSize(int width, int height)
	{
		actW = width;
		actH = height;
	}

	PingPong &getPP()
	{
		return pp;
	}

private:
	PingPong pp;
	Quad screenQuad;
	Quad brightnessQuad;
	bool brightEffect;
	int actW, actH;
};

struct FPSLimiter
{
	unsigned lastTickCount;
	unsigned mspf; /* ms per frame */

	FPSLimiter(unsigned desiredFPS)
	    : lastTickCount(SDL_GetTicks())
	{
		setDesiredFPS(desiredFPS);
	}

	void setDesiredFPS(unsigned value)
	{
		mspf = 1000 / value;
	}

	void delay()
	{
		unsigned tmpTicks = SDL_GetTicks();
		unsigned tickDelta = tmpTicks - lastTickCount;
		lastTickCount = tmpTicks;

		int toDelay = mspf - tickDelta;
		if (toDelay < 0)
			toDelay = 0;

		SDL_Delay(toDelay);
		lastTickCount = SDL_GetTicks();
	}
};

struct Timer
{
	uint64_t lastTicks;
	uint64_t acc;
	int counter;

	Timer()
	    : lastTicks(SDL_GetPerformanceCounter()),
	      acc(0),
	      counter(0)
	{}
};

struct GraphicsPrivate
{
	/* Screen resolution */
	Vec2i scRes;
	/* Actual screen size */
	Vec2i scSize;

	ScreenScene screen;
	RGSSThreadData *threadData;

	int frameRate;
	int frameCount;
	int brightness;

	FPSLimiter fpsLimiter;

	GPUTimer gpuTimer;
	CPUTimer cpuTimer;

	bool frozen;
	TEXFBO frozenScene;
	TEXFBO currentScene;
	Quad screenQuad;
	RBOFBO transBuffer;

	GraphicsPrivate()
	    : scRes(640, 480),
	      scSize(scRes),
	      screen(scRes.x, scRes.y),
	      frameRate(40),
	      frameCount(0),
	      brightness(255),
	      fpsLimiter(frameRate),
	      gpuTimer(frameRate),
	      cpuTimer(frameRate),
	      frozen(false)
	{
		TEXFBO::init(frozenScene);
		TEXFBO::allocEmpty(frozenScene, scRes.x, scRes.y);
		TEXFBO::linkFBO(frozenScene);

		TEXFBO::init(currentScene);
		TEXFBO::allocEmpty(currentScene, scRes.x, scRes.y);
		TEXFBO::linkFBO(currentScene);

		FloatRect screenRect(0, 0, scRes.x, scRes.y);
		screenQuad.setTexPosRect(screenRect, screenRect);

		RBOFBO::init(transBuffer);
		RBOFBO::allocEmpty(transBuffer, scRes.x, scRes.y);
		RBOFBO::linkFBO(transBuffer);
	}

	~GraphicsPrivate()
	{
		TEXFBO::fini(frozenScene);
		TEXFBO::fini(currentScene);

		RBOFBO::fini(transBuffer);
	}

	void updateScreenResoRatio()
	{
		Vec2 &ratio = gState->rtData().sizeResoRatio;
		ratio.x = (float) scRes.x / scSize.x;
		ratio.y = (float) scRes.y / scSize.y;
	}

	void checkResize()
	{
		if (threadData->windowSizeMsg.pollChange(&scSize.x, &scSize.y))
		{
			screen.setScreenSize(scSize.x, scSize.y);
			updateScreenResoRatio();
		}
	}

	void shutdown()
	{
		threadData->rqTermAck = true;
		gState->texPool().disable();

		scriptBinding->terminate();
	}

	void swapGLBuffer()
	{
		SDL_GL_SwapWindow(threadData->window);
		fpsLimiter.delay();

		++frameCount;
	}

	void compositeToBuffer(FBO::ID fbo)
	{
		screen.composite();
		FBO::bind(fbo, FBO::Draw);
		FBO::blit(0, 0, 0, 0, scRes.x, scRes.y);
	}

	void blitBuffer()
	{
		FBO::blit(0, 0, 0, 0, scRes.x, scRes.y);
	}

	void blitBufferScaled()
	{
		FBO::blit(0, 0, scRes.x, scRes.y, 0, 0, scSize.x, scSize.y);
	}

	void blitBufferFlippedScaled()
	{
		FBO::blit(0, 0, scRes.x, scRes.y, 0, scSize.y, scSize.x, -scSize.y);
	}

	/* Blits currently bound read FBO to screen (upside-down) */
	void blitToScreen()
	{
		FBO::unbind(FBO::Draw);
		glClear(GL_COLOR_BUFFER_BIT);
		blitBufferFlippedScaled();
	}

	void redrawScreen()
	{
		screen.composite();
		blitToScreen();

		swapGLBuffer();
	}
};

Graphics::Graphics(RGSSThreadData *data)
{
	p = new GraphicsPrivate;
	p->threadData = data;
}

Graphics::~Graphics()
{
	delete p;
}

void Graphics::update()
{
	gState->checkShutdown();

//	p->cpuTimer.endTiming();
//	p->gpuTimer.startTiming();

	if (p->frozen)
		return;

	p->checkResize();
	p->redrawScreen();

//	p->gpuTimer.endTiming();
//	p->cpuTimer.startTiming();
}

void Graphics::wait(int duration)
{
	for (int i = 0; i < duration; ++i)
	{
		gState->checkShutdown();
		p->checkResize();
		p->redrawScreen();
	}
}

void Graphics::fadeout(int duration)
{
	if (p->frozen)
		FBO::bind(p->frozenScene.fbo, FBO::Read);

	for (int i = duration-1; i > -1; --i)
	{
		setBrightness((255.0 / duration) * i);

		if (p->frozen)
		{
			p->blitToScreen();
			p->swapGLBuffer();
		}
		else
		{
			update();
		}
	}
}

void Graphics::fadein(int duration)
{
	if (p->frozen)
		FBO::bind(p->frozenScene.fbo, FBO::Read);

	for (int i = 0; i < duration; ++i)
	{
		setBrightness((255.0 / duration) * i);

		if (p->frozen)
		{
			p->blitToScreen();
			p->swapGLBuffer();
		}
		else
		{
			update();
		}
	}
}

void Graphics::freeze()
{
	p->frozen = true;

	gState->checkShutdown();
	p->checkResize();

	/* Capture scene into frozen buffer */
	p->compositeToBuffer(p->frozenScene.fbo);
}

void Graphics::transition(int duration,
                          const char *filename,
                          int vague)
{
	vague = clamp(vague, 0, 512);
	Bitmap *transMap = filename ? new Bitmap(filename) : 0;

	setBrightness(255);

	/* Capture new scene */
	p->compositeToBuffer(p->currentScene.fbo);

	/* If no transition bitmap is provided,
	 * we can use a simplified shader */
	if (transMap)
	{
		TransShader &shader = gState->transShader();
		shader.bind();
		shader.applyViewportProj();
		shader.setFrozenScene(p->frozenScene.tex);
		shader.setCurrentScene(p->currentScene.tex);
		shader.setTransMap(transMap->getGLTypes().tex);
		shader.setVague(vague / 512.0f);
		shader.setTexSize(p->scRes);
	}
	else
	{
		SimpleTransShader &shader = gState->sTransShader();
		shader.bind();
		shader.applyViewportProj();
		shader.setFrozenScene(p->frozenScene.tex);
		shader.setCurrentScene(p->currentScene.tex);
		shader.setTexSize(p->scRes);
	}

	glState.blendMode.pushSet(BlendNone);

	for (int i = 0; i < duration; ++i)
	{
		if (p->threadData->rqTerm)
		{
			delete transMap;
			p->shutdown();
		}

		const float prog = i * (1.0 / duration);

		if (transMap)
			gState->transShader().setProg(prog);
		else
			gState->sTransShader().setProg(prog);

		/* Draw the composed frame to a buffer first
		 * (we need this because we're skipping PingPong) */
		FBO::bind(p->transBuffer.fbo, FBO::Draw);
		glClear(GL_COLOR_BUFFER_BIT);
		p->screenQuad.draw();

		p->checkResize();

		/* Then blit it flipped and scaled to the screen */
		FBO::bind(p->transBuffer.fbo, FBO::Read);
		p->blitToScreen();

		p->swapGLBuffer();
	}

	glState.blendMode.pop();

	delete transMap;

	p->frozen = false;
}

Bitmap *Graphics::snapToBitmap()
{
	Bitmap *bitmap = new Bitmap(width(), height());

	p->compositeToBuffer(bitmap->getGLTypes().fbo);

	return bitmap;
}

void Graphics::frameReset()
{

}

int Graphics::width() const
{
	return p->scRes.x;
}

int Graphics::height() const
{
	return p->scRes.y;
}

void Graphics::resizeScreen(int width, int height)
{
	width = clamp(width, 1, 640);
	height = clamp(height, 1, 480);

	Vec2i size(width, height);

	if (p->scRes == size)
		return;

	gState->eThread().requestWindowResize(width, height);

	p->scRes = size;

	p->screen.setResolution(width, height);

	TEX::bind(p->frozenScene.tex);
	TEX::allocEmpty(width, height);
	TEX::bind(p->currentScene.tex);
	TEX::allocEmpty(width, height);

	FloatRect screenRect(0, 0, width, height);
	p->screenQuad.setTexPosRect(screenRect, screenRect);

	RBO::bind(p->transBuffer.rbo);
	RBO::allocEmpty(width, height);

	p->updateScreenResoRatio();
}

#undef RET_IF_DISP
#define RET_IF_DISP(x)

#undef CHK_DISP
#define CHK_DISP

DEF_ATTR_RD_SIMPLE(Graphics, FrameRate, int, p->frameRate)
DEF_ATTR_RD_SIMPLE(Graphics, Brightness, int, p->brightness)

DEF_ATTR_SIMPLE(Graphics, FrameCount, int, p->frameCount)

void Graphics::setFrameRate(int value)
{
	p->frameRate = clamp(value, 10, 120);
	p->fpsLimiter.setDesiredFPS(p->frameRate);
}

void Graphics::setBrightness(int value)
{
	value = clamp(value, 0, 255);

	if (p->brightness == value)
		return;

	p->brightness = value;
	p->screen.setBrightness(value / 255.0);
}

bool Graphics::getFullscreen() const
{
	return p->threadData->ethread->getFullscreen();
}

void Graphics::setFullscreen(bool value)
{
	p->threadData->ethread->requestFullscreenMode(value);
}

Scene *Graphics::getScreen() const
{
	return &p->screen;
}

void Graphics::repaintWait(volatile bool *exitCond)
{
	if (*exitCond)
		return;

	/* Repaint the screen with the last good frame we drew */
	p->screen.getPP().bindLastBuffer();
	FBO::unbind(FBO::Draw);

	while (!*exitCond)
	{
		gState->checkShutdown();

		glClear(GL_COLOR_BUFFER_BIT);
		p->blitBufferFlippedScaled();
		SDL_GL_SwapWindow(p->threadData->window);
		p->fpsLimiter.delay();
	}
}
