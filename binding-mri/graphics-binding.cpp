/*
** graphics-binding.cpp
**
** This file is part of mkxp.
**
** Copyright (C) 2013 Jonas Kulla <Nyocurio@gmail.com>
**
** mkxp is free software: you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation, either version 2 of the License, or
** (at your option) any later version.
**
** mkxp is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with mkxp.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "graphics.h"
#include "sharedstate.h"
#include "binding-util.h"
#include "binding-types.h"
#include "exception.h"

#include <ruby/thread.h>

static void *graphicsUpdate_internal(void*)
{
	shState->graphics().update();
	return NULL;
}

RB_METHOD(graphicsUpdate)
{
	RB_UNUSED_PARAM;

	rb_thread_call_without_gvl(&graphicsUpdate_internal, NULL, NULL, NULL);

	return Qnil;
}

RB_METHOD(graphicsFreeze)
{
	RB_UNUSED_PARAM;

	shState->graphics().freeze();

	return Qnil;
}

/** Argument structure used to pass arguments to various blocking calls,
 * meanwhile releasing the ruby interpreter lock. */
struct transition_args {
	int duration;
	const char *filename;
	int vague;
};

static void *graphicsTransition_internal(void* args_in)
{
	struct transition_args *args = static_cast<transition_args*>(args_in);
	GUARD_EXC( shState->graphics().transition(args->duration, args->filename, args->vague); )
	return NULL;
}

RB_METHOD(graphicsTransition)
{
	RB_UNUSED_PARAM;

	struct transition_args args;
	args.duration = 8;
	args.filename = "";
	args.vague = 40;

	rb_get_args(argc, argv, "|izi", &args.duration, &args.filename, &args.vague RB_ARG_END);

	rb_thread_call_without_gvl(&graphicsTransition_internal, &args, NULL, NULL);

	return Qnil;
}

RB_METHOD(graphicsFrameReset)
{
	RB_UNUSED_PARAM;

	shState->graphics().frameReset();

	return Qnil;
}

