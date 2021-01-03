MKXPZ_HOST=$(gcc -dumpmachine)
MKXPZ_PREFIX=$(ruby -e "printf ('${MKXPZ_HOST}'[/i686/].nil?) ? 'mingw64' : 'mingw'")
export LDFLAGS="-L$PWD/build-${MKXPZ_PREFIX}/lib -L$PWD/build-${MKXPZ_PREFIX}/bin"
export PATH="$PWD/build-${MKXPZ_PREFIX}/bin:$PATH"
MKXPZ_OLD_PC=$(pkg-config --variable pc_path pkg-config)

# Try to load the stuff we built first
export PKG_CONFIG_PATH="$PWD/build-${MKXPZ_PREFIX}/lib/pkgconfig"