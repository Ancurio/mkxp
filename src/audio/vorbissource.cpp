/*
** vorbissource.cpp
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

#define OV_EXCLUDE_STATIC_CALLBACKS
#include <vorbis/vorbisfile.h>
#include <vector>
#include <algorithm>

static size_t vfRead(void *ptr, size_t size, size_t nmemb, void *ops)
{
	return SDL_RWread(static_cast<SDL_RWops*>(ops), ptr, size, nmemb);
}

static int vfSeek(void *ops, ogg_int64_t offset, int whence)
{
	return SDL_RWseek(static_cast<SDL_RWops*>(ops), offset, whence);
}

static long vfTell(void *ops)
{
	return SDL_RWtell(static_cast<SDL_RWops*>(ops));
}

static ov_callbacks OvCallbacks =
{
    vfRead,
    vfSeek,
    0,
    vfTell
};


struct VorbisSource : ALDataSource
{
	SDL_RWops &src;

	OggVorbis_File vf;

	uint32_t currentFrame;

	struct
	{
		uint32_t start;
		uint32_t length;
		uint32_t end;
		bool valid;
		bool requested;
	} loop;

	struct
	{
		int channels;
		int rate;
		int frameSize;
		ALenum alFormat;
	} info;

	std::vector<int16_t> sampleBuf;

	VorbisSource(SDL_RWops &ops,
	             bool looped)
	    : src(ops),
	      currentFrame(0)
	{
		int error = ov_open_callbacks(&src, &vf, 0, 0, OvCallbacks);

		if (error)
		{
			SDL_RWclose(&src);
			throw Exception(Exception::MKXPError,
			                "Vorbisfile: Cannot read ogg file");
		}

		/* Extract bitstream info */
		info.channels = vf.vi->channels;
		info.rate = vf.vi->rate;

		if (info.channels > 2)
		{
			ov_clear(&vf);
			SDL_RWclose(&src);
			throw Exception(Exception::MKXPError,
			                "Cannot handle audio with more than 2 channels");
		}

		info.alFormat = chooseALFormat(sizeof(int16_t), info.channels);
		info.frameSize = sizeof(int16_t) * info.channels;

		sampleBuf.resize(STREAM_BUF_SIZE);

		loop.requested = looped;
		loop.valid = false;
		loop.start = loop.length = 0;

		if (!loop.requested)
			return;

		/* Try to extract loop info */
		for (int i = 0; i < vf.vc->comments; ++i)
		{
			char *comment = vf.vc->user_comments[i];
			char *sep = strstr(comment, "=");

			/* No '=' found */
			if (!sep)
				continue;

			/* Empty value */
			if (!*(sep+1))
				continue;

			*sep = '\0';

			if (!strcmp(comment, "LOOPSTART"))
				loop.start = strtol(sep+1, 0, 10);

			if (!strcmp(comment, "LOOPLENGTH"))
				loop.length = strtol(sep+1, 0, 10);

			*sep = '=';
		}

		loop.end = loop.start + loop.length;
		loop.valid = (loop.start && loop.length);
	}

	~VorbisSource()
	{
		ov_clear(&vf);
		SDL_RWclose(&src);
	}

	int sampleRate()
	{
		return info.rate;
	}

	void seekToOffset(float seconds)
	{
		if (seconds <= 0)
		{
			ov_raw_seek(&vf, 0);
			currentFrame = 0;
		}

		currentFrame = seconds * info.rate;

		if (loop.valid && currentFrame > loop.end)
			currentFrame = loop.start;

		/* If seeking fails, just seek back to start */
		if (ov_pcm_seek(&vf, currentFrame) != 0)
			ov_raw_seek(&vf, 0);
	}

	Status fillBuffer(AL::Buffer::ID alBuffer)
	{
		void *bufPtr = sampleBuf.data();
		int availBuf = sampleBuf.size();
		int bufUsed  = 0;

		int canRead = availBuf;

		Status retStatus = ALDataSource::NoError;

		bool readAgain = false;

		if (loop.valid)
		{
			int tilLoopEnd = loop.end * info.frameSize;

			canRead = std::min(availBuf, tilLoopEnd);
		}

		while (canRead > 16)
		{
			long res = ov_read(&vf, static_cast<char*>(bufPtr),
			                   canRead, 0, sizeof(int16_t), 1, 0);

			if (res < 0)
			{
				/* Read error */
				retStatus = ALDataSource::Error;

				break;
			}

			if (res == 0)
			{
				/* EOF */
				if (loop.requested)
				{
					retStatus = ALDataSource::WrapAround;
					seekToOffset(0);
				}
				else
				{
					retStatus = ALDataSource::EndOfStream;
				}

				/* If we sought right to the end of the file,
				 * we might be EOF without actually having read
				 * any data at all yet (which mustn't happen),
				 * so we try to continue reading some data. */
				if (bufUsed > 0)
					break;

				if (readAgain)
				{
					/* We're still not getting data though.
					 * Just error out to prevent an endless loop */
					retStatus = ALDataSource::Error;
					break;
				}

				readAgain = true;
			}

			bufUsed += (res / sizeof(int16_t));
			bufPtr = &sampleBuf[bufUsed];
			currentFrame += (res / info.frameSize);

			if (loop.valid && currentFrame >= loop.end)
			{
				/* Determine how many frames we're
				 * over the loop end */
				int discardFrames = currentFrame - loop.end;
				bufUsed -= discardFrames * info.channels;

				retStatus = ALDataSource::WrapAround;

				/* Seek to loop start */
				currentFrame = loop.start;
				if (ov_pcm_seek(&vf, currentFrame) != 0)
					retStatus = ALDataSource::Error;

				break;
			}

			canRead -= res;
		}

		if (retStatus != ALDataSource::Error)
			AL::Buffer::uploadData(alBuffer, info.alFormat, sampleBuf.data(),
			                       bufUsed*sizeof(int16_t), info.rate);

		return retStatus;
	}

	uint32_t loopStartFrames()
	{
		if (loop.valid)
			return loop.start;
		else
			return 0;
	}

	bool setPitch(float)
	{
		return false;
	}
};

ALDataSource *createVorbisSource(SDL_RWops &ops,
                                 bool looped)
{
	return new VorbisSource(ops, looped);
}
