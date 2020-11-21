/*
** audiostream.cpp
**
** This file is part of mkxp.
**
** Copyright (C) 2014 Jonas Kulla <Nyocurio@gmail.com>
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

#include "audiostream.h"

#include "util.h"
#include "exception.h"

#include <SDL_mutex.h>
#include <SDL_thread.h>
#include <SDL_timer.h>

AudioStream::AudioStream(ALStream::LoopMode loopMode,
                         const std::string &threadId)
	: extPaused(false),
	  noResumeStop(false),
	  stream(loopMode, threadId)
{
	current.volume = 1.0f;
	current.pitch = 1.0f;

	for (size_t i = 0; i < VolumeTypeCount; ++i)
		volumes[i] = 1.0f;

	fade.thread = 0;
	fade.threadName = std::string("audio_fadeout (") + threadId + ")";

	fadeIn.thread = 0;
	fadeIn.threadName = std::string("audio_fadein (") + threadId + ")";

	streamMut = SDL_CreateMutex();
}

AudioStream::~AudioStream()
{
	if (fade.thread)
	{
		fade.reqTerm.set();
		SDL_WaitThread(fade.thread, 0);
	}

	if (fadeIn.thread)
	{
		fadeIn.rqTerm.set();
		SDL_WaitThread(fadeIn.thread, 0);
	}

	lockStream();

	stream.stop();
	stream.close();

	unlockStream();

	SDL_DestroyMutex(streamMut);
}

void AudioStream::play(const std::string &filename,
                       int volume,
                       int pitch,
                       float offset)
{
	finiFadeOutInt();

	lockStream();

	float _volume = clamp<int>(volume, 0, 100) / 100.0f;
	float _pitch  = clamp<int>(pitch, 50, 150) / 100.0f;

	ALStream::State sState = stream.queryState();

	/* If all parameters match the current ones and we're
	 * still playing, there's nothing to do */
	if (filename == current.filename
	&&  _volume  == current.volume
	&&  _pitch   == current.pitch
	&&  (sState == ALStream::Playing || sState == ALStream::Paused))
	{
		unlockStream();
		return;
	}

	/* If all parameters except volume match the current ones,
	 * we update the volume and continue streaming */
	if (filename == current.filename
	&&  _pitch   == current.pitch
	&&  (sState == ALStream::Playing || sState == ALStream::Paused))
	{
		setVolume(Base, _volume);
		current.volume = _volume;
		unlockStream();
		return;
	}

	/* Requested audio file is different from current one */
	bool diffFile = (filename != current.filename);

	switch (sState)
	{
	case ALStream::Paused :
	case ALStream::Playing :
		stream.stop();
	case ALStream::Stopped :
		if (diffFile)
			stream.close();
	case ALStream::Closed :
		if (diffFile)
		{
			try
			{
				/* This will throw on errors while
				 * opening the data source */
				stream.open(filename);
			}
			catch (const Exception &e)
			{
				unlockStream();
				throw e;
			}
		}

		break;
	}

	setVolume(Base, _volume);
	stream.setPitch(_pitch);

	if (offset > 0)
	{
		setVolume(FadeIn, 0);
		startFadeIn();
	}

	current.filename = filename;
	current.volume = _volume;
	current.pitch = _pitch;

	if (!extPaused)
		stream.play(offset);
	else
		noResumeStop = false;

	unlockStream();
}

void AudioStream::stop()
{
	finiFadeOutInt();

	lockStream();

	noResumeStop = true;

	stream.stop();

	unlockStream();
}

void AudioStream::fadeOut(int duration)
{
	lockStream();

	ALStream::State sState = stream.queryState();
	noResumeStop = true;

	if (fade.active)
	{
		unlockStream();

		return;
	}

	if (sState == ALStream::Paused)
	{
		stream.stop();
		unlockStream();

		return;
	}

	if (sState != ALStream::Playing)
	{
		unlockStream();

		return;
	}

	if (fade.thread)
	{
		fade.reqFini.set();
		SDL_WaitThread(fade.thread, 0);
		fade.thread = 0;
	}

	fade.active.set();
	fade.msStep = 1.0f / duration;
	fade.reqFini.clear();
	fade.reqTerm.clear();
	fade.startTicks = SDL_GetTicks();

	fade.thread = createSDLThread
		<AudioStream, &AudioStream::fadeOutThread>(this, fade.threadName);

	unlockStream();
}

/* Any access to this classes 'stream' member,
 * whether state query or modification, must be
 * protected by a 'lock'/'unlock' pair */
void AudioStream::lockStream()
{
	SDL_LockMutex(streamMut);
}

void AudioStream::unlockStream()
{
	SDL_UnlockMutex(streamMut);
}

void AudioStream::setVolume(VolumeType type, float value)
{
	volumes[type] = value;
	updateVolume();
}

float AudioStream::getVolume(VolumeType type)
{
	return volumes[type];
}

float AudioStream::playingOffset()
{
	return stream.queryOffset();
}

void AudioStream::updateVolume()
{
	float vol = GLOBAL_VOLUME;

	for (size_t i = 0; i < VolumeTypeCount; ++i)
		vol *= volumes[i];

	stream.setVolume(vol);
}

void AudioStream::finiFadeOutInt()
{
	if (fade.thread)
	{
		fade.reqFini.set();
		SDL_WaitThread(fade.thread, 0);
		fade.thread = 0;
	}

	if (fadeIn.thread)
	{
		fadeIn.rqFini.set();
		SDL_WaitThread(fadeIn.thread, 0);
		fadeIn.thread = 0;
	}
}

void AudioStream::startFadeIn()
{
	/* Previous fadein should always be terminated in play() */
	assert(!fadeIn.thread);

	fadeIn.rqFini.clear();
	fadeIn.rqTerm.clear();
	fadeIn.startTicks = SDL_GetTicks();

	fadeIn.thread = createSDLThread
		<AudioStream, &AudioStream::fadeInThread>(this, fadeIn.threadName);
}

void AudioStream::fadeOutThread()
{
	while (true)
	{
		/* Just immediately terminate on request */
		if (fade.reqTerm)
			break;

		lockStream();

		uint32_t curDur = SDL_GetTicks() - fade.startTicks;
		float resVol = 1.0f - (curDur*fade.msStep);

		ALStream::State state = stream.queryState();

		if (state != ALStream::Playing
		|| resVol < 0
		|| fade.reqFini)
		{
			if (state != ALStream::Paused)
				stream.stop();

			setVolume(FadeOut, 1.0f);
			unlockStream();

			break;
		}

		setVolume(FadeOut, resVol);

		unlockStream();

		SDL_Delay(AUDIO_SLEEP);
	}

	fade.active.clear();
}

void AudioStream::fadeInThread()
{
	while (true)
	{
		if (fadeIn.rqTerm)
			break;

		lockStream();

		/* Fade in duration is always 1 second */
		uint32_t cur = SDL_GetTicks() - fadeIn.startTicks;
		float prog = cur / 1000.0f;

		ALStream::State state = stream.queryState();

		if (state != ALStream::Playing
		||  prog >= 1.0f
		||  fadeIn.rqFini)
		{
			setVolume(FadeIn, 1.0f);
			unlockStream();

			break;
		}

		/* Quadratic increase (not really the same as
		 * in RMVXA, but close enough) */
		setVolume(FadeIn, prog*prog);

		unlockStream();

		SDL_Delay(AUDIO_SLEEP);
	}
}
