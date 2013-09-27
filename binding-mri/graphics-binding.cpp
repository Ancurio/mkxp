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
#include "globalstate.h"
#include "binding-util.h"
#include "exception.h"

RB_METHOD(graphicsUpdate)
{
	RB_UNUSED_PARAM;

	gState->graphics().update();

	return Qnil;
}

RB_METHOD(graphicsFreeze)
{
	RB_UNUSED_PARAM;

	gState->graphics().freeze();

	return Qnil;
}

RB_METHOD(graphicsTransition)
{
	RB_UNUSED_PARAM;

	int duration = 8;
	const char *filename = 0;
	int vague = 40;

	rb_get_args(argc, argv, "|izi", &duration, &filename, &vague, RB_ARG_END);

	GUARD_EXC( gState->graphics().transition(duration, filename, vague); )

	return Qnil;
}

RB_METHOD(graphicsFrameReset)
{
	RB_UNUSED_PARAM;

	gState->graphics().frameReset();

	return Qnil;
}

#define DEF_GRA_PROP_I(PropName) \
	RB_METHOD(graphics##Get##PropName) \
	{ \
		RB_UNUSED_PARAM; \
		return rb_fix_new(gState->graphics().get##PropName()); \
	} \
	RB_METHOD(graphics##Set##PropName) \
	{ \
		RB_UNUSED_PARAM; \
		int value; \
		rb_get_args(argc, argv, "i", &value, RB_ARG_END); \
		gState->graphics().set##PropName(value); \
		return rb_fix_new(value); \
	}

#define DEF_GRA_PROP_B(PropName) \
	RB_METHOD(graphics##Get##PropName) \
	{ \
		RB_UNUSED_PARAM; \
		return rb_bool_new(gState->graphics().get##PropName()); \
	} \
	RB_METHOD(graphics##Set##PropName) \
	{ \
		RB_UNUSED_PARAM; \
		bool value; \
		rb_get_args(argc, argv, "b", &value, RB_ARG_END); \
		gState->graphics().set##PropName(value); \
		return rb_bool_new(value); \
	}

DEF_GRA_PROP_I(FrameRate)
DEF_GRA_PROP_I(FrameCount)

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

	INIT_GRA_PROP_BIND( FrameRate,  "frame_rate"  );
	INIT_GRA_PROP_BIND( FrameCount, "frame_count" );

	INIT_GRA_PROP_BIND( Fullscreen, "fullscreen"  );
	INIT_GRA_PROP_BIND( ShowCursor, "show_cursor" );
}
