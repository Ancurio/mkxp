Here lives Google's ANGLE, which is pretty necessary right now.

On top of Apple deprecating OpenGL, MKXP has crashing issues from
the OpenGL -> Metal translation implemented in Apple Silicon macs.

It also enables things like the Steam Overlay and the use of the Metal
Performance HUD on macOS 13+.

This particular build of ANGLE is made using commit `91bfd02e7089b`, built with Metal support enabled. Both arm64 and x64 libraries were built using the `target_cpu` arg, then joined using the `lipo` tool.