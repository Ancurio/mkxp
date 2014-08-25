/*
** sharedstate.cpp
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

#include "sharedstate.h"

#include "util.h"
#include "filesystem.h"
#include "graphics.h"
#include "input.h"
#include "audio.h"
#include "glstate.h"
#include "shader.h"
#include "texpool.h"
#include "font.h"
#include "eventthread.h"
#include "gl-util.h"
#include "global-ibo.h"
#include "quad.h"
#include "binding.h"
#include "exception.h"

#ifdef MIDI
#include "sharedmidistate.h"
#endif

#include <unistd.h>
#include <stdio.h>
#include <string>

SharedState *SharedState::instance = 0;
static GlobalIBO *_globalIBO = 0;

#ifdef RGSS3
#define GAME_ARCHIVE "Game.rgss3a"
#elif RGSS2
#define GAME_ARCHIVE "Game.rgss2a"
#else
#define GAME_ARCHIVE "Game.rgssad"
#endif

struct SharedStatePrivate
{
	void *bindingData;
	SDL_Window *sdlWindow;
	Scene *screen;

	FileSystem fileSystem;

	EventThread &eThread;
	RGSSThreadData &rtData;
	Config &config;

#ifdef MIDI
	SharedMidiState midiState;
#endif

	Graphics graphics;
	Input input;
	Audio audio;

	GLState _glState;

	ShaderSet shaders;

	TexPool texPool;

	SharedFontState fontState;
	Font *defaultFont;

	TEX::ID globalTex;
	int globalTexW, globalTexH;

	TEXFBO gpTexFBO;

	TEXFBO atlasTex;

	Quad gpQuad;

	unsigned int stampCounter;

	SharedStatePrivate(RGSSThreadData *threadData)
	    : bindingData(0),
	      sdlWindow(threadData->window),
	      fileSystem(threadData->argv0, threadData->config.allowSymlinks),
	      eThread(*threadData->ethread),
	      rtData(*threadData),
	      config(threadData->config),
#ifdef MIDI
	      midiState(threadData->config),
#endif
	      graphics(threadData),
	      audio(threadData->config),
	      fontState(threadData->config),
	      stampCounter(0)
	{
		if (!config.gameFolder.empty())
		{
			int result = chdir(config.gameFolder.c_str());

			if (result != 0)
				throw Exception(Exception::MKXPError,
			                    "Unable to switch into gameFolder '%s'",
			                    config.gameFolder.c_str());
		}

		// FIXME find out correct archive filename
		std::string archPath = GAME_ARCHIVE;

		/* Check if a game archive exists */
		FILE *tmp = fopen(archPath.c_str(), "r");
		if (tmp)
		{
			fileSystem.addPath(archPath.c_str());
			fclose(tmp);
		}

		fileSystem.addPath(".");

		for (size_t i = 0; i < config.rtps.size(); ++i)
			fileSystem.addPath(config.rtps[i].c_str());

		if (config.pathCache)
			fileSystem.createPathCache();

		fileSystem.initFontSets(fontState);

		globalTexW = 128;
		globalTexH = 64;

		globalTex = TEX::gen();
		TEX::bind(globalTex);
		TEX::setRepeat(false);
		TEX::setSmooth(false);
		TEX::allocEmpty(globalTexW, globalTexH);

		TEXFBO::init(gpTexFBO);
		/* Reuse starting values */
		TEXFBO::allocEmpty(gpTexFBO, globalTexW, globalTexH);
		TEXFBO::linkFBO(gpTexFBO);

		/* RGSS3 games will call setup_midi, so there's
		 * no need to do it on startup */
#if MIDI && !RGSS3
		midiState.initDefaultSynths();
#endif
	}

	~SharedStatePrivate()
	{
		TEX::del(globalTex);
		TEXFBO::fini(gpTexFBO);
		TEXFBO::fini(atlasTex);
	}
};

