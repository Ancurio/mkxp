#include "fluid-fun.h"

#include <string.h>
#include <SDL_loadso.h>
#include <SDL_platform.h>

#include "debugwriter.h"

#if __LINUX__ || __ANDROID__
#define FLUID_LIB "libfluidsynth.so.3"
#elif MKXPZ_BUILD_XCODE
#define FLUID_LIB "@rpath/libfluidsynth.dylib"
#elif __MACOSX__
#define FLUID_LIB "libfluidsynth.3.dylib"
#elif __WINDOWS__
#define FLUID_LIB "fluidsynth.dll"
#else
#error "platform not recognized"
#endif

struct FluidFunctions fluid;
#ifndef SHARED_FLUID
static void *so;
#endif

void initFluidFunctions()
{
#ifdef SHARED_FLUID

#define FLUID_FUN(name, type) \
	fluid.name = fluid_##name;

#define FLUID_FUN2(name, type, real_name) \
	fluid.name = real_name;

#else
	so = SDL_LoadObject(FLUID_LIB);

	if (!so)
		goto fail;

#define FLUID_FUN(name, type) \
	fluid.name = (type) SDL_LoadFunction(so, "fluid_" #name); \
	if (!fluid.name) \
		goto fail;

#define FLUID_FUN2(name, type, real_name) \
	fluid.name = (type) SDL_LoadFunction(so, #real_name); \
	if (!fluid.name) \
		goto fail;
#endif

FLUID_FUNCS
FLUID_FUNCS2

	return;

#ifndef SHARED_FLUID
fail:
	Debug() << "Failed to load " FLUID_LIB ". Midi playback is disabled.";

	memset(&fluid, 0, sizeof(fluid));
	SDL_UnloadObject(so);
	so = 0;
#endif
}
