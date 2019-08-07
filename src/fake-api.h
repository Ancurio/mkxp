#pragma once
#ifdef __WIN32__
#include <windows.h>

/*
#ifndef __WIN32__
typedef unsigned long HWND;
typedef unsigned int DWORD;
typedef unsigned int* LPDWORD;
typedef char* LPCSTR;
typedef int LONG;
typedef bool BOOL;
typedef unsigned int UINT;
typedef struct {
    LONG x;
    LONG y;
} POINT;
typedef POINT* LPPOINT;
#endif
*/

#define DUMMY_VAL 571
#define NOP \
{ \
return; \
}
#define NOP_VAL(x) \
{ \
return x; \
}

DWORD __stdcall
MKXP_GetCurrentThreadId(void);

DWORD __stdcall
MKXP_GetWindowThreadProcessId(HWND hWnd, LPDWORD lpdwProcessId);

HWND __stdcall
MKXP_FindWindowEx(HWND hWnd,
                  HWND hWndChildAfter,
                  LPCSTR lpszClass,
                  LPCSTR lpszWindow
                  );

DWORD __stdcall
MKXP_GetForegroundWindow(void);

BOOL __stdcall
MKXP_GetClientRect(HWND hWnd, LPRECT lpRect);

BOOL __stdcall
MKXP_GetCursorPos(LPPOINT lpPoint);

BOOL __stdcall
MKXP_ScreenToClient(HWND hWnd, LPPOINT lpPoint);

BOOL __stdcall
MKXP_SetWindowPos(HWND hWnd,
                  HWND hWndInsertAfter,
                  int X,
                  int Y,
                  int cx,
                  int cy,
                  UINT uFlags);

BOOL __stdcall
MKXP_RegisterHotKey(HWND hWnd,
                    int id,
                    UINT fsModifiers,
                    UINT vk);
#endif