void SharedState::initInstance(RGSSThreadData *threadData)
{
	/* This section is tricky because of dependencies:
	 * SharedState depends on GlobalIBO existing,
	 * Font depends on SharedState existing */

	_globalIBO = new GlobalIBO();
	_globalIBO->ensureSize(1);

	SharedState::instance = 0;
	Font *defaultFont = 0;

	try
	{
		SharedState::instance = new SharedState(threadData);
		defaultFont = new Font();
	}
	catch (const Exception &exc)
	{
		delete _globalIBO;
		delete SharedState::instance;
		delete defaultFont;

		throw exc;
	}

	SharedState::instance->p->defaultFont = defaultFont;
}

void SharedState::finiInstance()
{
	delete SharedState::instance->p->defaultFont;

	delete SharedState::instance;

	delete _globalIBO;
}

void SharedState::setScreen(Scene &screen)
{
	p->screen = &screen;
}

#define GSATT(type, lower) \
	type SharedState :: lower() const \
	{ \
		return p->lower; \
	}

GSATT(void*, bindingData)
GSATT(SDL_Window*, sdlWindow)
GSATT(Scene*, screen)
GSATT(FileSystem&, fileSystem)
GSATT(EventThread&, eThread)
GSATT(RGSSThreadData&, rtData)
GSATT(Config&, config)
GSATT(Graphics&, graphics)
GSATT(Input&, input)
GSATT(Audio&, audio)
GSATT(GLState&, _glState)
GSATT(ShaderSet&, shaders)
GSATT(TexPool&, texPool)
GSATT(Quad&, gpQuad)
GSATT(SharedFontState&, fontState)

#ifdef MIDI
GSATT(SharedMidiState&, midiState)
#endif

void SharedState::setBindingData(void *data)
{
	p->bindingData = data;
}

void SharedState::ensureQuadIBO(size_t minSize)
{
	_globalIBO->ensureSize(minSize);
}

GlobalIBO &SharedState::globalIBO()
{
	return *_globalIBO;
}

void SharedState::bindTex()
{
	TEX::bind(p->globalTex);
	TEX::allocEmpty(p->globalTexW, p->globalTexH);
}

void SharedState::ensureTexSize(int minW, int minH, Vec2i &currentSizeOut)
{
	if (minW > p->globalTexW)
		p->globalTexW = findNextPow2(minW);

	if (minH > p->globalTexH)
		p->globalTexH = findNextPow2(minH);

	currentSizeOut = Vec2i(p->globalTexW, p->globalTexH);
}

TEXFBO &SharedState::gpTexFBO(int minW, int minH)
{
	bool needResize = false;

	if (minW > p->gpTexFBO.width)
	{
		p->gpTexFBO.width = findNextPow2(minW);
		needResize = true;
	}

	if (minH > p->gpTexFBO.height)
	{
		p->gpTexFBO.height = findNextPow2(minH);
		needResize = true;
	}

	if (needResize)
	{
		TEX::bind(p->gpTexFBO.tex);
		TEX::allocEmpty(p->gpTexFBO.width, p->gpTexFBO.height);
	}

	return p->gpTexFBO;
}

void SharedState::requestAtlasTex(int w, int h, TEXFBO &out)
{
	TEXFBO tex;

	if (w == p->atlasTex.width && h == p->atlasTex.height)
	{
		tex = p->atlasTex;
		p->atlasTex = TEXFBO();
	}
	else
	{
		TEXFBO::init(tex);
		TEXFBO::allocEmpty(tex, w, h);
		TEXFBO::linkFBO(tex);
	}

	out = tex;
}

void SharedState::releaseAtlasTex(TEXFBO &tex)
{
	/* No point in caching an invalid object */
	if (tex.tex == TEX::ID(0))
		return;

	TEXFBO::fini(p->atlasTex);

	p->atlasTex = tex;
}

void SharedState::checkShutdown()
{
	if (!p->rtData.rqTerm)
		return;

	p->rtData.rqTermAck = true;
	p->texPool.disable();
	scriptBinding->terminate();
}

Font &SharedState::defaultFont() const
{
	return *p->defaultFont;
}

unsigned int SharedState::genTimeStamp()
{
	return p->stampCounter++;
}

SharedState::SharedState(RGSSThreadData *threadData)
{
	p = new SharedStatePrivate(threadData);
	p->screen = p->graphics.getScreen();
}

SharedState::~SharedState()
{
	delete p;
}
