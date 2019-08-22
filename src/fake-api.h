#pragma once

#ifdef __WIN32__
#include <windows.h>
#else
#include <map>
#endif

#define ABI(x) __attribute__((x))
#if defined(__WIN32__)
#define PREFABI ABI(stdcall)
#else
#define PREFABI
#endif



#ifndef __WIN32__
typedef unsigned int HWND, DWORD, UINT, *LPDWORD;
typedef char *LPCSTR, *PBYTE;
typedef int LONG;
typedef bool BOOL;
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
typedef void VOID, *LPVOID, *HANDLE, *HMODULE;
typedef size_t SIZE_T;

extern std::map<int, int> vKeyToScancode;
#endif

#define DUMMY_VAL 571
#define NOP \
{ \
return; \
}
#define NOP_VAL(x) \
{ \
return x; \
}

PREFABI DWORD
MKXP_GetCurrentThreadId(void);

PREFABI DWORD
MKXP_GetWindowThreadProcessId(HWND hWnd, LPDWORD lpdwProcessId);

PREFABI HWND
MKXP_FindWindowEx(HWND hWnd,
                  HWND hWndChildAfter,
                  LPCSTR lpszClass,
                  LPCSTR lpszWindow
                  );

PREFABI DWORD
MKXP_GetForegroundWindow(void);

PREFABI BOOL
MKXP_GetClientRect(HWND hWnd, LPRECT lpRect);

PREFABI BOOL
MKXP_GetCursorPos(LPPOINT lpPoint);

PREFABI BOOL
MKXP_ScreenToClient(HWND hWnd, LPPOINT lpPoint);

PREFABI BOOL
MKXP_SetWindowPos(HWND hWnd,
                  HWND hWndInsertAfter,
                  int X,
                  int Y,
                  int cx,
                  int cy,
                  UINT uFlags);

PREFABI BOOL
MKXP_SetWindowTextA(HWND hWnd, LPCSTR lpString);

PREFABI BOOL
MKXP_GetWindowRect(HWND hWnd, LPRECT lpRect);

PREFABI BOOL
MKXP_RegisterHotKey(HWND hWnd,
                    int id,
                    UINT fsModifiers,
                    UINT vk);

PREFABI BOOL
MKXP_GetKeyboardState(PBYTE lpKeyState);

#ifndef __WIN32__

PREFABI VOID
MKXP_RtlMoveMemory(VOID *Destination, VOID *Source, SIZE_T Length);

PREFABI HMODULE
MKXP_LoadLibrary(LPCSTR lpLibFileName);

PREFABI BOOL
MKXP_FreeLibrary(HMODULE hLibModule);

#endif
