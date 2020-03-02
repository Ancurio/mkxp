#!/bin/bash

EXE=${MESON_INSTALL_PREFIX}/Contents/MacOS/mkxp-z
install_name_tool -add_rpath "@executable_path/../libs" $EXE

macpack $EXE

if [ -n "$1" ]; then
  echo "Setting up steam_api manually..."
  cp "$1/libsteam_api.dylib" "${MESON_INSTALL_PREFIX}/Contents/libs"
  install_name_tool -change "@loader_path/libsteam_api.dylib" "@executable_path/../libs/libsteam_api.dylib" $EXE
fi
