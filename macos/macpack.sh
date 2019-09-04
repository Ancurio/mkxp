#!/bin/bash

EXE=${MESON_INSTALL_PREFIX}/Contents/MacOS/mkxp-z
install_name_tool -add_rpath "@executable_path/../libs" $EXE

macpack -v $EXE
