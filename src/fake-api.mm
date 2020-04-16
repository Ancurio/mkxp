#import <SDL.h>
#ifdef __APPLE__
#import <Foundation/Foundation.h>
#endif
#import <ObjFW/ObjFW.h>

#ifdef __WIN32__
#import <SDL_syswm.h>
#import <windows.h>
#else
#import <cstring>
#endif

#import "eventthread.h"
#import "fake-api.h"
#import "filesystem.h"
#import "input.h"
#import "lang-fun.h"
#import "sharedstate.h"

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

PREFABI DWORD MKXP_GetCurrentThreadId(void) NOP_VAL(DUMMY_VAL)

    PREFABI DWORD
    MKXP_GetWindowThreadProcessId(HWND hWnd, LPDWORD lpdwProcessId)
        NOP_VAL(DUMMY_VAL)

            PREFABI HWND MKXP_FindWindowEx(HWND hWnd, HWND hWndChildAfter,
                                           LPCSTR lpszClass, LPCSTR lpszWindow)
#ifdef __WIN32__
{
  SDL_SysWMinfo wm;
  SDL_GetWindowWMInfo(shState->sdlWindow(), &wm);
  return wm.info.win.window;
}
#else
                NOP_VAL((HWND)DUMMY_VAL)
#endif

PREFABI DWORD MKXP_GetForegroundWindow(void) {
  if (SDL_GetWindowFlags(shState->sdlWindow()) & SDL_WINDOW_INPUT_FOCUS) {
    return DUMMY_VAL;
  }
  return 0;
}

PREFABI BOOL MKXP_GetClientRect(HWND hWnd, LPRECT lpRect) {
  SDL_GetWindowSize(shState->sdlWindow(), (int *)&lpRect->right,
                    (int *)&lpRect->bottom);
  return true;
}

// You would think that you could just call GetCursorPos
// and ScreenToClient with the window handle and lppoint
// and be fine, but nope
PREFABI BOOL MKXP_GetCursorPos(LPPOINT lpPoint) {
  SDL_GetGlobalMouseState((int *)&lpPoint->x, (int *)&lpPoint->y);
  return true;
}

PREFABI BOOL MKXP_ScreenToClient(HWND hWnd, LPPOINT lpPoint) {
  lpPoint->x = shState->input().mouseX();
  lpPoint->y = shState->input().mouseY();
  return true;
}

PREFABI BOOL MKXP_SetWindowPos(HWND hWnd, HWND hWndInsertAfter, int X, int Y,
                               int cx, int cy, UINT uFlags) {
  // The game calls resize_screen which will automatically
  // ... well, resize the screen
  // shState->eThread().requestWindowResize(cx, cy);

  shState->eThread().requestWindowReposition(X, Y);
  return true;
}

PREFABI BOOL MKXP_SetWindowTextA(HWND hWnd, LPCSTR lpString) {
  shState->eThread().requestWindowRename((const char *)lpString);
  return true;
}

PREFABI BOOL MKXP_GetWindowRect(HWND hWnd, LPRECT lpRect) {
  int cur_x, cur_y, cur_w, cur_h;
  SDL_GetWindowPosition(shState->sdlWindow(), &cur_x, &cur_y);
  SDL_GetWindowSize(shState->sdlWindow(), &cur_w, &cur_h);
  lpRect->left = cur_x;
  lpRect->right = cur_x + cur_w + 1;
  lpRect->top = cur_y;
  lpRect->bottom = cur_y + cur_h + 1;
  return true;
}

// Shift key with GetKeyboardState doesn't work for whatever reason,
// so Windows needs this too
#define ks(sc) shState->eThread().keyStates[SDL_SCANCODE_##sc]
PREFABI BOOL MKXP_GetKeyboardState(PBYTE lpKeyState) {
#ifdef __WIN32__
  bool rc = GetKeyboardState(lpKeyState);
  if (rc) {
    lpKeyState[VK_LSHIFT] = ks(LSHIFT) << 7;
    lpKeyState[VK_RSHIFT] = ks(RSHIFT) << 7;
    lpKeyState[VK_SHIFT] =
        (lpKeyState[VK_LSHIFT] || lpKeyState[VK_RSHIFT]) ? 0x80 : 0;
  }
  return rc;
#else
  for (int i = 254; i > 0; i--) {
    lpKeyState[i] = (MKXP_GetAsyncKeyState(i)) ? 0x80 : 0;
  }
  return true;
#endif
}

