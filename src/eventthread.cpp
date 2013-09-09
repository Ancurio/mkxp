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

#include "SDL2/SDL_events.h"
#include "SDL2/SDL_joystick.h"
#include "SDL2/SDL_messagebox.h"
#include "SDL2/SDL_timer.h"
#include "SDL2/SDL_thread.h"

#include "globalstate.h"
#include "graphics.h"

#include "string.h"

#include <QDebug>

bool EventThread::keyStates[] = { false };

EventThread::JoyState EventThread::joyState =
{
     0, 0, { false }
};

EventThread::MouseState EventThread::mouseState =
{
    0, 0, { false }
};

enum
{
	REQUEST_FIRST = SDL_USEREVENT,

	REQUEST_TERMINATION,
	REQUEST_SETFULLSCREEN,
	REQUEST_WINRESIZE,
	REQUEST_MESSAGEBOX
};

EventThread::EventThread()
    : fullscreen(false)
{}

void EventThread::process(RGSSThreadData &rtData)
{
	SDL_Event event;
	SDL_Window *win = rtData.window;
	WindowSizeNotify &windowSizeMsg = rtData.windowSizeMsg;

	fullscreen = rtData.config.fullscreen;

	bool terminate = false;

	SDL_Joystick *js = 0;
	if (SDL_NumJoysticks() > 0)
		js = SDL_JoystickOpen(0);

	while (true)
	{
		if (!SDL_WaitEvent(&event))
		{
			qDebug() << "EventThread: Event error";
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

			case SDL_WINDOWEVENT_CLOSE :
				terminate = true;
				break;

			case SDL_WINDOWEVENT_FOCUS_LOST :
				resetInputStates();
				break;
			}
			break;

		case SDL_QUIT :
		case REQUEST_TERMINATION :
			terminate = true;
			qDebug() << "EventThread termination requested";
			break;

		case SDL_KEYDOWN :
			if (event.key.keysym.scancode == SDL_SCANCODE_RETURN &&
			    (event.key.keysym.mod & KMOD_LALT))
			{
				setFullscreen(win, !fullscreen);
				break;
			}

			keyStates[event.key.keysym.scancode] = true;
			break;

		case REQUEST_SETFULLSCREEN :
			setFullscreen(win, static_cast<bool>(event.user.code));
			break;

		case REQUEST_WINRESIZE :
			SDL_SetWindowSize(win, event.window.data1, event.window.data2);
			break;

		case REQUEST_MESSAGEBOX :
			SDL_ShowSimpleMessageBox(event.user.code,
			                         rtData.config.game.title.constData(),
			                         (const char*) event.user.data1, win);
			free(event.user.data1);
			msgBoxDone = true;
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
	{
		if (event.type == REQUEST_MESSAGEBOX)
		{
			free(event.user.data1);
		}
	}
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

void EventThread::requestTerminate()
{
	SDL_Event event;
	event.type = REQUEST_TERMINATION;
	SDL_PushEvent(&event);
}

void EventThread::requestFullscreenMode(bool mode)
{
	if (mode == fullscreen)
		return;

	SDL_Event event;
	event.type = REQUEST_SETFULLSCREEN;
	event.user.code = static_cast<Sint32>(mode);
	SDL_PushEvent(&event);
}

void EventThread::requestWindowResize(int width, int height)
{
	SDL_Event event;
	event.type = REQUEST_WINRESIZE;
	event.window.data1 = width;
	event.window.data2 = height;
	SDL_PushEvent(&event);
}

void EventThread::showMessageBox(const char *body, int flags)
{
	msgBoxDone = false;

	SDL_Event event;
	event.user.code = flags;
	event.user.data1 = strdup(body);
	event.type = REQUEST_MESSAGEBOX;
	SDL_PushEvent(&event);

	/* Keep repainting screen while box is open */
	gState->graphics().repaintWait(&msgBoxDone);
	/* Prevent endless loops */
	resetInputStates();
}

bool EventThread::getFullscreen()
{
	return fullscreen;
}
