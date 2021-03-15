#ifndef WIN_CONSOLEUTIL_H
#define WIN_CONSOLEUTIL_H

#include <iostream>
#include "windows.h"

// Attempts to allocate a console and fetch the output handle.
// Returns whether the operation was successful.
// Extended error information can be received via GetLastError().
bool setupWindowsConsole(HANDLE &handle)
{
    if (!AllocConsole())
        return false;
        
    handle = GetStdHandle(STD_OUTPUT_HANDLE);

    return (handle != NULL && handle != INVALID_HANDLE_VALUE);
}

// Reopens the file streams. This should be done after successfully
// setting up the console.
void reopenWindowsStreams()
{
    FILE* fDummy;
    freopen_s(&fDummy, "CONOUT$", "w", stdout);
    freopen_s(&fDummy, "CONOUT$", "w", stderr);
    freopen_s(&fDummy, "CONIN$", "r", stdin);
    std::cout.clear();
    std::clog.clear();
    std::cerr.clear();
    std::cin.clear();
}

#endif