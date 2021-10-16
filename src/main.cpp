/*
** main.cpp
**
** This file is part of mkxp.
**
** Copyright (C) 2013 - 2021 Amaryllis Kulla <ancurio@mapleshrine.eu>
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

#ifdef __WINDOWS__
#include "resource.h"
#endif

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
	const Config &conf = threadData->config;
	SDL_Window *win = threadData->window;
	SDL_GLContext glCtx;

	/* Setup GL context */
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

	if (conf.debugMode)
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

	if (!conf.enableBlitting)
		gl.BlitFramebuffer = 0;

	gl.ClearColor(0, 0, 0, 1);
	gl.Clear(GL_COLOR_BUFFER_BIT);
	SDL_GL_SwapWindow(win);

	printGLInfo();

	bool vsync = conf.vsync || conf.syncToRefreshrate;
	SDL_GL_SetSwapInterval(vsync ? 1 : 0);

	GLDebugLogger dLogger;

	/* Setup AL context */
	ALCcontext *alcCtx = alcCreateContext(threadData->alcDev, 0);

	if (!alcCtx)
	{
		rgssThreadError(threadData, "Error creating OpenAL context");
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
		SDL_GL_DeleteContext(glCtx);

		return 0;
	}

	/* Start script execution */
	scriptBinding->execute();

	threadData->rqTermAck.set();
	threadData->ethread->requestTerminate();

	SharedState::finiInstance();

	alcDestroyContext(alcCtx);
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

static void showInitError(const std::string &msg)
{
	Debug() << msg;
	SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "mkxp", msg.c_str(), 0);
}

static void setupWindowIcon(const Config &conf, SDL_Window *win)
{
	SDL_RWops *iconSrc;

	if (conf.iconPath.empty())
		iconSrc = SDL_RWFromConstMem(assets_icon_png, assets_icon_png_len);
	else
		iconSrc = SDL_RWFromFile(conf.iconPath.c_str(), "rb");

	SDL_Surface *iconImg = IMG_Load_RW(iconSrc, SDL_TRUE);

	if (iconImg)
	{
		SDL_SetWindowIcon(win, iconImg);
		SDL_FreeSurface(iconImg);
	}
}

int main(int argc, char *argv[])
{
	SDL_SetHint(SDL_HINT_VIDEO_MINIMIZE_ON_FOCUS_LOSS, "0");
	SDL_SetHint(SDL_HINT_ACCELEROMETER_AS_JOYSTICK, "0");

	/* initialize SDL first */
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK) < 0)
	{
		showInitError(std::string("Error initializing SDL: ") + SDL_GetError());
		return 0;
	}

	if (!EventThread::allocUserEvents())
	{
		showInitError("Error allocating SDL user events");
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

	if (!conf.gameFolder.empty())
		if (chdir(conf.gameFolder.c_str()) != 0)
		{
			showInitError(std::string("Unable to switch into gameFolder ") + conf.gameFolder);
			return 0;
		}

	conf.readGameINI();

	if (conf.windowTitle.empty())
		conf.windowTitle = conf.game.title;

	assert(conf.rgssVersion >= 1 && conf.rgssVersion <= 3);
	printRgssVersion(conf.rgssVersion);

	int imgFlags = IMG_INIT_PNG | IMG_INIT_JPG;
	if (IMG_Init(imgFlags) != imgFlags)
	{
		showInitError(std::string("Error initializing SDL_image: ") + SDL_GetError());
		SDL_Quit();

		return 0;
	}

	if (TTF_Init() < 0)
	{
		showInitError(std::string("Error initializing SDL_ttf: ") + SDL_GetError());
		IMG_Quit();
		SDL_Quit();

		return 0;
	}

	if (Sound_Init() == 0)
	{
		showInitError(std::string("Error initializing SDL_sound: ") + Sound_GetError());
		TTF_Quit();
		IMG_Quit();
		SDL_Quit();

		return 0;
	}

	SDL_Window *win;
	Uint32 winFlags = SDL_WINDOW_OPENGL | SDL_WINDOW_INPUT_FOCUS;

	if (conf.winResizable)
		winFlags |= SDL_WINDOW_RESIZABLE;
	if (conf.fullscreen)
		winFlags |= SDL_WINDOW_FULLSCREEN_DESKTOP;

	win = SDL_CreateWindow(conf.windowTitle.c_str(),
	                       SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
	                       conf.defScreenW, conf.defScreenH, winFlags);

	if (!win)
	{
		showInitError(std::string("Error creating window: ") + SDL_GetError());
		return 0;
	}

	/* OSX and Windows have their own native ways of
	 * dealing with icons; don't interfere with them */
#ifdef __LINUX__
	setupWindowIcon(conf, win);
#else
	(void) setupWindowIcon;
#endif

	ALCdevice *alcDev = alcOpenDevice(0);

	if (!alcDev)
	{
		showInitError("Error opening OpenAL device");
		SDL_DestroyWindow(win);
		TTF_Quit();
		IMG_Quit();
		SDL_Quit();

		return 0;
	}

	SDL_DisplayMode mode;
	SDL_GetDisplayMode(0, 0, &mode);

	/* Can't sync to display refresh rate if its value is unknown */
	if (!mode.refresh_rate)
		conf.syncToRefreshrate = false;

	EventThread eventThread;
	RGSSThreadData rtData(&eventThread, argv[0], win,
	                      alcDev, mode.refresh_rate, conf);

	int winW, winH;
	SDL_GetWindowSize(win, &winW, &winH);
	rtData.windowSizeMsg.post(Vec2i(winW, winH));

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
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, conf.windowTitle.c_str(),
		                         "The RGSS script seems to be stuck and mkxp will now force quit", win);

	if (!rtData.rgssErrorMsg.empty())
	{
		Debug() << rtData.rgssErrorMsg;
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, conf.windowTitle.c_str(),
		                         rtData.rgssErrorMsg.c_str(), win);
	}

	/* Clean up any remainin events */
	eventThread.cleanup();

	Debug() << "Shutting down.";

	alcCloseDevice(alcDev);
	SDL_DestroyWindow(win);

	Sound_Quit();
	TTF_Quit();
	IMG_Quit();
	SDL_Quit();

	return 0;
}
