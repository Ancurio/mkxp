DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

MKXPZ_HOST=$(gcc -dumpmachine)
MKXPZ_PREFIX=$(ruby -e "printf ('${MKXPZ_HOST}'[/i686/].nil?) ? 'mingw64' : 'mingw'")
export LDFLAGS="-L$DIR/build-${MKXPZ_PREFIX}/lib -L$DIR/build-${MKXPZ_PREFIX}/bin"
export PATH="$DIR/build-${MKXPZ_PREFIX}/bin:$PATH"
MKXPZ_OLD_PC=$(pkg-config --variable pc_path pkg-config)

# Try to load the stuff we built first
export PKG_CONFIG_PATH="$DIR/build-${MKXPZ_PREFIX}/lib/pkgconfig"