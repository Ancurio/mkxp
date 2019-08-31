#include <SDL.h>

#ifdef __WIN32__
#include <windows.h>
#include <SDL_syswm.h>
#else
#include <map>
#include <cstring>
#include "iniconfig.h"
#endif

#include "sharedstate.h"
#include "eventthread.h"
#include "filesystem.h"
#include "input.h"
#include "fake-api.h"


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

// A lot of functions here will probably be bound directly to Ruby
// eventually (so that you can just call Graphics.screenshot or
// something instead of having to make a messy Win32API call
// that *now* has to run entirely different function to even work)

// macOS/Linux support WIP.

// ===============================================================
// This map is for converting Windows virtual keycodes to SDL's
// scancodes. Only needs to exist on macOS and Linux to allow
// getAsyncKeyState to work.
// ===============================================================
#ifndef __WIN32__
#define m(vk,sc) { vk, SDL_SCANCODE_##sc }
std::map<int, int> vKeyToScancode{
    // 0x01 LEFT MOUSE
    // 0x02 RIGHT MOUSE
    m(0x03, CANCEL),
    // 0x04 MIDDLE MOUSE
    // 0x05 XBUTTON 1
    // 0x06 XBUTTON 2
    // 0x07 undef
    m(0x08, BACKSPACE),
    m(0x09, TAB),
    // 0x0a reserved
    // 0x0b reserved
    m(0x0c, CLEAR),
    m(0x0d, RETURN),
    // 0x0e undefined
    // 0x0f undefined
    // 0x10 SHIFT (both)
    // 0x11 CONTROL (both)
    // 0x12 ALT (both)
    m(0x13, PAUSE),
    m(0x14, CAPSLOCK),
    // 0x15 KANA, HANGUL
    // 0x16 undefined
    // 0x17 JUNJA
    // 0x18 FINAL
    // 0x19 HANJA, KANJI
    // 0x1a undefined
    m(0x1b, ESCAPE),
    // 0x1c CONVERT
    // 0x1d NONCONVERT
    // 0x1e ACCEPT
    // 0x1f MODECHANGE
    m(0x20, SPACE),
    m(0x21, PAGEUP),
    m(0x22, PAGEDOWN),
    m(0x23, END),
    m(0x24, HOME),
    m(0x25, LEFT),
    m(0x26, UP),
    m(0x27, RIGHT),
    m(0x28, DOWN),
    m(0x29, SELECT),
    // 0x2A print
    m(0x2b, EXECUTE),
    m(0x2c, PRINTSCREEN),
    m(0x2d, INSERT),
    m(0x2e, DELETE),
    m(0x2f, HELP),
    m(0x30, 0), m(0x31, 1),
    m(0x32, 2), m(0x33, 3),
    m(0x34, 4), m(0x35, 5),
    m(0x36, 6), m(0x37, 7),
    m(0x38, 8), m(0x39, 9),
    // 0x3a-0x40 undefined
    m(0x41, A), m(0x42, B),
    m(0x43, C), m(0x44, D),
    m(0x45, E), m(0x46, F),
    m(0x47, G), m(0x48, H),
    m(0x49, I), m(0x4a, J),
    m(0x4b, K), m(0x4c, L),
    m(0x4d, M), m(0x4e, N),
    m(0x4f, O), m(0x50, P),
    m(0x51, Q), m(0x52, R),
    m(0x53, S), m(0x54, T),
    m(0x55, U), m(0x56, V),
    m(0x57, W), m(0x58, X),
    m(0x59, Y), m(0x5a, Z),
    m(0x5b, LGUI), m(0x5c, RGUI),
    m(0x5d, APPLICATION),
    // 0x5e reserved
    m(0x5f, SLEEP),
    m(0x60, KP_0), m(0x61, KP_1),
    m(0x62, KP_2), m(0x63, KP_3),
    m(0x64, KP_4), m(0x65, KP_5),
    m(0x66, KP_6), m(0x67, KP_7),
    m(0x68, KP_8), m(0x69, KP_9),
    m(0x6a, KP_MULTIPLY),
    m(0x6b, KP_PLUS),
    m(0x6c, RETURN), // SEPARATOR
    m(0x6d, KP_MINUS),
    m(0x6e, KP_DECIMAL),
    m(0x6f, KP_DIVIDE),
    m(0x70, F1), m(0x71, F2),
    m(0x72, F3), m(0x73, F4),
    m(0x74, F5), m(0x75, F6),
    m(0x76, F7), m(0x77, F8),
    m(0x78, F9), m(0x79, F10),
    m(0x7a, F11), m(0x7b, F12),
    m(0x7c, F13), m(0x7d, F14),
    m(0x7e, F15), m(0x7f, F16),
    m(0x80, F17), m(0x81, F18),
    m(0x82, F19), m(0x83, F20),
    m(0x84, F21), m(0x85, F22),
    m(0x86, F23), m(0x87, F24),
    // 0x88-0x8f unassigned
    m(0x90, NUMLOCKCLEAR),
    m(0x91, SCROLLLOCK),
    // 0x92-0x96 oem specific
    // 0x97-0x9f unassigned
    m(0xa0, LSHIFT),
    m(0xa1, RSHIFT),
    m(0xa2, LCTRL),
    m(0xa3, RCTRL),
    m(0xa4, LALT),
    m(0xa5, RALT),
    m(0xa6, AC_BACK),
    m(0xa7, AC_FORWARD),
    m(0xa8, AC_REFRESH),
    m(0xa9, AC_STOP),
    m(0xaa, AC_SEARCH),
    // 0xab BROWSER_FAVORITES
    m(0xac, AC_HOME),
    m(0xad, AUDIOMUTE),
    m(0xae, VOLUMEDOWN),
    m(0xaf, VOLUMEUP),
    m(0xb0, AUDIONEXT),
    m(0xb1, AUDIOPREV),
    m(0xb2, AUDIOSTOP),
    m(0xb3, AUDIOPLAY),
    m(0xb4, MAIL),
    m(0xb5, MEDIASELECT),
    // 0xb6 LAUNCH_APP1
    // 0xb7 LAUNCH_APP2
    // 0xb8-0xb9 reserved
    
    // Everything below here is OEM
    // and can vary by country
    m(0xba, SEMICOLON),
    m(0xbb, EQUALS),
    m(0xbc, COMMA),
    m(0xbd, MINUS),
    m(0xbe, PERIOD),
    m(0xbf, SLASH),
    m(0xc0, GRAVE),
    // 0xc1-0xd7 reserved
    // 0xd8-0xda unassigned
    m(0xdb, LEFTBRACKET),
    m(0xdc, BACKSLASH),
    m(0xdd, RIGHTBRACKET),
    m(0xde, APOSTROPHE),
    // 0xdf OEM_8
    // 0xe0 reserved
    // 0xe1 oem-specific
    // 0xe2 OEM_102
    // 0xe3-0xe4 oem-specific
    // 0xe5 PROCESSKEY
    // 0xe6 oem-specific
    // 0xe7 PACKET
    // 0xe8 unassigned
    // 0xe9-0xf5 oem_specific
    // 0xf6 ATTN
    m(0xf7, CRSEL),
    m(0xf8, EXSEL),
    // 0xf9 EREOF
    m(0xfa, AUDIOPLAY), // PLAY, guessing
    // 0xfb ZOOM
    // 0xfc NONAME
    // 0xfd PA1
    m(0xfe, CLEAR)
};
#undef m
#endif

