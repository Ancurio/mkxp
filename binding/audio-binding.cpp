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

#define DEF_PLAY_STOP_POS(entity) \
	RB_METHOD(audio_##entity##Play) \
	{ \
		RB_UNUSED_PARAM; \
		const char *filename; \
		int volume = 100; \
		int pitch = 100; \
		double pos = 0.0; \
        rb_get_args(argc, argv, "z|iif", &filename, &volume, &pitch, &pos RB_ARG_END); \
		GUARD_EXC( shState->audio().entity##Play(filename, volume, pitch, pos); ) \
		return Qnil; \
	} \
	RB_METHOD(audio_##entity##Stop) \
	{ \
		RB_UNUSED_PARAM; \
		shState->audio().entity##Stop(); \
		return Qnil; \
	} \
	RB_METHOD(audio_##entity##Pos) \
	{ \
		RB_UNUSED_PARAM; \
		return rb_float_new(shState->audio().entity##Pos()); \
	}

#define DEF_PLAY_STOP(entity) \
	RB_METHOD(audio_##entity##Play) \
	{ \
		RB_UNUSED_PARAM; \
		const char *filename; \
		int volume = 100; \
		int pitch = 100; \
		rb_get_args(argc, argv, "z|ii", &filename, &volume, &pitch RB_ARG_END); \
		GUARD_EXC( shState->audio().entity##Play(filename, volume, pitch); ) \
		return Qnil; \
	} \
	RB_METHOD(audio_##entity##Stop) \
	{ \
		RB_UNUSED_PARAM; \
		shState->audio().entity##Stop(); \
		return Qnil; \
	}

#define DEF_FADE(entity) \
RB_METHOD(audio_##entity##Fade) \
{ \
	RB_UNUSED_PARAM; \
	int time; \
	rb_get_args(argc, argv, "i", &time RB_ARG_END); \
	shState->audio().entity##Fade(time); \
	return Qnil; \
}

#define DEF_POS(entity) \
	RB_METHOD(audio_##entity##Pos) \
	{ \
		RB_UNUSED_PARAM; \
		return rb_float_new(shState->audio().entity##Pos()); \
	}

// DEF_PLAY_STOP_POS( bgm )

RB_METHOD(audio_bgmPlay)
{
    RB_UNUSED_PARAM;
    const char *filename;
    int volume = 100;
    int pitch = 100;
    double pos = 0.0;
    int track = -127;
    rb_get_args(argc, argv, "z|iifi", &filename, &volume, &pitch, &pos, &track RB_ARG_END);
    GUARD_EXC( shState->audio().bgmPlay(filename, volume, pitch, pos, track); )
    return Qnil;
}

RB_METHOD(audio_bgmStop)
{
    RB_UNUSED_PARAM;
    int track = -127;
    rb_get_args(argc, argv, "|i", &track RB_ARG_END);
    shState->audio().bgmStop(track);
    return Qnil;
}

RB_METHOD(audio_bgmPos)
{
    RB_UNUSED_PARAM;
    int track = 0;
    rb_get_args(argc, argv, "|i", &track RB_ARG_END);
    return rb_float_new(shState->audio().bgmPos(track));
}

RB_METHOD(audio_bgmGetVolume)
{
    RB_UNUSED_PARAM;
    int track = -127;
    rb_get_args(argc, argv, "|i", &track RB_ARG_END);
    int ret = 0;
    GUARD_EXC( ret = shState->audio().bgmGetVolume(track); )
    return rb_fix_new(ret);
}

RB_METHOD(audio_bgmSetVolume)
{
    RB_UNUSED_PARAM;
    int volume;
    int track = -127;
    rb_get_args(argc, argv, "i|i", &volume, &track RB_ARG_END);
    GUARD_EXC( shState->audio().bgmSetVolume(volume, track); )
    return Qnil;
}

DEF_PLAY_STOP_POS( bgs )

DEF_PLAY_STOP( me )

//DEF_FADE( bgm )
RB_METHOD(audio_bgmFade)
{
    RB_UNUSED_PARAM;
    int time;
    int track = -127;
    rb_get_args(argc, argv, "i|i", &time, &track RB_ARG_END);
    shState->audio().bgmFade(time, track);
    return Qnil;
}

DEF_FADE( bgs )
DEF_FADE( me )

DEF_PLAY_STOP( se )

RB_METHOD(audioSetupMidi)
{
	RB_UNUSED_PARAM;

	shState->audio().setupMidi();

	return Qnil;
}

RB_METHOD(audioReset)
{
	RB_UNUSED_PARAM;

	shState->audio().reset();

	return Qnil;
}


#define BIND_PLAY_STOP(entity) \
	_rb_define_module_function(module, #entity "_play", audio_##entity##Play); \
	_rb_define_module_function(module, #entity "_stop", audio_##entity##Stop);

#define BIND_FADE(entity) \
	_rb_define_module_function(module, #entity "_fade", audio_##entity##Fade);

#define BIND_PLAY_STOP_FADE(entity) \
	BIND_PLAY_STOP(entity) \
	BIND_FADE(entity)

#define BIND_POS(entity) \
	_rb_define_module_function(module, #entity "_pos", audio_##entity##Pos);


void
audioBindingInit()
{
	VALUE module = rb_define_module("Audio");

	BIND_PLAY_STOP_FADE( bgm );
    _rb_define_module_function(module, "bgm_volume", audio_bgmGetVolume);
    _rb_define_module_function(module, "bgm_set_volume", audio_bgmSetVolume);
	BIND_PLAY_STOP_FADE( bgs );
	BIND_PLAY_STOP_FADE( me  );

	BIND_POS( bgm );
	BIND_POS( bgs );

	_rb_define_module_function(module, "setup_midi", audioSetupMidi);

	BIND_PLAY_STOP( se )

	_rb_define_module_function(module, "__reset__", audioReset);
}
