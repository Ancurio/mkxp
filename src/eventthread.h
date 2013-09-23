/*
** eventthread.h
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

#ifndef EVENTTHREAD_H
#define EVENTTHREAD_H

#include "config.h"
#include "etc-internal.h"

#include "SDL2/SDL_scancode.h"
#include "SDL2/SDL_joystick.h"
#include "SDL2/SDL_mouse.h"

#include "SDL2/SDL_mutex.h"

#include <QByteArray>
#include <QVector>

struct RGSSThreadData;
struct SDL_Thread;
struct SDL_Window;

class EventThread
{
public:
	static bool keyStates[SDL_NUM_SCANCODES];

	struct JoyState
	{
		int xAxis;
		int yAxis;

		bool buttons[16];
	};

	static JoyState joyState;

	struct MouseState
	{
		int x, y;
		bool buttons[SDL_BUTTON_X2+1];
	};

	static MouseState mouseState;

	EventThread();

	void process(RGSSThreadData &rtData);
	void cleanup();

	/* Called from rgss thread */
	void requestFullscreenMode(bool mode);
	void requestWindowResize(int width, int height);

	void requestTerminate();

	bool getFullscreen();

	void showMessageBox(const char *body, int flags = 0);

private:
	void resetInputStates();
	void setFullscreen(SDL_Window *, bool mode);
	bool fullscreen;
	volatile bool msgBoxDone;
};

/* Used to asynchronously inform the rgss thread
 * about window size changes */
struct WindowSizeNotify
{
	SDL_mutex *mutex;

	volatile bool changedFlag;
	volatile int w, h;

	WindowSizeNotify()
	{
		mutex = SDL_CreateMutex();
		changedFlag = false;
		w = h = 0;
	}

	~WindowSizeNotify()
	{
		SDL_DestroyMutex(mutex);
	}

	/* Done from the sending side */
	void notifyChange(int w, int h)
	{
		SDL_LockMutex(mutex);

		this->w = w;
		this->h = h;
		changedFlag = true;

		SDL_UnlockMutex(mutex);
	}

	/* Done from the receiving side */
	bool pollChange(int *w, int *h)
	{
		if (!changedFlag)
			return false;

		SDL_LockMutex(mutex);

		*w = this->w;
		*h = this->h;
		changedFlag = false;

		SDL_UnlockMutex(mutex);

		return true;
	}
};

struct RGSSThreadData
{
	/* Main thread sets this to request rgss thread to terminate */
	volatile bool rqTerm;
	/* In response, rgss thread sets this to confirm
	 * that it received the request and isn't stuck */
	volatile bool rqTermAck;

	EventThread *ethread;
	WindowSizeNotify windowSizeMsg;

	const char *argv0;

	SDL_Window *window;

	Vec2 sizeResoRatio;
	Vec2i screenOffset;

	Config config;

	QByteArray rgssErrorMsg;

	RGSSThreadData(EventThread *ethread,
	                 const char *argv0,
	                 SDL_Window *window)
	    : rqTerm(false),
	      rqTermAck(false),
	      ethread(ethread),
	      argv0(argv0),
	      window(window),
	      sizeResoRatio(1, 1)
	{}
};

#endif // EVENTTHREAD_H