PREFABI DWORD
MKXP_GetCurrentThreadId(void)
NOP_VAL(DUMMY_VAL)

PREFABI DWORD
MKXP_GetWindowThreadProcessId(HWND hWnd, LPDWORD lpdwProcessId)
NOP_VAL(DUMMY_VAL)

PREFABI HWND
MKXP_FindWindowEx(HWND hWnd,
                  HWND hWndChildAfter,
                  LPCSTR lpszClass,
                  LPCSTR lpszWindow
                  )
#ifdef __WIN32__
{
    SDL_SysWMinfo wm;
    SDL_GetWindowWMInfo(shState->sdlWindow(), &wm);
    return wm.info.win.window;
}
#else
NOP_VAL((HWND)DUMMY_VAL)
#endif

PREFABI DWORD
MKXP_GetForegroundWindow(void)
{
    if (SDL_GetWindowFlags(shState->sdlWindow()) & SDL_WINDOW_INPUT_FOCUS)
    {
        return DUMMY_VAL;
    }
    return 0;
}

PREFABI BOOL
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
PREFABI BOOL
MKXP_GetCursorPos(LPPOINT lpPoint)
{
    SDL_GetGlobalMouseState((int*)&lpPoint->x, (int*)&lpPoint->y);
    return true;
}

PREFABI BOOL
MKXP_ScreenToClient(HWND hWnd, LPPOINT lpPoint)
{
    lpPoint->x = shState->input().mouseX();
    lpPoint->y = shState->input().mouseY();
    return true;
}

