/*
** audio-binding.cpp
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

#include "audio.h"
#include "sharedstate.h"
#include "binding-util.h"
#include "exception.h"

#define DEF_PLAY_STOP(entity) \
	MRB_FUNCTION(audio_##entity##Play) \
	{ \
		char *filename; \
		mrb_int volume = 100; \
		mrb_int pitch = 100; \
		mrb_get_args(mrb, "z|ii", &filename, &volume, &pitch); \
		GUARD_EXC( shState->audio().entity##Play(filename, volume, pitch); ) \
		return mrb_nil_value(); \
	} \
	MRB_FUNCTION(audio_##entity##Stop) \
	{ \
		MRB_FUN_UNUSED_PARAM; \
		shState->audio().entity##Stop(); \
		return mrb_nil_value(); \
	}

#define DEF_FADE(entity) \
MRB_FUNCTION(audio_##entity##Fade) \
{ \
	mrb_int time; \
	mrb_get_args(mrb, "i", &time); \
	shState->audio().entity##Fade(time); \
	return mrb_nil_value(); \
}

#define DEF_PLAY_STOP_FADE(entity) \
	DEF_PLAY_STOP(entity) \
	DEF_FADE(entity)

DEF_PLAY_STOP_FADE( bgm )
DEF_PLAY_STOP_FADE( bgs )
DEF_PLAY_STOP_FADE( me  )

DEF_PLAY_STOP( se  )


#define BIND_PLAY_STOP(entity) \
	mrb_define_module_function(mrb, module, #entity "_play", audio_##entity##Play, MRB_ARGS_REQ(1) | MRB_ARGS_OPT(2)); \
	mrb_define_module_function(mrb, module, #entity "_stop", audio_##entity##Stop, MRB_ARGS_NONE());

#define BIND_FADE(entity) \
	mrb_define_module_function(mrb, module, #entity "_fade", audio_##entity##Fade, MRB_ARGS_REQ(1));

#define BIND_PLAY_STOP_FADE(entity) \
	BIND_PLAY_STOP(entity) \
	BIND_FADE(entity)


void
audioBindingInit(mrb_state *mrb)
{
	RClass *module = mrb_define_module(mrb, "Audio");

	BIND_PLAY_STOP_FADE( bgm )
	BIND_PLAY_STOP_FADE( bgs )
	BIND_PLAY_STOP_FADE( me  )

	BIND_PLAY_STOP( se )
}
