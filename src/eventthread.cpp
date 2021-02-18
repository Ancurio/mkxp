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
#include <SDL_touch.h>
#include <SDL_rect.h>

#include <al.h>
#include <alc.h>
#include <alext.h>

#include "sharedstate.h"
#include "graphics.h"

#ifndef MKXPZ_BUILD_XCODE
#include "settingsmenu.h"
#else
#include "system/system.h"
#endif

#include "al-util.h"
#include "debugwriter.h"

#include <string.h>

typedef void (ALC_APIENTRY *LPALCDEVICEPAUSESOFT) (ALCdevice *device);
typedef void (ALC_APIENTRY *LPALCDEVICERESUMESOFT) (ALCdevice *device);

#define AL_DEVICE_PAUSE_FUN \
	AL_FUN(DevicePause, LPALCDEVICEPAUSESOFT) \
	AL_FUN(DeviceResume, LPALCDEVICERESUMESOFT)

struct ALCFunctions
{
#define AL_FUN(name, type) type name;
	AL_DEVICE_PAUSE_FUN
#undef AL_FUN
} static alc;

static void
initALCFunctions(ALCdevice *alcDev)
{
	if (!strstr(alcGetString(alcDev, ALC_EXTENSIONS), "ALC_SOFT_pause_device"))
		return;

	Debug() << "ALC_SOFT_pause_device present";

#define AL_FUN(name, type) alc. name = (type) alcGetProcAddress(alcDev, "alc" #name "SOFT");
	AL_DEVICE_PAUSE_FUN;
#undef AL_FUN
}

#define HAVE_ALC_DEVICE_PAUSE alc.DevicePause

uint8_t EventThread::keyStates[];
EventThread::JoyState EventThread::joyState;
EventThread::MouseState EventThread::mouseState;
EventThread::TouchState EventThread::touchState;

/* User event codes */
enum
{
	REQUEST_SETFULLSCREEN = 0,
	REQUEST_WINRESIZE,
    REQUEST_WINREPOSITION,
    REQUEST_WINRENAME,
    REQUEST_WINCENTER,
	REQUEST_MESSAGEBOX,
	REQUEST_SETCURSORVISIBLE,
    
    REQUEST_TEXTMODE,
    
    REQUEST_SETTINGS,
    
    REQUEST_RUMBLE,