PREFABI BOOL
MKXP_SetWindowPos(HWND hWnd,
                  HWND hWndInsertAfter,
                  int X,
                  int Y,
                  int cx,
                  int cy,
                  UINT uFlags)
{
    // The game calls resize_screen which will automatically
    // ... well, resize the screen
    //shState->eThread().requestWindowResize(cx, cy);
    
    shState->eThread().requestWindowReposition(X, Y);
    return true;
}

PREFABI BOOL
MKXP_SetWindowTextA(HWND hWnd, LPCSTR lpString)
{
    SDL_SetWindowTitle(shState->sdlWindow(), (const char*)lpString);
    return true;
}


PREFABI BOOL
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

PREFABI BOOL
MKXP_RegisterHotKey(HWND hWnd,
                    int id,
                    UINT fsModifiers,
                    UINT vk)
NOP_VAL(true)

PREFABI LONG
MKXP_SetWindowLong(HWND hWnd, int nIndex, LONG dwNewLong)
{
#ifdef __WIN32__
    return SetWindowLong(hWnd, nIndex, dwNewLong);
#else
    if (nIndex == -16)
    {
        if (dwNewLong == 0)
        {
            shState->eThread().requestFullscreenMode(true);
        }
        else if (dwNewLong == 0x14ca0000)
        {
            shState->eThread().requestFullscreenMode(false);
        }
    }
    return DUMMY_VAL;
#endif
};

// Shift key with GetKeyboardState doesn't work for whatever reason,
// so Windows needs this too
#define ks(sc) shState->eThread().keyStates[SDL_SCANCODE_##sc]
PREFABI BOOL
MKXP_GetKeyboardState(PBYTE lpKeyState)
{
#ifdef __WIN32__
    bool rc = GetKeyboardState(lpKeyState);
    if (rc)
    {
        lpKeyState[VK_LSHIFT] = ks(LSHIFT) << 7;
        lpKeyState[VK_RSHIFT] = ks(RSHIFT) << 7;
        lpKeyState[VK_SHIFT] = (lpKeyState[VK_LSHIFT] || lpKeyState[VK_RSHIFT]) ? 0x80 : 0;
    }
    return rc;
#else
    for (int i = 254; i > 0; i--)
    {
        lpKeyState[i] = (MKXP_GetAsyncKeyState(i)) ? 0x80 : 0;
    }
    return true;
#endif
}

// =========================================
// macOS / Linux only stuff starts here
// =========================================

#ifndef __WIN32__

PREFABI VOID
MKXP_RtlMoveMemory(VOID *Destination, VOID *Source, SIZE_T Length)
{
    // I have no idea if this is a good idea or not
    // or why it's even necessary in the first place,
    // getting rid of this is priority #1
    memcpy(Destination, Source, Length);
}

// I don't know who's more crazy, them for writing this stuff
// or me for being willing to put it here.

// Probably me.

PREFABI HMODULE
MKXP_LoadLibrary(LPCSTR lpLibFileName)
{
    return SDL_LoadObject(lpLibFileName);
}

PREFABI BOOL
MKXP_FreeLibrary(HMODULE hLibModule)
{
    SDL_UnloadObject(hLibModule);
    return true;
}


// Luckily, Essentials only cares about the high-order bit,
// so SDL's keystates will work perfectly fine
PREFABI SHORT
MKXP_GetAsyncKeyState(int vKey)
{
    SHORT result;
    switch (vKey) {
        case 0x10: // Any Shift
        result = (ks(LSHIFT) || ks(RSHIFT)) ? 0x8000 : 0;
        break;
        
        case 0x11: // Any Ctrl
        result = (ks(LCTRL) || ks(RCTRL)) ? 0x8000 : 0;
        break;
        
        case 0x12: // Any Alt
        result = (ks(LALT) || ks(RALT)) ? 0x8000 : 0;
        break;
        
        case 0x1: // Mouse button 1
        result = (SDL_GetMouseState(0,0) & SDL_BUTTON(1)) ? 0x8000 : 0;
        break;
        
        case 0x2: // Mouse button 2
        result = (SDL_GetMouseState(0,0) & SDL_BUTTON(3)) ? 0x8000 : 0;
        break;
        
        case 0x4: // Middle mouse
        result = (SDL_GetMouseState(0,0) & SDL_BUTTON(2)) ? 0x8000 : 0;
        break;
        
        default:
        try {
            result = shState->eThread().keyStates[vKeyToScancode[vKey]] << 15;
        }
        catch (...) {
            result = 0;
        }
        break;
    }
    return result;
}
#undef ks