#define DEF_GRA_PROP_I(PropName) \
	RB_METHOD(graphics##Get##PropName) \
	{ \
		RB_UNUSED_PARAM; \
		return rb_fix_new(shState->graphics().get##PropName()); \
	} \
	RB_METHOD(graphics##Set##PropName) \
	{ \
		RB_UNUSED_PARAM; \
		int value; \
		rb_get_args(argc, argv, "i", &value RB_ARG_END); \
		shState->graphics().set##PropName(value); \
		return rb_fix_new(value); \
	}

#define DEF_GRA_PROP_B(PropName) \
	RB_METHOD(graphics##Get##PropName) \
	{ \
		RB_UNUSED_PARAM; \
		return rb_bool_new(shState->graphics().get##PropName()); \
	} \
	RB_METHOD(graphics##Set##PropName) \
	{ \
		RB_UNUSED_PARAM; \
		bool value; \
		rb_get_args(argc, argv, "b", &value RB_ARG_END); \
		shState->graphics().set##PropName(value); \
		return rb_bool_new(value); \
	}

RB_METHOD(graphicsWidth)
{
	RB_UNUSED_PARAM;

	return rb_fix_new(shState->graphics().width());
}

RB_METHOD(graphicsHeight)
{
	RB_UNUSED_PARAM;

	return rb_fix_new(shState->graphics().height());
}

static void *graphicsWait_internal(void* args_in)
{
	struct transition_args *args = static_cast<transition_args*>(args_in);
	shState->graphics().wait(args->duration);
	return NULL;
}

RB_METHOD(graphicsWait)
{
	RB_UNUSED_PARAM;

	struct transition_args args;
	rb_get_args(argc, argv, "i", &args.duration RB_ARG_END);
	rb_thread_call_without_gvl(&graphicsWait_internal, &args, NULL, NULL);

	return Qnil;
}

static void *graphicsFadeout_internal(void* args_in)
{
	struct transition_args *args = static_cast<transition_args*>(args_in);
	shState->graphics().fadeout(args->duration);
	return NULL;
}

RB_METHOD(graphicsFadeout)
{
	RB_UNUSED_PARAM;

	struct transition_args args;
	rb_get_args(argc, argv, "i", &args.duration RB_ARG_END);
	rb_thread_call_without_gvl(&graphicsFadeout_internal, &args, NULL, NULL);

	return Qnil;
}

static void *graphicsFadein_internal(void* args_in)
{
	struct transition_args *args = static_cast<transition_args*>(args_in);
	shState->graphics().fadein(args->duration);
	return NULL;
}

RB_METHOD(graphicsFadein)
{
	RB_UNUSED_PARAM;

	struct transition_args args;
	rb_get_args(argc, argv, "i", &args.duration RB_ARG_END);
	rb_thread_call_without_gvl(&graphicsFadein_internal, &args, NULL, NULL);

	return Qnil;
}

void bitmapInitProps(Bitmap *b, VALUE self);

RB_METHOD(graphicsSnapToBitmap)
{
	RB_UNUSED_PARAM;

	Bitmap *result = 0;
	GUARD_EXC( result = shState->graphics().snapToBitmap(); );

	VALUE obj = wrapObject(result, BitmapType);
	bitmapInitProps(result, obj);

	return obj;
}

RB_METHOD(graphicsResizeScreen)
{
	RB_UNUSED_PARAM;

	int width, height;
	rb_get_args(argc, argv, "ii", &width, &height RB_ARG_END);

	shState->graphics().resizeScreen(width, height);

	return Qnil;
}

RB_METHOD(graphicsReset)
{
	RB_UNUSED_PARAM;

	shState->graphics().reset();

	return Qnil;
}

RB_METHOD(graphicsPlayMovie)
{
	RB_UNUSED_PARAM;

	const char *filename;
	rb_get_args(argc, argv, "z", &filename RB_ARG_END);

	shState->graphics().playMovie(filename);

	return Qnil;
}

DEF_GRA_PROP_I(FrameRate)
DEF_GRA_PROP_I(FrameCount)
DEF_GRA_PROP_I(Brightness)

DEF_GRA_PROP_B(Fullscreen)
DEF_GRA_PROP_B(ShowCursor)

#define INIT_GRA_PROP_BIND(PropName, prop_name_s) \
{ \
	_rb_define_module_function(module, prop_name_s, graphics##Get##PropName); \
	_rb_define_module_function(module, prop_name_s "=", graphics##Set##PropName); \
}

void graphicsBindingInit()
{
	VALUE module = rb_define_module("Graphics");

	_rb_define_module_function(module, "update", graphicsUpdate);
	_rb_define_module_function(module, "freeze", graphicsFreeze);
	_rb_define_module_function(module, "transition", graphicsTransition);
	_rb_define_module_function(module, "frame_reset", graphicsFrameReset);

	_rb_define_module_function(module, "__reset__", graphicsReset);

	INIT_GRA_PROP_BIND( FrameRate,  "frame_rate"  );
	INIT_GRA_PROP_BIND( FrameCount, "frame_count" );

	if (rgssVer >= 2)
	{
	_rb_define_module_function(module, "width", graphicsWidth);
	_rb_define_module_function(module, "height", graphicsHeight);
	_rb_define_module_function(module, "wait", graphicsWait);
	_rb_define_module_function(module, "fadeout", graphicsFadeout);
	_rb_define_module_function(module, "fadein", graphicsFadein);
	_rb_define_module_function(module, "snap_to_bitmap", graphicsSnapToBitmap);
	_rb_define_module_function(module, "resize_screen", graphicsResizeScreen);

	INIT_GRA_PROP_BIND( Brightness, "brightness" );
	}

	if (rgssVer >= 3)
	{
	_rb_define_module_function(module, "play_movie", graphicsPlayMovie);
	}

	INIT_GRA_PROP_BIND( Fullscreen, "fullscreen"  );
	INIT_GRA_PROP_BIND( ShowCursor, "show_cursor" );
}
