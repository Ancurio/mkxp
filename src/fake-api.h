#pragma once

#define ABI(x) __attribute__((x))
#if defined(__WIN32__)

// Before, it needed to be ensured that functions called from
// MiniFFI/Win32API were using stdcall as a calling convention,
// but now that the stack pointer is automatically reset after
// getting the result, cdecl should work just fine too

// ... Oh, and by the way, Windows was a mistake

// #define PREFABI ABI(stdcall)
#else
#define PREFABI
#endif

#ifdef USE_FAKEAPI
#include <stddef.h>

#ifdef __WIN32__
#include <windows.h>
#endif

#ifndef __WIN32__
typedef unsigned int DWORD, UINT, *LPDWORD;
typedef char BYTE, *LPSTR, *LPCSTR, *LPCTSTR, *LPTSTR, *PBYTE;
typedef short SHORT;
typedef int LONG;

#ifdef __APPLE__
typedef signed char BOOL;
#else
typedef bool BOOL;
#endif

typedef void VOID, *LPVOID, *HANDLE, *HMODULE, *HWND;
typedef size_t SIZE_T;

typedef struct {
  LONG x;
  LONG y;
} POINT, *LPPOINT;

typedef struct {
  LONG left;
  LONG top;
  LONG right;
  LONG bottom;
} RECT, *PRECT, *NPRECT, *LPRECT;

typedef struct {
  BYTE ACLineStatus;
  BYTE BatteryFlag;
  BYTE BatteryLifePercent;
  BYTE SystemStatusFlag;
  DWORD BatteryLifeTime;
  DWORD BatteryFullLifeTime;
} SYSTEM_POWER_STATUS, *LPSYSTEM_POWER_STATUS;
#endif

#define DUMMY_VAL 571
#define NOP                                                                    \
  { return; }
#define NOP_VAL(x)                                                             \
  { return x; }

PREFABI DWORD MKXP_GetCurrentThreadId(void);

PREFABI DWORD MKXP_GetWindowThreadProcessId(HWND hWnd, LPDWORD lpdwProcessId);

PREFABI HWND MKXP_FindWindowEx(HWND hWnd, HWND hWndChildAfter, LPCSTR lpszClass,
                               LPCSTR lpszWindow);

PREFABI DWORD MKXP_GetForegroundWindow(void);

PREFABI BOOL MKXP_GetClientRect(HWND hWnd, LPRECT lpRect);

PREFABI BOOL MKXP_GetCursorPos(LPPOINT lpPoint);

PREFABI BOOL MKXP_ScreenToClient(HWND hWnd, LPPOINT lpPoint);

PREFABI BOOL MKXP_SetWindowPos(HWND hWnd, HWND hWndInsertAfter, int X, int Y,
                               int cx, int cy, UINT uFlags);

PREFABI BOOL MKXP_SetWindowTextA(HWND hWnd, LPCSTR lpString);

PREFABI BOOL MKXP_GetWindowRect(HWND hWnd, LPRECT lpRect);

PREFABI BOOL MKXP_GetKeyboardState(PBYTE lpKeyState);

#ifndef __WIN32__

PREFABI VOID MKXP_RtlMoveMemory(VOID *Destination, VOID *Source, SIZE_T Length);

PREFABI HMODULE MKXP_LoadLibrary(LPCSTR lpLibFileName);

PREFABI BOOL MKXP_FreeLibrary(HMODULE hLibModule);

PREFABI SHORT MKXP_GetAsyncKeyState(int vKey);

PREFABI BOOL
MKXP_GetSystemPowerStatus(LPSYSTEM_POWER_STATUS lpSystemPowerStatus);

PREFABI BOOL MKXP_ShowWindow(HWND hWnd, int nCmdShow);

PREFABI int MKXP_GetSystemMetrics(int nIndex);

PREFABI HWND MKXP_SetCapture(HWND hWnd);

PREFABI BOOL MKXP_ReleaseCapture(void);

PREFABI int MKXP_ShowCursor(BOOL bShow);

PREFABI DWORD MKXP_GetPrivateProfileString(LPCTSTR lpAppName, LPCTSTR lpKeyName,
                                           LPCTSTR lpDefault,
                                           LPTSTR lpReturnedString, DWORD nSize,
                                           LPCTSTR lpFileName);

PREFABI short MKXP_GetUserDefaultLangID(void);

PREFABI BOOL MKXP_GetUserName(LPSTR lpBuffer, LPDWORD pcbBuffer);

PREFABI BOOL MKXP_RegisterHotKey(HWND hWnd, int id, UINT fsModifiers, UINT vk);

PREFABI LONG MKXP_SetWindowLong(HWND hWnd, int nIndex, LONG dwNewLong);

#endif
#endif
