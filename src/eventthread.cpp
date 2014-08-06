/*
** eventthread.cpp
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

#include "eventthread.h"

#include <SDL_events.h>
#include <SDL_joystick.h>
#include <SDL_messagebox.h>
#include <SDL_timer.h>
#include <SDL_thread.h>

#include "sharedstate.h"
#include "graphics.h"
#include "debugwriter.h"

#include <string.h>

bool EventThread::keyStates[] = { false };

EventThread::JoyState EventThread::joyState =
{
	0, 0, { false }
};

EventThread::MouseState EventThread::mouseState =
{
	0, 0, false, { false }
};

/* User event codes */
enum
{
	REQUEST_SETFULLSCREEN = 0,
	REQUEST_WINRESIZE,
	REQUEST_MESSAGEBOX,
	REQUEST_SETCURSORVISIBLE,

	UPDATE_FPS,

	EVENT_COUNT
};

static uint32_t usrIdStart;

bool EventThread::allocUserEvents()
{
	usrIdStart = SDL_RegisterEvents(EVENT_COUNT);

	if (usrIdStart == (uint32_t) -1)
		return false;

	return true;
}

EventThread::EventThread()
    : fullscreen(false),
      showCursor(false)
{}

void EventThread::process(RGSSThreadData &rtData)
{
	SDL_Event event;
	SDL_Window *win = rtData.window;
	WindowSizeNotify &windowSizeMsg = rtData.windowSizeMsg;

	fullscreen = rtData.config.fullscreen;
	int toggleFSMod = rtData.config.anyAltToggleFS ? KMOD_ALT : KMOD_LALT;

	fps.lastFrame = SDL_GetPerformanceCounter();
	fps.displaying = false;
	fps.immInitFlag = false;
	fps.immFiniFlag = false;
	fps.acc = 0;
	fps.accDiv = 0;

	bool cursorInWindow = false;
	bool windowFocused = false;

	bool terminate = false;

	SDL_Joystick *js = 0;
	if (SDL_NumJoysticks() > 0)
		js = SDL_JoystickOpen(0);

	char buffer[128];

	char pendingTitle[128];
	bool havePendingTitle = false;

	while (true)
	{
		if (!SDL_WaitEvent(&event))
		{
			Debug() << "EventThread: Event error";
			break;
		}

		switch (event.type)
		{
		case SDL_WINDOWEVENT :
			switch (event.window.event)
			{
			case SDL_WINDOWEVENT_RESIZED :
				windowSizeMsg.notifyChange(event.window.data1,
				                           event.window.data2);
				break;

			case SDL_WINDOWEVENT_ENTER :
				cursorInWindow = true;
				mouseState.inWindow = true;
				updateCursorState(cursorInWindow && windowFocused);

				break;

			case SDL_WINDOWEVENT_LEAVE :
				cursorInWindow = false;
				mouseState.inWindow = false;
				updateCursorState(cursorInWindow && windowFocused);

				break;

			case SDL_WINDOWEVENT_CLOSE :
				terminate = true;

				break;

			case SDL_WINDOWEVENT_FOCUS_GAINED :
				windowFocused = true;
				updateCursorState(cursorInWindow && windowFocused);

				break;

			case SDL_WINDOWEVENT_FOCUS_LOST :
				windowFocused = false;
				updateCursorState(cursorInWindow && windowFocused);
				resetInputStates();

				break;
			}
			break;

		case SDL_QUIT :
			terminate = true;
			Debug() << "EventThread termination requested";

			break;

		case SDL_KEYDOWN :
			if (event.key.keysym.scancode == SDL_SCANCODE_RETURN &&
			    (event.key.keysym.mod & toggleFSMod))
			{
				setFullscreen(win, !fullscreen);
				if (!fullscreen && havePendingTitle)
				{
					SDL_SetWindowTitle(win, pendingTitle);
					pendingTitle[0] = '\0';
					havePendingTitle = false;
				}

				break;
			}

			if (event.key.keysym.scancode == SDL_SCANCODE_F2)
			{
				if (!fps.displaying)
				{
					fps.immInitFlag = true;
					fps.displaying = true;
				}
				else
				{
					fps.displaying = false;

					if (fullscreen)
					{
						/* Prevent fullscreen flicker */
						strncpy(pendingTitle, rtData.config.game.title.c_str(),
						        sizeof(pendingTitle));
						havePendingTitle = true;

						break;
					}

					SDL_SetWindowTitle(win, rtData.config.game.title.c_str());
				}

				break;
			}

			keyStates[event.key.keysym.scancode] = true;
			break;

		case SDL_KEYUP :
			keyStates[event.key.keysym.scancode] = false;
			break;

		case SDL_JOYBUTTONDOWN :
			joyState.buttons[event.jbutton.button] = true;
			break;

		case SDL_JOYBUTTONUP :
			joyState.buttons[event.jbutton.button] = false;
			break;

		case SDL_JOYAXISMOTION :
			if (event.jaxis.axis == 0)
				joyState.xAxis = event.jaxis.value;
			else
				joyState.yAxis = event.jaxis.value;

			break;

		case SDL_JOYDEVICEADDED :
			if (event.jdevice.which > 0)
				break;

			js = SDL_JoystickOpen(0);
			break;

		case SDL_JOYDEVICEREMOVED :
			resetInputStates();
			break;

		case SDL_MOUSEBUTTONDOWN :
			mouseState.buttons[event.button.button] = true;
			break;

		case SDL_MOUSEBUTTONUP :
			mouseState.buttons[event.button.button] = false;
			break;

		case SDL_MOUSEMOTION :
			mouseState.x = event.motion.x;
			mouseState.y = event.motion.y;

			break;

		default :
			/* Handle user events */
			switch(event.type - usrIdStart)
			{
			case REQUEST_SETFULLSCREEN :
				setFullscreen(win, static_cast<bool>(event.user.code));
				break;

			case REQUEST_WINRESIZE :
				SDL_SetWindowSize(win, event.window.data1, event.window.data2);
				break;

			case REQUEST_MESSAGEBOX :
				SDL_ShowSimpleMessageBox(event.user.code,
				                         rtData.config.game.title.c_str(),
				                         (const char*) event.user.data1, win);
				free(event.user.data1);
				msgBoxDone = true;
				break;

			case REQUEST_SETCURSORVISIBLE :
				showCursor = event.user.code;
				updateCursorState(cursorInWindow);
				break;

			case UPDATE_FPS :
				if (!fps.displaying)
					break;

				snprintf(buffer, sizeof(buffer), "%s - %d FPS",
				         rtData.config.game.title.c_str(), event.user.code);

				/* Updating the window title in fullscreen
				 * mode seems to cause flickering */
				if (fullscreen)
				{
					strncpy(pendingTitle, buffer, sizeof(pendingTitle));
					havePendingTitle = true;

					break;
				}

				SDL_SetWindowTitle(win, buffer);
				break;
			}
		}

		if (terminate)
			break;
	}

	if (SDL_JoystickGetAttached(js))
		SDL_JoystickClose(js);
}