// =========================================
// macOS / Linux only stuff starts here
// =========================================

#ifndef __WIN32__

PREFABI VOID MKXP_RtlMoveMemory(VOID *Destination, VOID *Source,
                                SIZE_T Length) {
  // I have no idea if this is a good idea or not
  // or why it's even necessary in the first place,
  // getting rid of this is priority #1
  memcpy(Destination, Source, Length);
}

// I don't know who's more crazy, them for writing this stuff
// or me for being willing to put it here.

// Probably me.

PREFABI HMODULE MKXP_LoadLibrary(LPCSTR lpLibFileName) {
  return SDL_LoadObject(lpLibFileName);
}

PREFABI BOOL MKXP_FreeLibrary(HMODULE hLibModule) {
  SDL_UnloadObject(hLibModule);
  return true;
}

// Luckily, Essentials only cares about the high-order bit,
// so SDL's keystates will work perfectly fine
PREFABI SHORT MKXP_GetAsyncKeyState(int vKey) {
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
    result = (SDL_GetMouseState(0, 0) & SDL_BUTTON(1)) ? 0x8000 : 0;
    break;

  case 0x2: // Mouse button 2
    result = (SDL_GetMouseState(0, 0) & SDL_BUTTON(3)) ? 0x8000 : 0;
    break;

  case 0x4: // Middle mouse
    result = (SDL_GetMouseState(0, 0) & SDL_BUTTON(2)) ? 0x8000 : 0;
    break;

  default:
    try {
      // Use EventThread instead of Input because
      // Input.update typically gets overridden
      result = shState->eThread().keyStates[vKeyToScancode[vKey]] << 15;
    } catch (...) {
      result = 0;
    }
    break;
  }
  return result;
}
#undef ks

PREFABI BOOL
MKXP_GetSystemPowerStatus(LPSYSTEM_POWER_STATUS lpSystemPowerStatus) {
  int seconds, percent;
  SDL_PowerState ps;
  ps = SDL_GetPowerInfo(&seconds, &percent);

  // Setting ACLineStatus
  if (ps == SDL_POWERSTATE_UNKNOWN) {
    lpSystemPowerStatus->ACLineStatus = 0xFF;
  } else {
    lpSystemPowerStatus->ACLineStatus =
        (ps == SDL_POWERSTATE_ON_BATTERY) ? 1 : 0;
  }

  // Setting BatteryFlag
  if (ps == SDL_POWERSTATE_ON_BATTERY) {
    if (percent == -1) {
      lpSystemPowerStatus->BatteryFlag = 0xFF;
    } else if (percent >= 33) {
      lpSystemPowerStatus->BatteryFlag = 1;
    } else if (4 < percent && percent < 33) {
      lpSystemPowerStatus->BatteryFlag = 2;
    } else {
      lpSystemPowerStatus->BatteryFlag = 4;
    }
  } else if (ps == SDL_POWERSTATE_CHARGING) {
    lpSystemPowerStatus->BatteryFlag = 8;
  } else if (ps == SDL_POWERSTATE_NO_BATTERY) {
    lpSystemPowerStatus->BatteryFlag = (BYTE)128;
  } else {
    lpSystemPowerStatus->BatteryFlag = 0xFF;
  }

  // Setting BatteryLifePercent
  lpSystemPowerStatus->BatteryLifePercent =
      (percent != -1) ? (BYTE)percent : 0xFF;

  // Setting SystemStatusFlag
  lpSystemPowerStatus->SystemStatusFlag = 0;

  // Setting BatteryLifeTime
  lpSystemPowerStatus->BatteryLifeTime = seconds;

  // Setting BatteryFullLifeTime
  lpSystemPowerStatus->BatteryFullLifeTime = -1;

  return true;
}

