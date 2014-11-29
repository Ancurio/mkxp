/*
** main.cpp
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

#include <alc.h>

#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>
#include <SDL_sound.h>

#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <string>

#include "sharedstate.h"
#include "eventthread.h"
#include "gl-debug.h"
#include "debugwriter.h"
#include "exception.h"
#include "gl-fun.h"

#include "binding.h"

#include "icon.png.xxd"

static void
rgssThreadError(RGSSThreadData *rtData, const std::string &msg)
{
	rtData->rgssErrorMsg = msg;
	rtData->ethread->requestTerminate();
	rtData->rqTermAck.set();
}

static inline const char*
glGetStringInt(GLenum name)
{
	return (const char*) gl.GetString(name);
}

static void
printGLInfo()
{
	Debug() << "GL Vendor    :" << glGetStringInt(GL_VENDOR);
	Debug() << "GL Renderer  :" << glGetStringInt(GL_RENDERER);
	Debug() << "GL Version   :" << glGetStringInt(GL_VERSION);
	Debug() << "GLSL Version :" << glGetStringInt(GL_SHADING_LANGUAGE_VERSION);
}

int rgssThreadFun(void *userdata)
{
	RGSSThreadData *threadData = static_cast<RGSSThreadData*>(userdata);
	SDL_Window *win = threadData->window;
	SDL_GLContext glCtx;

	/* Setup GL context */
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

	if (threadData->config.debugMode)
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);

	glCtx = SDL_GL_CreateContext(win);

	if (!glCtx)
	{
		rgssThreadError(threadData, std::string("Error creating context: ") + SDL_GetError());
		return 0;
	}

	try
	{
		initGLFunctions();
	}
	catch (const Exception &exc)
	{
		rgssThreadError(threadData, exc.msg);
		SDL_GL_DeleteContext(glCtx);

		return 0;
	}

	gl.ClearColor(0, 0, 0, 1);
	gl.Clear(GL_COLOR_BUFFER_BIT);
	SDL_GL_SwapWindow(win);

	printGLInfo();

	SDL_GL_SetSwapInterval(threadData->config.vsync ? 1 : 0);

	GLDebugLogger dLogger;

	/* Setup AL context */
	ALCdevice *alcDev = alcOpenDevice(0);

	if (!alcDev)
	{
		rgssThreadError(threadData, "Error opening OpenAL device");
		SDL_GL_DeleteContext(glCtx);

		return 0;
	}

	ALCcontext *alcCtx = alcCreateContext(alcDev, 0);

	if (!alcCtx)
	{
		rgssThreadError(threadData, "Error creating OpenAL context");
		alcCloseDevice(alcDev);
		SDL_GL_DeleteContext(glCtx);

		return 0;
	}

	alcMakeContextCurrent(alcCtx);

	try
	{
		SharedState::initInstance(threadData);
	}
	catch (const Exception &exc)
	{
		rgssThreadError(threadData, exc.msg);
		alcDestroyContext(alcCtx);
		alcCloseDevice(alcDev);
		SDL_GL_DeleteContext(glCtx);

		return 0;
	}

	/* Start script execution */
	scriptBinding->execute();

	threadData->rqTermAck.set();
	threadData->ethread->requestTerminate();

	SharedState::finiInstance();

	alcDestroyContext(alcCtx);
	alcCloseDevice(alcDev);

	SDL_GL_DeleteContext(glCtx);

	return 0;
}

static void printRgssVersion(int ver)
{
	const char *const makers[] =
		{ "", "XP", "VX", "VX Ace" };

	char buf[128];
	snprintf(buf, sizeof(buf), "RGSS version %d (%s)", ver, makers[ver]);

	Debug() << buf;
}

int main(int argc, char *argv[])
{
	/* initialize SDL first */
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK) < 0)
	{
		Debug() << "Error initializing SDL:" << SDL_GetError();

		return 0;
	}

	if (!EventThread::allocUserEvents())
	{
		Debug() << "Error allocating SDL user events";

		return 0;
	}

#ifndef WORKDIR_CURRENT
	/* set working directory */
	char *dataDir = SDL_GetBasePath();
	if (dataDir)
	{
		int result = chdir(dataDir);
		(void)result;
		SDL_free(dataDir);
	}
