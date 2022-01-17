Here lives Google's ANGLE, which is pretty necessary right now.

On top of Apple deprecating OpenGL, MKXP has crashing issues from
the OpenGL -> Metal translation implemented in Apple Silicon macs.

This particular build of ANGLE is made using [this commit](https://github.com/google/angle/tree/0aae0d7ad535aedba34daea325269e419baead68), built with Metal support enabled. Both arm64 and x64 libraries were built using the `target_cpu` arg, then joined using the `lipo` tool.