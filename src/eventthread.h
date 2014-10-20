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
#include "sdl-util.h"
#include "keybindings.h"

#include <SDL_scancode.h>
#include <SDL_joystick.h>
#include <SDL_mouse.h>
#include <SDL_mutex.h>

#include <string>

#include <stdint.h>

struct RGSSThreadData;
struct SDL_Window;

class EventThread
{
public:
	static uint8_t keyStates[SDL_NUM_SCANCODES];

	struct JoyState
	{
		int axis[256];
		bool buttons[256];
	};

	static JoyState joyState;

	struct MouseState
	{
		int x, y;
		bool inWindow;
		bool buttons[32];
	};

	static MouseState mouseState;

	static bool allocUserEvents();

	EventThread();

	void process(RGSSThreadData &rtData);
	void cleanup();

	/* Called from RGSS thread */
	void requestFullscreenMode(bool mode);
	void requestWindowResize(int width, int height);
	void requestShowCursor(bool mode);

	void requestTerminate();

	bool getFullscreen() const;
	bool getShowCursor() const;

	void showMessageBox(const char *body, int flags = 0);

	/* RGSS thread calls this once per frame */
	void notifyFrame();

private:
	void resetInputStates();
	void setFullscreen(SDL_Window *, bool mode);
	void updateCursorState(bool inWindow);

	bool fullscreen;
	bool showCursor;
	AtomicFlag msgBoxDone;

	struct
	{
		uint64_t lastFrame;
		uint64_t displayCounter;
		bool displaying;
		bool immInitFlag;
		bool immFiniFlag;
		double acc;
		uint32_t accDiv;
	} fps;
};

/* Used to asynchronously inform the RGSS thread
 * about window size changes */
struct WindowSizeNotify
{
	SDL_mutex *mutex;

	AtomicFlag changed;
	int w, h;

	WindowSizeNotify()
	{
		mutex = SDL_CreateMutex();
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
		changed.set();

		SDL_UnlockMutex(mutex);
	}

	/* Done from the receiving side */
	bool pollChange(int *w, int *h)
	{
		if (!changed)
			return false;

		SDL_LockMutex(mutex);

		*w = this->w;
		*h = this->h;
		changed.clear();

		SDL_UnlockMutex(mutex);

		return true;
	}
};

struct BindingNotify
{
	BindingNotify()
	{
		mut = SDL_CreateMutex();
	}
	~BindingNotify()
	{
		SDL_DestroyMutex(mut);
	}

	bool poll(BDescVec &out) const
	{
		if (!changed)
			return false;

		SDL_LockMutex(mut);

		out = data;
		changed.clear();

		SDL_UnlockMutex(mut);

		return true;
	}

	void get(BDescVec &out) const
	{
		SDL_LockMutex(mut);
		out = data;
		SDL_UnlockMutex(mut);
	}

	void post(const BDescVec &d)
	{
		SDL_LockMutex(mut);

		changed.set();
		data = d;

		SDL_UnlockMutex(mut);
	}

private:
	SDL_mutex *mut;
	BDescVec data;
	mutable AtomicFlag changed;
};

struct RGSSThreadData
{
	/* Main thread sets this to request RGSS thread to terminate */
	AtomicFlag rqTerm;
	/* In response, RGSS thread sets this to confirm
	 * that it received the request and isn't stuck */
	AtomicFlag rqTermAck;

	/* Set when F12 is pressed */
	AtomicFlag rqReset;

	/* Set when F12 is released */
	AtomicFlag rqResetFinish;

	EventThread *ethread;
	WindowSizeNotify windowSizeMsg;
	BindingNotify bindingUpdateMsg;

	const char *argv0;

	SDL_Window *window;

	Vec2 sizeResoRatio;
	Vec2i screenOffset;

	Config config;

	std::string rgssErrorMsg;

	RGSSThreadData(EventThread *ethread,
	               const char *argv0,
	               SDL_Window *window,
	               const Config& newconf)
	    : ethread(ethread),
	      argv0(argv0),
	      window(window),
	      sizeResoRatio(1, 1),
	      config(newconf)
	{}
};

#endif // EVENTTHREAD_H