#endif

	/* now we load the config */
	Config conf;

	conf.read(argc, argv);
	conf.readGameINI();

	assert(conf.rgssVersion >= 1 && conf.rgssVersion <= 3);
	printRgssVersion(conf.rgssVersion);

	int imgFlags = IMG_INIT_PNG | IMG_INIT_JPG;
	if (IMG_Init(imgFlags) != imgFlags)
	{
		Debug() << "Error initializing SDL_image:" << SDL_GetError();
		SDL_Quit();

		return 0;
	}

	if (TTF_Init() < 0)
	{
		Debug() << "Error initializing SDL_ttf:" << SDL_GetError();
		IMG_Quit();
		SDL_Quit();

		return 0;
	}

	if (Sound_Init() == 0)
	{
		Debug() << "Error initializing SDL_sound:" << Sound_GetError();
		TTF_Quit();
		IMG_Quit();
		SDL_Quit();

		return 0;
	}

	/* Setup application icon */
	SDL_RWops *iconSrc;

	if (conf.iconPath.empty())
		iconSrc = SDL_RWFromConstMem(assets_icon_png, assets_icon_png_len);
	else
		iconSrc = SDL_RWFromFile(conf.iconPath.c_str(), "rb");

	SDL_Surface *iconImg = IMG_Load_RW(iconSrc, SDL_TRUE);

	SDL_SetHint("SDL_VIDEO_MINIMIZE_ON_FOCUS_LOSS", "0");

	SDL_Window *win;
	Uint32 winFlags = SDL_WINDOW_OPENGL | SDL_WINDOW_INPUT_FOCUS;

	if (conf.winResizable)
		winFlags |= SDL_WINDOW_RESIZABLE;
	if (conf.fullscreen)
		winFlags |= SDL_WINDOW_FULLSCREEN_DESKTOP;

	win = SDL_CreateWindow(conf.game.title.c_str(),
	                       SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
	                       conf.defScreenW, conf.defScreenH, winFlags);

	if (!win)
	{
		Debug() << "Error creating window:" << SDL_GetError();
		return 0;
	}

	if (iconImg)
	{
		SDL_SetWindowIcon(win, iconImg);
		SDL_FreeSurface(iconImg);
	}

	EventThread eventThread;
	RGSSThreadData rtData(&eventThread, argv[0], win, conf);

	/* Load and post key bindings */
	rtData.bindingUpdateMsg.post(loadBindings(conf));

	/* Start RGSS thread */
	SDL_Thread *rgssThread =
	        SDL_CreateThread(rgssThreadFun, "rgss", &rtData);

	/* Start event processing */
	eventThread.process(rtData);

	/* Request RGSS thread to stop */
	rtData.rqTerm.set();

	/* Wait for RGSS thread response */
	for (int i = 0; i < 1000; ++i)
	{
		/* We can stop waiting when the request was ack'd */
		if (rtData.rqTermAck)
		{
			Debug() << "RGSS thread ack'd request after" << i*10 << "ms";
			break;
		}

		/* Give RGSS thread some time to respond */
		SDL_Delay(10);
	}

	/* If RGSS thread ack'd request, wait for it to shutdown,
	 * otherwise abandon hope and just end the process as is. */
	if (rtData.rqTermAck)
		SDL_WaitThread(rgssThread, 0);
	else
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, conf.game.title.c_str(),
		                         "The RGSS script seems to be stuck and mkxp will now force quit", win);

	if (!rtData.rgssErrorMsg.empty())
	{
		Debug() << rtData.rgssErrorMsg;
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, conf.game.title.c_str(),
		                         rtData.rgssErrorMsg.c_str(), win);
	}

	/* Clean up any remainin events */
	eventThread.cleanup();

	/* Store key bindings */
	BDescVec keyBinds;
	rtData.bindingUpdateMsg.get(keyBinds);
	storeBindings(keyBinds, rtData.config);

	Debug() << "Shutting down.";

	SDL_DestroyWindow(win);

	Sound_Quit();
	TTF_Quit();
	IMG_Quit();
	SDL_Quit();

	return 0;
}
