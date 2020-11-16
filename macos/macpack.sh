#!/bin/bash

EXE=${MESON_INSTALL_PREFIX}/Contents/MacOS/$2

if [ -n "$1" ]; then
  echo "Setting up steam_api manually..."
  mkdir -p "${MESON_INSTALL_PREFIX}/Contents/Frameworks"
  cp "$1/libsteam_api.dylib" "${MESON_INSTALL_PREFIX}/Contents/Frameworks"
  install_name_tool -change "@loader_path/libsteam_api.dylib" "@executable_path/../Frameworks/libsteam_api.dylib" $EXE
  install_name_tool -add_rpath "@executable_path/../Frameworks" ${EXE}_rt
  macpack -d "../Frameworks" ${EXE}_rt
else
  install_name_tool -add_rpath "@executable_path/../Frameworks" ${EXE}
  macpack -d "../Frameworks" ${EXE}
fi