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
	: baseVolume(1.0),
	  fadeVolume(1.0),
	  extVolume(1.0),
	  extPaused(false),
	  noResumeStop(false),
	  stream(loopMode, threadId)
{
	current.volume = 1.0;
	current.pitch = 1.0;

	fade.active = false;
	fade.thread = 0;
	fade.threadName = std::string("audio_fade (") + threadId + ")";

	streamMut = SDL_CreateMutex();
}

AudioStream::~AudioStream()
{
	if (fade.thread)
	{
		fade.reqTerm = true;
		SDL_WaitThread(fade.thread, 0);
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
	finiFadeInt();

	lockStream();

	float _volume = clamp<int>(volume, 0, 100) / 100.f;
	float _pitch  = clamp<int>(pitch, 50, 150) / 100.f;

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
		setBaseVolume(_volume);
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

	setBaseVolume(_volume);
	stream.setPitch(_pitch);

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
	finiFadeInt();

	lockStream();

	noResumeStop = true;

	stream.stop();

	unlockStream();
}

void AudioStream::fadeOut(int duration)
{
	lockStream();

	ALStream::State sState = stream.queryState();

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
		fade.reqFini = true;
		SDL_WaitThread(fade.thread, 0);
		fade.thread = 0;
	}

	fade.active = true;
	fade.msStep = (1.0) / duration;
	fade.reqFini = false;
	fade.reqTerm = false;
	fade.startTicks = SDL_GetTicks();

	fade.thread = SDL_CreateThread(fadeThreadFun, fade.threadName.c_str(), this);

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

void AudioStream::setFadeVolume(float value)
{
	fadeVolume = value;
	updateVolume();
}

void AudioStream::setExtVolume1(float value)
{
	extVolume = value;
	updateVolume();
}

float AudioStream::playingOffset()
{
	return stream.queryOffset();
}

void AudioStream::finiFadeInt()
{
	if (!fade.thread)
		return;

	fade.reqFini = true;
	SDL_WaitThread(fade.thread, 0);
	fade.thread = 0;
}

void AudioStream::updateVolume()
{
	stream.setVolume(baseVolume * fadeVolume * extVolume);
}

void AudioStream::setBaseVolume(float value)
{
	baseVolume = value;
	updateVolume();
}

void AudioStream::fadeThread()
{
	while (true)
	{
		/* Just immediately terminate on request */
		if (fade.reqTerm)
			break;

		lockStream();

		uint32_t curDur = SDL_GetTicks() - fade.startTicks;
		float resVol = 1.0 - (curDur*fade.msStep);

		ALStream::State state = stream.queryState();

		if (state != ALStream::Playing
		||  resVol < 0
		||  fade.reqFini)
		{
			if (state != ALStream::Paused)
				stream.stop();

			setFadeVolume(1.0);
			unlockStream();

			break;
		}

		setFadeVolume(resVol);

		unlockStream();

		SDL_Delay(AUDIO_SLEEP);
	}

	fade.active = false;
}

int AudioStream::fadeThreadFun(void *self)
{
	static_cast<AudioStream*>(self)->fadeThread();

	return 0;
}
