#!/bin/bash

IDENTITY="$1"
if [[ $IDENTITY == "" ]]; then
    IDENTITY=-
fi

  codesign -vfs $IDENTITY --entitlements "${MESON_SOURCE_ROOT}/macos/entitlements.plist" --deep -o runtime "${MESON_INSTALL_PREFIX}"