	UPDATE_FPS,
	UPDATE_SCREEN_RECT,

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
    : js(0),
      hapt(0),
      fullscreen(false),
      showCursor(true),
      joystickConnected(false),
      hapticEffectId(0)
{}

void EventThread::process(RGSSThreadData &rtData)
{
	SDL_Event event;
	SDL_Window *win = rtData.window;
	UnidirMessage<Vec2i> &windowSizeMsg = rtData.windowSizeMsg;

	initALCFunctions(rtData.alcDev);

	// XXX this function breaks input focus on OSX
#ifndef __MACOSX__
	SDL_SetEventFilter(eventFilter, &rtData);
#endif

	fullscreen = rtData.config.fullscreen;
	int toggleFSMod = rtData.config.anyAltToggleFS ? KMOD_ALT : KMOD_LALT;

	fps.lastFrame = SDL_GetPerformanceCounter();
	fps.displayCounter = 0;
	fps.acc = 0;
	fps.accDiv = 0;

	if (rtData.config.printFPS)
		fps.sendUpdates.set();

	bool displayingFPS = false;

	bool cursorInWindow = false;
	/* Will be updated eventually */
	SDL_Rect gameScreen = { 0, 0, 0, 0 };

	/* SDL doesn't send an initial FOCUS_GAINED event */
	bool windowFocused = true;

	bool terminate = false;

	if (SDL_NumJoysticks() > 0)
		js = SDL_JoystickOpen(0);

	char buffer[128];

	char pendingTitle[128];
	bool havePendingTitle = false;

	bool resetting = false;

	int winW, winH;
	int i, rc;
    
    SDL_DisplayMode dm = {0};

	SDL_GetWindowSize(win, &winW, &winH);
    
    SDL_Haptic *hap;
    memset(&hapticEffect, 0, sizeof(SDL_HapticEffect));
    textInputBuffer.clear();
#ifndef MKXPZ_BUILD_XCODE
	SettingsMenu *sMenu = 0;
#else
    // Will always be 0
    void *sMenu = 0;
#endif

	while (true)
	{
		if (!SDL_WaitEvent(&event))
		{
			Debug() << "EventThread: Event error";
			break;
		}
#ifndef MKXPZ_BUILD_XCODE
		if (sMenu && sMenu->onEvent(event))
		{
			if (sMenu->destroyReq())
			{
				delete sMenu;
				sMenu = 0;

				updateCursorState(cursorInWindow && windowFocused, gameScreen);
			}

			continue;
		}
#endif

		/* Preselect and discard unwanted events here */
		switch (event.type)
		{
		case SDL_MOUSEBUTTONDOWN :
		case SDL_MOUSEBUTTONUP :
		case SDL_MOUSEMOTION :
			if (event.button.which == SDL_TOUCH_MOUSEID)
				continue;
			break;

		case SDL_FINGERDOWN :
		case SDL_FINGERUP :
		case SDL_FINGERMOTION :
			if (event.tfinger.fingerId >= MAX_FINGERS)
				continue;
			break;
		}

		/* Now process the rest */
		switch (event.type)
		{
		case SDL_WINDOWEVENT :
			switch (event.window.event)
			{
			case SDL_WINDOWEVENT_SIZE_CHANGED :
				winW = event.window.data1;
				winH = event.window.data2;

				windowSizeMsg.post(Vec2i(winW, winH));
				resetInputStates();
				break;

			case SDL_WINDOWEVENT_ENTER :
				cursorInWindow = true;
				mouseState.inWindow = true;
				updateCursorState(cursorInWindow && windowFocused && !sMenu, gameScreen);

				break;

			case SDL_WINDOWEVENT_LEAVE :
				cursorInWindow = false;
				mouseState.inWindow = false;
				updateCursorState(cursorInWindow && windowFocused && !sMenu, gameScreen);

				break;

			case SDL_WINDOWEVENT_CLOSE :
				terminate = true;

				break;

			case SDL_WINDOWEVENT_FOCUS_GAINED :
				windowFocused = true;
				updateCursorState(cursorInWindow && windowFocused && !sMenu, gameScreen);

				break;

			case SDL_WINDOWEVENT_FOCUS_LOST :
				windowFocused = false;
				updateCursorState(cursorInWindow && windowFocused && !sMenu, gameScreen);
				resetInputStates();

				break;
			}
			break;
                
        case SDL_TEXTINPUT :
            if (textInputBuffer.size() < 512)
                textInputBuffer += event.text.text;
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

			if (event.key.keysym.scancode == SDL_SCANCODE_F1)
			{
#ifndef MKXPZ_BUILD_XCODE
				if (!sMenu)
				{
					sMenu = new SettingsMenu(rtData);
					updateCursorState(false, gameScreen);
				}

				sMenu->raise();
#else
                openSettingsWindow();
#endif
			}

			if (event.key.keysym.scancode == SDL_SCANCODE_F2)
			{
				if (!displayingFPS)
				{
					fps.immInitFlag.set();
					fps.sendUpdates.set();
					displayingFPS = true;
				}
				else
				{
					displayingFPS = false;

					if (!rtData.config.printFPS)
						fps.sendUpdates.clear();

					if (fullscreen)
					{
						/* Prevent fullscreen flicker */
						strncpy(pendingTitle, rtData.config.windowTitle.c_str(),
						        sizeof(pendingTitle));
						havePendingTitle = true;

						break;
					}

					SDL_SetWindowTitle(win, rtData.config.windowTitle.c_str());
				}

				break;
			}

			if (event.key.keysym.scancode == SDL_SCANCODE_F12)
			{
				if (!rtData.config.enableReset)
					break;

				if (resetting)
					break;

				resetting = true;
				rtData.rqResetFinish.clear();
				rtData.rqReset.set();
				break;
			}

			keyStates[event.key.keysym.scancode] = true;
			break;

		case SDL_KEYUP :
			if (event.key.keysym.scancode == SDL_SCANCODE_F12)
			{
				if (!rtData.config.enableReset)
					break;

				resetting = false;
				rtData.rqResetFinish.set();
				break;
			}

			keyStates[event.key.keysym.scancode] = false;
			break;

		case SDL_JOYBUTTONDOWN :
			joyState.buttons[event.jbutton.button] = true;
			break;

		case SDL_JOYBUTTONUP :
			joyState.buttons[event.jbutton.button] = false;
			break;

		case SDL_JOYHATMOTION :
			joyState.hats[event.jhat.hat] = event.jhat.value;
			break;

		case SDL_JOYAXISMOTION :
			joyState.axes[event.jaxis.axis] = event.jaxis.value;
			break;

		case SDL_JOYDEVICEADDED :
			if (event.jdevice.which > 0)
				break;

			js = SDL_JoystickOpen(0);
            joystickConnected = true;
                
            hap = SDL_HapticOpenFromJoystick(js);
            Debug() << (hap ? "true" : "false");
            if (hap && (SDL_HapticQuery(hap) & SDL_HAPTIC_SINE))
            {
                hapt = hap;
                Debug() << "Haptic device initialized";
            }
            else
            {
                Debug() << "No haptic support found";
            }
            
			break;

		case SDL_JOYDEVICEREMOVED :
			resetInputStates();
            joystickConnected = false;
            hapt = 0;
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
			updateCursorState(cursorInWindow, gameScreen);
			break;

		case SDL_FINGERDOWN :
			i = event.tfinger.fingerId;
			touchState.fingers[i].down = true;

		case SDL_FINGERMOTION :
			i = event.tfinger.fingerId;
			touchState.fingers[i].x = event.tfinger.x * winW;
			touchState.fingers[i].y = event.tfinger.y * winH;
			break;

		case SDL_FINGERUP :
			i = event.tfinger.fingerId;
			memset(&touchState.fingers[i], 0, sizeof(touchState.fingers[0]));
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
                    
            case REQUEST_WINREPOSITION :
                SDL_SetWindowPosition(win, event.window.data1, event.window.data2);
                break;
                    
            case REQUEST_WINCENTER :
                    rc = SDL_GetDesktopDisplayMode(SDL_GetWindowDisplayIndex(win), &dm);
                    if (!rc)
                        SDL_SetWindowPosition(win,
                                              (dm.w / 2) - (winW / 2),
                                              (dm.h / 2) - (winH / 2));
                    break;
                    
            case REQUEST_WINRENAME :
                rtData.config.windowTitle = (const char*)event.user.data1;
                SDL_SetWindowTitle(win, rtData.config.windowTitle.c_str());
                break;
                    
            case REQUEST_TEXTMODE :
                if (event.user.code)
                {
                    SDL_StartTextInput();
                    textInputBuffer.clear();
                }
                else
                {
                    SDL_StopTextInput();
                    textInputBuffer.clear();
                }
                break;

			case REQUEST_MESSAGEBOX :
				SDL_ShowSimpleMessageBox(event.user.code,
				                         rtData.config.windowTitle.c_str(),
				                         (const char*) event.user.data1, win);
				free(event.user.data1);
				msgBoxDone.set();
				break;

			case REQUEST_SETCURSORVISIBLE :
				showCursor = event.user.code;
				updateCursorState(cursorInWindow, gameScreen);
				break;
                    
            case REQUEST_SETTINGS :
#ifndef MKXPZ_BUILD_XCODE
                if (!sMenu)
                {
                    sMenu = new SettingsMenu(rtData);
                    updateCursorState(false, gameScreen);
                }
                
                sMenu->raise();
#else
                openSettingsWindow();
#endif
                break;
                    
            case REQUEST_RUMBLE :
                if (!hapt) break;
                if (!hapticEffect.type)
                {
                    hapticEffect.type = SDL_HAPTIC_SINE;
                    hapticEffect.constant.attack_level = 0;
                    hapticEffect.constant.fade_level = 0;
                    hapticEffectId = SDL_HapticNewEffect(hapt, &hapticEffect);
                }
                if (hapticEffect.constant.length == 0 && SDL_HapticGetEffectStatus(hapt, hapticEffectId) > 0)
                {
                    SDL_HapticStopEffect(hapt, hapticEffectId);
                    break;
                }
                SDL_HapticUpdateEffect(hapt, hapticEffectId, &hapticEffect);
                    
                SDL_HapticRunEffect(hapt, hapticEffectId, 1);
                break;

			case UPDATE_FPS :
				if (rtData.config.printFPS)
					Debug() << "FPS:" << event.user.code;

				if (!fps.sendUpdates)
					break;

				snprintf(buffer, sizeof(buffer), "%s - %d FPS",
				         rtData.config.windowTitle.c_str(), event.user.code);

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

			case UPDATE_SCREEN_RECT :
				gameScreen.x = event.user.windowID;
				gameScreen.y = event.user.code;
				gameScreen.w = reinterpret_cast<intptr_t>(event.user.data1);
				gameScreen.h = reinterpret_cast<intptr_t>(event.user.data2);
				updateCursorState(cursorInWindow, gameScreen);

				break;
			}
		}

		if (terminate)
			break;
	}

