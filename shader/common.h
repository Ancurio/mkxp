#ifdef GLSLES

#ifdef FRAGMENT_SHADER
/* Only the fragment shader has no default float precision */
precision mediump float;
#endif

#else

/* Desktop GLSL doesn't know about these */
#define highp
#define mediump
#define lowp

#endif