void EventThread::cleanup()
{
	SDL_Event event;

	while (SDL_PollEvent(&event))
		if ((event.type - usrIdStart) == REQUEST_MESSAGEBOX)
			free(event.user.data1);
}

void EventThread::resetInputStates()
{
	memset(&keyStates, 0, sizeof(keyStates));
	memset(&joyState, 0, sizeof(joyState));
	memset(&mouseState.buttons, 0, sizeof(mouseState.buttons));
}

void EventThread::setFullscreen(SDL_Window *win, bool mode)
{
	SDL_SetWindowFullscreen
	        (win, mode ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
	fullscreen = mode;
}

void EventThread::updateCursorState(bool inWindow)
{
	if (inWindow)
		SDL_ShowCursor(showCursor ? SDL_TRUE : SDL_FALSE);
	else
		SDL_ShowCursor(SDL_TRUE);
}

void EventThread::requestTerminate()
{
	SDL_Event event;
	event.type = SDL_QUIT;
	SDL_PushEvent(&event);
}

void EventThread::requestFullscreenMode(bool mode)
{
	if (mode == fullscreen)
		return;

	SDL_Event event;
	event.type = usrIdStart + REQUEST_SETFULLSCREEN;
	event.user.code = static_cast<Sint32>(mode);
	SDL_PushEvent(&event);
}

void EventThread::requestWindowResize(int width, int height)
{
	SDL_Event event;
	event.type = usrIdStart + REQUEST_WINRESIZE;
	event.window.data1 = width;
	event.window.data2 = height;
	SDL_PushEvent(&event);
}

void EventThread::requestShowCursor(bool mode)
{
	SDL_Event event;
	event.type = usrIdStart + REQUEST_SETCURSORVISIBLE;
	event.user.code = mode;
	SDL_PushEvent(&event);
}

void EventThread::showMessageBox(const char *body, int flags)
{
	msgBoxDone = false;

	SDL_Event event;
	event.user.code = flags;
	event.user.data1 = strdup(body);
	event.type = usrIdStart + REQUEST_MESSAGEBOX;
	SDL_PushEvent(&event);

	/* Keep repainting screen while box is open */
	shState->graphics().repaintWait(&msgBoxDone);
	/* Prevent endless loops */
	resetInputStates();
}

bool EventThread::getFullscreen() const
{
	return fullscreen;
}

bool EventThread::getShowCursor() const
{
	return showCursor;
}

void EventThread::notifyFrame()
{
	if (!fps.displaying)
		return;

	uint64_t current = SDL_GetPerformanceCounter();
	uint64_t diff = current - fps.lastFrame;
	fps.lastFrame = current;

	if (fps.immInitFlag)
	{
		fps.immInitFlag = false;
		fps.immFiniFlag = true;

		return;
	}

	static uint64_t freq = SDL_GetPerformanceFrequency();

	double currFPS = (double) freq / diff;
	fps.acc += currFPS;
	++fps.accDiv;

	fps.displayCounter += diff;
	if (fps.displayCounter < freq && !fps.immFiniFlag)
		return;

	fps.displayCounter = 0;
	fps.immFiniFlag = false;

	int32_t avgFPS = fps.acc / fps.accDiv;
	fps.acc = fps.accDiv = 0;

	SDL_Event event;
	event.user.code = avgFPS;
	event.user.type = usrIdStart + UPDATE_FPS;
	SDL_PushEvent(&event);
}
