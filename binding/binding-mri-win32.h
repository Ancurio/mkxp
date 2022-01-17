#ifndef BINDING_MRI_WIN32_H
#define BINDING_MRI_WIN32_H

#include <windows.h>
#include "util/win-consoleutils.h"

// Attempts to set $stdout and $stdin accordingly on Windows. Only
// called when debug mode is on, since that's when the console
// should be active.
void configureWindowsStreams() {
    const int stdoutFD = getStdFD(STD_OUTPUT_HANDLE);

    // Configure $stdout
    if (stdoutFD >= 0) {
        VALUE winStdout = rb_funcall(rb_cIO, rb_intern("new"), 2,
            INT2NUM(stdoutFD), rb_str_new_cstr("w+"));

        rb_gv_set("stdout", winStdout);
    }

    const int stdinFD = getStdFD(STD_INPUT_HANDLE);

    // Configure $stdin
    if (stdinFD >= 0) {
        VALUE winStdin = rb_funcall(rb_cIO, rb_intern("new"), 2,
            INT2NUM(stdinFD), rb_str_new_cstr("r"));

        rb_gv_set("stdin", winStdin);
    }

    const int stderrFD = getStdFD(STD_ERROR_HANDLE);

    // Configure $stderr
    if (stderrFD >= 0) {
        VALUE winStderr = rb_funcall(rb_cIO, rb_intern("new"), 2,
            INT2NUM(stderrFD), rb_str_new_cstr("w+"));

        rb_gv_set("stderr", winStderr);
    }
}

#endif
