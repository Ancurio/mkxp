#if __WIN32__

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

static FILE *outStream;
static FILE *inStream;
static FILE *errStream;

static int stdoutFD = -1;
static int stderrFD = -1;
static int stdinFD  = -1;

static int openStdHandle(const DWORD &nStdHandle);

// Reopens the file streams. This should be done after successfully
// setting up the console.
void reopenWindowsStreams()
{
    freopen_s(&outStream, "CONOUT$", "w+", stdout);
    freopen_s(&errStream, "CONOUT$", "w+", stderr);
    freopen_s(&inStream, "CONIN$", "r", stdin);
    std::cout.clear();
    std::clog.clear();
    std::cerr.clear();
    std::cin.clear();

    stdoutFD = openStdHandle(STD_OUTPUT_HANDLE);
    stdinFD = openStdHandle(STD_INPUT_HANDLE);
    stderrFD = openStdHandle(STD_ERROR_HANDLE);
}

int getStdFD(const DWORD &nStdHandle)
{
    switch (nStdHandle)
    {
        case STD_OUTPUT_HANDLE:
            return stdoutFD;
        case STD_INPUT_HANDLE:
            return stdinFD;
        case STD_ERROR_HANDLE:
            return stderrFD;
        default:
            return -1;
    }
}

static int openStdHandle(const DWORD &nStdHandle)
{
    const HANDLE handle = GetStdHandle(nStdHandle);

    if (!handle || handle == INVALID_HANDLE_VALUE)
        return -1;

    return _open_osfhandle((intptr_t)handle, _O_TEXT);
}

#endif // __WIN32__
