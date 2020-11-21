/*
** sdlsoundsource.cpp
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

#include "aldatasource.h"
#include "exception.h"

#include <SDL_sound.h>

struct SDLSoundSource : ALDataSource
{
	Sound_Sample *sample;
	SDL_RWops &srcOps;
	uint8_t sampleSize;
	bool looped;

	ALenum alFormat;
	ALsizei alFreq;

	SDLSoundSource(SDL_RWops &ops,
	               const char *extension,
	               uint32_t maxBufSize,
	               bool looped)
	    : srcOps(ops),
	      looped(looped)
	{
		sample = Sound_NewSample(&srcOps, extension, 0, maxBufSize);

		if (!sample)
		{
			SDL_RWclose(&ops);
			throw Exception(Exception::SDLError, "SDL_sound: %s", Sound_GetError());
		}

		sampleSize = formatSampleSize(sample->actual.format);

		alFormat = chooseALFormat(sampleSize, sample->actual.channels);
		alFreq = sample->actual.rate;
	}

	~SDLSoundSource()
	{
		/* This also closes 'srcOps' */
		Sound_FreeSample(sample);
	}

	Status fillBuffer(AL::Buffer::ID alBuffer)
	{
		uint32_t decoded = Sound_Decode(sample);

		if (sample->flags & SOUND_SAMPLEFLAG_EAGAIN)
		{
			/* Try to decode one more time on EAGAIN */
			decoded = Sound_Decode(sample);

			/* Give up */
			if (sample->flags & SOUND_SAMPLEFLAG_EAGAIN)
				return ALDataSource::Error;
		}

		if (sample->flags & SOUND_SAMPLEFLAG_ERROR)
			return ALDataSource::Error;

		AL::Buffer::uploadData(alBuffer, alFormat, sample->buffer, decoded, alFreq);

		if (sample->flags & SOUND_SAMPLEFLAG_EOF)
		{
			if (looped)
			{
				Sound_Rewind(sample);
				return ALDataSource::WrapAround;
			}
			else
			{
				return ALDataSource::EndOfStream;
			}
		}

		return ALDataSource::NoError;
	}

	int sampleRate()
	{
		return sample->actual.rate;
	}

	void seekToOffset(float seconds)
	{
		if (seconds <= 0)
			Sound_Rewind(sample);
		else
			Sound_Seek(sample, static_cast<uint32_t>(seconds * 1000));
	}

	uint32_t loopStartFrames()
	{
		/* Loops from the beginning of the file */
		return 0;
	}

	bool setPitch(float)
	{
		return false;
	}
};

ALDataSource *createSDLSource(SDL_RWops &ops,
                              const char *extension,
			                  uint32_t maxBufSize,
			                  bool looped)
{
	return new SDLSoundSource(ops, extension, maxBufSize, looped);
}
