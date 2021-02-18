DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

MKXPZ_PREFIX=$(shell gcc -dumpmachine | sed -r "s/(.+)-.+-.+/\1/")
export LDFLAGS="-L$DIR/build-${MKXPZ_PREFIX}/lib -L$DIR/build-${MKXPZ_PREFIX}/bin"
export PATH="$DIR/build-${MKXPZ_PREFIX}/bin:$PATH"
MKXPZ_OLD_PC=$(pkg-config --variable pc_path pkg-config)

# Try to load the stuff we built first
export PKG_CONFIG_LIBDIR="$DIR/build-${MKXPZ_PREFIX}/lib/pkgconfig:${MKXPZ_OLD_PC}"
export MKXPZ_PREFIX="$DIR/build-${MKXPZ_PREFIX}"