	/* Just in case */
	rtData.syncPoint.resumeThreads();

	if (SDL_JoystickGetAttached(js))
		SDL_JoystickClose(js);

	delete sMenu;
}

int EventThread::eventFilter(void *data, SDL_Event *event)
{
	RGSSThreadData &rtData = *static_cast<RGSSThreadData*>(data);

	switch (event->type)
	{
	case SDL_APP_WILLENTERBACKGROUND :
		Debug() << "SDL_APP_WILLENTERBACKGROUND";

		if (HAVE_ALC_DEVICE_PAUSE)
			alc.DevicePause(rtData.alcDev);

		rtData.syncPoint.haltThreads();

		return 0;

	case SDL_APP_DIDENTERBACKGROUND :
		Debug() << "SDL_APP_DIDENTERBACKGROUND";
		return 0;

	case SDL_APP_WILLENTERFOREGROUND :
		Debug() << "SDL_APP_WILLENTERFOREGROUND";
		return 0;

	case SDL_APP_DIDENTERFOREGROUND :
		Debug() << "SDL_APP_DIDENTERFOREGROUND";

		if (HAVE_ALC_DEVICE_PAUSE)
			alc.DeviceResume(rtData.alcDev);

		rtData.syncPoint.resumeThreads();

		return 0;

	case SDL_APP_TERMINATING :
		Debug() << "SDL_APP_TERMINATING";
		return 0;

	case SDL_APP_LOWMEMORY :
		Debug() << "SDL_APP_LOWMEMORY";
		return 0;

//	case SDL_RENDER_TARGETS_RESET :
//		Debug() << "****** SDL_RENDER_TARGETS_RESET";
//		return 0;

//	case SDL_RENDER_DEVICE_RESET :
//		Debug() << "****** SDL_RENDER_DEVICE_RESET";
//		return 0;
	}

	return 1;
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
	memset(&touchState, 0, sizeof(touchState));
}

