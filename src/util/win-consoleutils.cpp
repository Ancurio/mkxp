#include "win-consoleutils.h"

// Attempts to allocate a console and fetch the output handle.
// Returns whether the operation was successful.
// Extended error information can be received via GetLastError().
bool setupWindowsConsole()
{
    if (!AllocConsole())
        return false;
        
    const HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);

    return (handle != NULL && handle != INVALID_HANDLE_VALUE);
}

FILE *outStream;
FILE *inStream;
FILE *errStream;

// Reopens the file streams. This should be done after successfully
// setting up the console.
void reopenWindowsStreams()
{
    freopen_s(&outStream, "CONOUT$", "w+", stdout);
    freopen_s(&errStream, "CONOUT$", "w+", stderr);
    freopen_s(&inStream, "CONIN$", "r", stdin);
}
