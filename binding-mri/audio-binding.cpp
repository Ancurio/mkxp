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
#include "globalstate.h"
#include "binding-util.h"
#include "exception.h"

#define DEF_PLAY_STOP(entity) \
	RB_METHOD(audio_##entity##Play) \
	{ \
		RB_UNUSED_PARAM; \
		const char *filename; \
		int volume = 100; \
		int pitch = 100; \
		rb_get_args(argc, argv, "z|ii", &filename, &volume, &pitch); \
		GUARD_EXC( gState->audio().entity##Play(filename, volume, pitch); ) \
		return Qnil; \
	} \
	RB_METHOD(audio_##entity##Stop) \
	{ \
		RB_UNUSED_PARAM; \
		gState->audio().entity##Stop(); \
		return Qnil; \
	}

#define DEF_FADE(entity) \
RB_METHOD(audio_##entity##Fade) \
{ \
	RB_UNUSED_PARAM; \
	int time; \
	rb_get_args(argc, argv, "i", &time); \
	gState->audio().bgmFade(time); \
	return Qnil; \
}

#define DEF_PLAY_STOP_FADE(entity) \
	DEF_PLAY_STOP(entity) \
	DEF_FADE(entity)

DEF_PLAY_STOP_FADE( bgm )
DEF_PLAY_STOP_FADE( bgs )
DEF_PLAY_STOP_FADE( me  )

DEF_PLAY_STOP( se  )


#define BIND_PLAY_STOP(entity) \
	_rb_define_module_function(module, #entity "_play", audio_##entity##Play); \
	_rb_define_module_function(module, #entity "_stop", audio_##entity##Stop);

#define BIND_FADE(entity) \
	_rb_define_module_function(module, #entity "_fade", audio_##entity##Fade);

#define BIND_PLAY_STOP_FADE(entity) \
	BIND_PLAY_STOP(entity) \
	BIND_FADE(entity)


void
audioBindingInit()
{
	VALUE module = rb_define_module("Audio");

	BIND_PLAY_STOP_FADE( bgm )
	BIND_PLAY_STOP_FADE( bgs )
	BIND_PLAY_STOP_FADE( me  )

	BIND_PLAY_STOP( se )
}