void EventThread::setFullscreen(SDL_Window *win, bool mode)
{
	SDL_SetWindowFullscreen
	        (win, mode ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
	fullscreen = mode;
}

void EventThread::updateCursorState(bool inWindow,
                                    const SDL_Rect &screen)
{
	SDL_Point pos = { mouseState.x, mouseState.y };
	bool inScreen = inWindow && SDL_PointInRect(&pos, &screen);

	if (inScreen)
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

void EventThread::requestWindowReposition(int x, int y)
{
    SDL_Event event;
    event.type = usrIdStart + REQUEST_WINREPOSITION;
    event.window.data1 = x;
    event.window.data2 = y;
    SDL_PushEvent(&event);
}

void EventThread::requestWindowCenter()
{
    SDL_Event event;
    event.type = usrIdStart + REQUEST_WINCENTER;
    SDL_PushEvent(&event);
}

void EventThread::requestWindowRename(const char *title)
{
    SDL_Event event;
    event.type = usrIdStart + REQUEST_WINRENAME;
    event.user.data1 = (void*)title;
    SDL_PushEvent(&event);
}

void EventThread::requestShowCursor(bool mode)
{
	SDL_Event event;
	event.type = usrIdStart + REQUEST_SETCURSORVISIBLE;
	event.user.code = mode;
	SDL_PushEvent(&event);
}

void EventThread::requestTextInputMode(bool mode)
{
    SDL_Event event;
    event.type = usrIdStart + REQUEST_TEXTMODE;
    event.user.code = mode;
    SDL_PushEvent(&event);
}

void EventThread::requestSettingsMenu()
{
    SDL_Event event;
    event.type = usrIdStart + REQUEST_SETTINGS;
    SDL_PushEvent(&event);
}

void EventThread::requestRumble(int duration, short strength, int attack, int fade)
{
    SDL_Event event;
    event.type = usrIdStart + REQUEST_RUMBLE;
    
    hapticEffect.constant.length = duration;
    hapticEffect.constant.level = strength;
    hapticEffect.constant.attack_length = attack;
    hapticEffect.constant.fade_length = fade;
    
    SDL_PushEvent(&event);
}

void EventThread::showMessageBox(const char *body, int flags)
{
	msgBoxDone.clear();

	SDL_Event event;
	event.user.code = flags;
	event.user.data1 = strdup(body);
	event.type = usrIdStart + REQUEST_MESSAGEBOX;
	SDL_PushEvent(&event);

	/* Keep repainting screen while box is open */
	shState->graphics().repaintWait(msgBoxDone);
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

bool EventThread::getJoystickConnected() const
{
    return joystickConnected;
}

SDL_Joystick *EventThread::joystick() const
{
    return (joystickConnected) ? js : 0;
}

SDL_Haptic *EventThread::haptic() const
{
    return hapt;
}

void EventThread::notifyFrame()
{
	if (!fps.sendUpdates)
		return;

	uint64_t current = SDL_GetPerformanceCounter();
	uint64_t diff = current - fps.lastFrame;
	fps.lastFrame = current;

	if (fps.immInitFlag)
	{
		fps.immInitFlag.clear();
		fps.immFiniFlag.set();

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
	fps.immFiniFlag.clear();

	int32_t avgFPS = fps.acc / fps.accDiv;
	fps.acc = fps.accDiv = 0;

	SDL_Event event;
	event.user.code = avgFPS;
	event.user.type = usrIdStart + UPDATE_FPS;
	SDL_PushEvent(&event);
}

void EventThread::notifyGameScreenChange(const SDL_Rect &screen)
{
	/* We have to get a bit hacky here to fit the rectangle
	 * data into the user event struct */
	SDL_Event event;
	event.type = usrIdStart + UPDATE_SCREEN_RECT;
	event.user.windowID = screen.x;
	event.user.code = screen.y;
	event.user.data1 = reinterpret_cast<void*>(screen.w);
	event.user.data2 = reinterpret_cast<void*>(screen.h);
	SDL_PushEvent(&event);
}

void SyncPoint::haltThreads()
{
	if (mainSync.locked)
		return;

	/* Lock the reply sync first to avoid races */
	reply.lock();

	/* Lock main sync and sleep until RGSS thread
	 * reports back */
	mainSync.lock();
	reply.waitForUnlock();

	/* Now that the RGSS thread is asleep, we can
	 * safely put the other threads to sleep as well
	 * without causing deadlocks */
	secondSync.lock();
}

void SyncPoint::resumeThreads()
{
	if (!mainSync.locked)
		return;

	mainSync.unlock(false);
	secondSync.unlock(true);
}

bool SyncPoint::mainSyncLocked()
{
	return mainSync.locked;
}

void SyncPoint::waitMainSync()
{
	reply.unlock(false);
	mainSync.waitForUnlock();
}

void SyncPoint::passSecondarySync()
{
	if (!secondSync.locked)
		return;

	secondSync.waitForUnlock();
}

SyncPoint::Util::Util()
{
	mut = SDL_CreateMutex();
	cond = SDL_CreateCond();
}

SyncPoint::Util::~Util()
{
	SDL_DestroyCond(cond);
	SDL_DestroyMutex(mut);
}

void SyncPoint::Util::lock()
{
	locked.set();
}

void SyncPoint::Util::unlock(bool multi)
{
	locked.clear();

	if (multi)
		SDL_CondBroadcast(cond);
	else
		SDL_CondSignal(cond);
}

void SyncPoint::Util::waitForUnlock()
{
	SDL_LockMutex(mut);

	while (locked)
		SDL_CondWait(cond, mut);

	SDL_UnlockMutex(mut);
}
