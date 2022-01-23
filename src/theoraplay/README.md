# TheoraPlay

A small C library to make Ogg Theora decoding easier.

A tiny example to pull data out of an .ogv file is about 
[50 lines of C code](https://github.com/icculus/theoraplay/blob/main/test/testtheoraplay.c),
and a complete SDL-based media player is about
[300 lines of code](https://github.com/icculus/theoraplay/blob/main/test/simplesdl.c).

TheoraPlay is optimized for multicore CPUs, and is designed to be 
programmer-friendly. You will need libogg, libvorbis, and libtheora,
of course, but then you just drop a .c file and two headers into your
project and you're ready to hook up video decoding, without worrying
about Ogg pages, Vorbis blocks, or Theora decoder state.