PREFABI BOOL
MKXP_GetSystemPowerStatus(LPSYSTEM_POWER_STATUS lpSystemPowerStatus)
{
    int seconds, percent;
    SDL_PowerState ps;
    ps = SDL_GetPowerInfo(&seconds, &percent);
    
    // Setting ACLineStatus
    if (ps == SDL_POWERSTATE_UNKNOWN)
    {
        lpSystemPowerStatus->ACLineStatus = 0xFF;
    }
    else
    {
        lpSystemPowerStatus->ACLineStatus =
            (ps == SDL_POWERSTATE_ON_BATTERY) ? 1 : 0;
    }
    
    // Setting BatteryFlag
    if (ps == SDL_POWERSTATE_ON_BATTERY)
    {
        if (percent == -1)
        {
            lpSystemPowerStatus->BatteryFlag = 0xFF;
        }
        else if (percent >= 33)
        {
            lpSystemPowerStatus->BatteryFlag = 1;
        }
        else if (4 < percent && percent < 33)
        {
            lpSystemPowerStatus->BatteryFlag = 2;
        }
        else
        {
            lpSystemPowerStatus->BatteryFlag = 4;
        }
    }
    else if (ps == SDL_POWERSTATE_CHARGING)
    {
        lpSystemPowerStatus->BatteryFlag = 8;
    }
    else if (ps == SDL_POWERSTATE_NO_BATTERY)
    {
        lpSystemPowerStatus->BatteryFlag = (BYTE)128;
    }
    else
    {
        lpSystemPowerStatus->BatteryFlag = 0xFF;
    }
    
    // Setting BatteryLifePercent
    lpSystemPowerStatus->BatteryLifePercent = (percent != -1) ? (BYTE)percent : 0xFF;
    
    // Setting SystemStatusFlag
    lpSystemPowerStatus->SystemStatusFlag = 0;
    
    // Setting BatteryLifeTime
    lpSystemPowerStatus->BatteryLifeTime = seconds;
    
    // Setting BatteryFullLifeTime
    lpSystemPowerStatus->BatteryFullLifeTime = -1;
    
    return true;
}

PREFABI BOOL
MKXP_ShowWindow(HWND hWnd, int nCmdShow)
NOP_VAL(true);


// This only currently supports getting screen width/height
// Not really motivated to do the other ones when I'll be
// extending Ruby at a later time anyway
PREFABI int
MKXP_GetSystemMetrics(int nIndex)
{
    SDL_DisplayMode dm = {0};
    int rc = SDL_GetDesktopDisplayMode(
                              SDL_GetWindowDisplayIndex(shState->sdlWindow()),
                              &dm
                            );
    if (!rc)
    {
        switch (nIndex) {
            case 0:
            return dm.w;
            break;
            
            case 1:
            return dm.h;
            break;
            
            default:
            return -1;
            break;
        }
    }
    
    return -1;
}

PREFABI HWND
MKXP_SetCapture(HWND hWnd)
NOP_VAL((HWND)DUMMY_VAL);

PREFABI BOOL
MKXP_ReleaseCapture(void)
NOP_VAL(true);

PREFABI int
MKXP_ShowCursor(BOOL bShow)
NOP_VAL(DUMMY_VAL);

PREFABI DWORD
MKXP_GetPrivateProfileString(LPCTSTR lpAppName,
                             LPCTSTR lpKeyName,
                             LPCTSTR lpDefault,
                             LPTSTR lpReturnedString,
                             DWORD nSize,
                             LPCTSTR lpFileName)
{
    char *lpFileName_normal = shState->fileSystem().normalize(lpFileName, true, false);
    SDLRWStream iniFile(lpFileName_normal, "r");
    delete lpFileName_normal;
    if (iniFile)
    {
        INIConfiguration ic;
        if (ic.load(iniFile.stream()))
        {
            std::string result = ic.getStringProperty(lpAppName, lpKeyName);
            if (!result.empty())
            {
                strncpy(lpReturnedString, result.c_str(), nSize);
                return result.length();
            }
        }
    }
    strncpy(lpReturnedString, lpDefault, nSize);
    return strlen(lpDefault);
}


// Only supports English, other languages are too
// much work to keep in hacky code like this

PREFABI short // I know it's a LANGID but I don't care
MKXP_GetUserDefaultLangID(void)
NOP_VAL(0xC09);

#endif
