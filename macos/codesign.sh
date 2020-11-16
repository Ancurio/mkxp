#!/bin/bash

IDENTITY="$1"
TIMESTAMP=--timestamp
if [[ $IDENTITY == "" ]]; then
    IDENTITY=-
    TIMESTAMP=""
fi

  codesign -vfs $IDENTITY --entitlements "${MESON_SOURCE_ROOT}/macos/entitlements.plist" --deep --ignore-resources -o runtime $TIMESTAMP "${MESON_INSTALL_PREFIX}"