#!/bin/bash
BINARY=$1
APPIMAGETOOL=$2
STEAM=$3
function get_dep()
{
echo "Copying $1..."
DEP=$(ldd $BINARY | grep $1 | sed -r 's/	\w.+ => (\/.+) .+$/\1/g')
cp "$DEP" "${MESON_INSTALL_PREFIX}/usr/lib"
}

mkdir -p ${MESON_INSTALL_PREFIX}/usr/lib


# Required by Ubuntu
get_dep openal
get_dep physfs
get_dep SDL2-2.0
get_dep SDL2_ttf
get_dep SDL2_image
get_dep SDL_sound
get_dep fluidsynth
get_dep ruby
get_dep sndio
get_dep objfw.so
get_dep objfwrt.so

# Required by Fedora & Manjaro
get_dep libXss
get_dep libjpeg
get_dep libwebp
get_dep libcrypt
get_dep libbsd
get_dep tinfo
get_dep readline

if [ -n "$STEAM" ]; then
  echo "Copying steam_api..."
  cp "$STEAM/libsteam_api.so" "${MESON_INSTALL_PREFIX}/usr/lib"
fi

cp ${MESON_INSTALL_PREFIX}/share/mkxp-z/* ${MESON_INSTALL_PREFIX}
rm -rf ${MESON_INSTALL_PREFIX}/share
$APPIMAGETOOL ${MESON_INSTALL_PREFIX}
