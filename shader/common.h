#ifdef GLSLES

precision mediump float;

#else

/* Desktop GLSL doesn't know about these */
#define highp
#define mediump
#define lowp

#endif
