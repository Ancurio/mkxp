#ifdef __WIN32__
#include <SDL.h>
#include <SDL_syswm.h>

#include <windows.h>

#include "sharedstate.h"
#include "fake-api.h"
#include "debugwriter.h"


// Essentials, without edits, needs Win32API. Two problems with that:

// 1. Not all Win32API functions work here, and
// 2. It uses Windows libraries.

// So, even on Windows, we need to switch out some of those functions
// with our own in order to trick the game into doing what we want.

// On Windows, we just have to worry about functions that involve
// thread IDs and window handles mostly. On anything else, every
// function that an Essentials game uses has to be impersonated.

// If you're lucky enough to be using a crossplatform library, you
// could just load that since MiniFFI should work anywhere, but
// every single system or windows-specific function you use has
// to be intercepted.

// This would also apply if you were trying to load a macOS .dylib
// from Windows or a linux .so from macOS. It's just not gonna work,
// chief.

// There's probably not much reason to change this outside of
// improving compatibility with games in general, since if you just
// want to fix one specific game and you're smart enough to add to
// this then you're also smart enough to either use MiniFFI on a
// library made for your platform or, better yet, just write the
// functionality into MKXP and don't use MiniFFI/Win32API at all.

// No macOS/Linux support yet.


DWORD __stdcall
MKXP_GetCurrentThreadId(void)
NOP_VAL(DUMMY_VAL)

DWORD __stdcall
MKXP_GetWindowThreadProcessId(HWND hWnd, LPDWORD lpdwProcessId)
NOP_VAL(DUMMY_VAL)

HWND __stdcall
MKXP_FindWindowEx(HWND hWnd,
                  HWND hWndChildAfter,
                  LPCSTR lpszClass,
                  LPCSTR lpszWindow
                  )
{
    SDL_SysWMinfo wm;
    SDL_GetWindowWMInfo(shState->sdlWindow(), &wm);
    return wm.info.win.window;
}

DWORD __stdcall
MKXP_GetForegroundWindow(void)
{
    if (SDL_GetWindowFlags(shState->sdlWindow()) & SDL_WINDOW_INPUT_FOCUS)
    {
        return DUMMY_VAL;
    }
    return 0;
}

BOOL __stdcall
MKXP_GetClientRect(HWND hWnd, LPRECT lpRect)
{
    SDL_GetWindowSize(shState->sdlWindow(),
                      (int*)&lpRect->right,
                      (int*)&lpRect->bottom);
    return true;
}


// You would think that you could just call GetCursorPos
// and ScreenToClient with the window handle and lppoint
// and be fine, but nope
BOOL __stdcall
MKXP_GetCursorPos(LPPOINT lpPoint)
{
    SDL_GetGlobalMouseState((int*)&lpPoint->x, (int*)&lpPoint->y);
    return true;
}

BOOL __stdcall
MKXP_ScreenToClient(HWND hWnd, LPPOINT lpPoint)
{
    SDL_GetMouseState((int*)&lpPoint->x, (int*)&lpPoint->y);
    return true;
}

BOOL __stdcall
MKXP_SetWindowPos(HWND hWnd,
                  HWND hWndInsertAfter,
                  int X,
                  int Y,
                  int cx,
                  int cy,
                  UINT uFlags)
{
    // Setting window position still doesn't work with
    // SetWindowPos, but it still needs to be called
    // because Win32API.restoreScreen is picky about its
    // metrics
    SetWindowPos(hWnd, hWndInsertAfter, X, Y, cx, cy, uFlags);
    SDL_SetWindowPosition(shState->sdlWindow(), X, Y);
    return true;
}


// Games that use this to resize the window won't center
// themselves, but it's better than having the window sent
// so far into the corner that you can't even grab onto
// the title bar
BOOL __stdcall
MKXP_GetWindowRect(HWND hWnd, LPRECT lpRect)
{
    int cur_x, cur_y, cur_w, cur_h;
    SDL_GetWindowPosition(shState->sdlWindow(), &cur_x, &cur_y);
    SDL_GetWindowSize(shState->sdlWindow(), &cur_w, &cur_h);
    lpRect->left = cur_x;
    lpRect->right = cur_x + cur_w + 1;
    lpRect->top = cur_y;
    lpRect->bottom = cur_y + cur_h + 1;
    return true;
}

BOOL __stdcall
MKXP_RegisterHotKey(HWND hWnd,
                    int id,
                    UINT fsModifiers,
                    UINT vk)
NOP_VAL(true)
#endif