PREFABI BOOL MKXP_ShowWindow(HWND hWnd, int nCmdShow) NOP_VAL(true);

// This only currently supports getting screen width/height
// Not really motivated to do the other ones when I'll be
// extending Ruby at a later time anyway
PREFABI int MKXP_GetSystemMetrics(int nIndex) {
  SDL_DisplayMode dm = {0};
  int rc = SDL_GetDesktopDisplayMode(
      SDL_GetWindowDisplayIndex(shState->sdlWindow()), &dm);
  if (!rc) {
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

PREFABI HWND MKXP_SetCapture(HWND hWnd) NOP_VAL((HWND)DUMMY_VAL);

PREFABI BOOL MKXP_ReleaseCapture(void) NOP_VAL(true);

PREFABI int MKXP_ShowCursor(BOOL bShow) NOP_VAL(DUMMY_VAL);

PREFABI DWORD MKXP_GetPrivateProfileString(LPCTSTR lpAppName, LPCTSTR lpKeyName,
                                           LPCTSTR lpDefault,
                                           LPTSTR lpReturnedString, DWORD nSize,
                                           LPCTSTR lpFileName) {
  OFString *filePath = [OFString stringWithUTF8String:lpFileName];
  OFString *ret = 0;
  if ([OFFileManager.defaultManager fileExistsAtPath:filePath]) {
    @try {
      OFINIFile *iniFile = [OFINIFile fileWithPath:filePath];
      OFINICategory *iniCat =
          [iniFile categoryForName:[OFString stringWithUTF8String:lpAppName]];
      ret = [iniCat stringForKey:[OFString stringWithUTF8String:lpKeyName]];
    } @catch (...) {
    }
  }
  if (ret) {
    strncpy(lpReturnedString, ret.UTF8String, nSize);
  } else {
    strncpy(lpReturnedString, lpDefault, nSize);
  }
  return strlen(lpDefault);
}

// Does not handle sublanguages, only returns a few
// common languages

// Use MKXP.user_language instead

PREFABI short // I know it's a LANGID but I don't care
MKXP_GetUserDefaultLangID(void) {
  char buf[50];
  strncpy(buf, getUserLanguage(), sizeof(buf));

  for (int i = 0; i < strlen(buf); i++) {
    if (buf[i] == '_') {
      buf[i] = 0;
      break;
    }
  }

#define MATCH(l, c)                                                            \
  if (!strcmp(l, buf))                                                         \
    return (c & 0x3ff);
  MATCH("ja", 0x11);
  MATCH("en", 0x09);
  MATCH("fr", 0x0c);
  MATCH("it", 0x10);
  MATCH("de", 0x07);
  MATCH("es", 0x0a);
  MATCH("ko", 0x12);
  MATCH("pt", 0x16);
  MATCH("zh", 0x04);
#undef MATCH
  return 0x09;
}

PREFABI BOOL MKXP_GetUserName(LPSTR lpBuffer, LPDWORD pcbBuffer) {
  if (*pcbBuffer < 1)
    return false;
#ifdef __APPLE__
  strncpy(lpBuffer, NSUserName().UTF8String, *pcbBuffer);
#else
  char *username = getenv("USER");
  strncpy(lpBuffer, (username) ? username : "ditto", *pcbBuffer);
  lpBuffer[0] = toupper(lpBuffer[0]);
#endif
  return true;
}

PREFABI BOOL MKXP_RegisterHotKey(HWND hWnd, int id, UINT fsModifiers, UINT vk)
    NOP_VAL(true);

PREFABI LONG MKXP_SetWindowLong(HWND hWnd, int nIndex, LONG dwNewLong) {
  if (nIndex == -16) {
    if (dwNewLong == 0) {
      shState->eThread().requestFullscreenMode(true);
    } else if (dwNewLong == 0x14ca0000) {
      shState->eThread().requestFullscreenMode(false);
    }
  }
  return DUMMY_VAL;
};

#endif
