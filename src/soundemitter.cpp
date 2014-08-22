/*
** soundemitter.cpp
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

#include "soundemitter.h"

#include "sharedstate.h"
#include "filesystem.h"
#include "exception.h"
#include "util.h"

#include <SDL_sound.h>

#define SE_CACHE_MEM (10*1024*1024) // 10 MB

struct SoundBuffer
{
	/* Uniquely identifies this or equal buffer */
	std::string key;

	AL::Buffer::ID alBuffer;

	/* Link into the buffer cache priority list */
	IntruListLink<SoundBuffer> link;

	/* Buffer byte count */
	uint32_t bytes;

	/* Reference count */
	uint8_t refCount;

	SoundBuffer()
	    : link(this),
	      refCount(1)

	{
		alBuffer = AL::Buffer::gen();
	}

	static SoundBuffer *ref(SoundBuffer *buffer)
	{
		++buffer->refCount;

		return buffer;
	}

	static void deref(SoundBuffer *buffer)
	{
		if (--buffer->refCount == 0)
			delete buffer;
	}

private:
	~SoundBuffer()
	{
		AL::Buffer::del(alBuffer);
	}
};

/* Before: [a][b][c][d], After (index=1): [a][c][d][b] */
static void
arrayPushBack(size_t array[], size_t size, size_t index)
{
	size_t v = array[index];

	for (size_t t = index; t < size-1; ++t)
		array[t] = array[t+1];

	array[size-1] = v;
}

SoundEmitter::SoundEmitter()
	: bufferBytes(0)
{
	for (int i = 0; i < SE_SOURCES; ++i)
	{
		alSrcs[i] = AL::Source::gen();
		atchBufs[i] = 0;
		srcPrio[i] = i;
	}
}

SoundEmitter::~SoundEmitter()
{
	for (int i = 0; i < SE_SOURCES; ++i)
	{
		AL::Source::stop(alSrcs[i]);
		AL::Source::del(alSrcs[i]);

		if (atchBufs[i])
			SoundBuffer::deref(atchBufs[i]);
	}

	BufferHash::const_iterator iter;
	for (iter = bufferHash.cbegin(); iter != bufferHash.cend(); ++iter)
		SoundBuffer::deref(iter->second);
}

void SoundEmitter::play(const std::string &filename,
                        int volume,
					    int pitch)
{
	float _volume = clamp<int>(volume, 0, 100) / 100.f;
	float _pitch  = clamp<int>(pitch, 50, 150) / 100.f;

	SoundBuffer *buffer = allocateBuffer(filename);

	/* Try to find first free source */
	size_t i;
	for (i = 0; i < SE_SOURCES; ++i)
		if (AL::Source::getState(alSrcs[srcPrio[i]]) != AL_PLAYING)
			break;

	/* If we didn't find any, overtake the one with lowest priority */
	if (i == SE_SOURCES)
		i = 0;

	/* Push the used source to the back of the priority list */
	size_t srcIndex = srcPrio[i];
	arrayPushBack(srcPrio, SE_SOURCES, i);

	AL::Source::ID src = alSrcs[srcIndex];
	AL::Source::stop(src);
	AL::Source::detachBuffer(src);

	SoundBuffer *old = atchBufs[srcIndex];

	if (old)
		SoundBuffer::deref(old);

	atchBufs[srcIndex] = SoundBuffer::ref(buffer);

	AL::Source::attachBuffer(src, buffer->alBuffer);

	AL::Source::setVolume(src, _volume);
	AL::Source::setPitch(src, _pitch);

	AL::Source::play(src);
}

void SoundEmitter::stop()
{
	for (int i = 0; i < SE_SOURCES; i++)
		AL::Source::stop(alSrcs[i]);
}

SoundBuffer *SoundEmitter::allocateBuffer(const std::string &filename)
{
	SoundBuffer *buffer = bufferHash.value(filename, 0);

	if (buffer)
	{
		/* Buffer still in cashe.
		 * Move to front of priority list */
		buffers.remove(buffer->link);
		buffers.append(buffer->link);

		return buffer;
	}
	else
	{
		/* Buffer not in cashe, needs to be loaded */
		SDL_RWops dataSource;
		const char *extension;

		shState->fileSystem().openRead(dataSource, filename.c_str(),
									   FileSystem::Audio, false, &extension);

		Sound_Sample *sampleHandle = Sound_NewSample(&dataSource, extension, 0, STREAM_BUF_SIZE);

		if (!sampleHandle)
		{
			SDL_RWclose(&dataSource);
			throw Exception(Exception::SDLError, "%s.%s: %s",
			                filename.c_str(), extension, Sound_GetError());
		}

		uint32_t decBytes = Sound_DecodeAll(sampleHandle);
		uint8_t sampleSize = formatSampleSize(sampleHandle->actual.format);
		uint32_t sampleCount = decBytes / sampleSize;

		buffer = new SoundBuffer;
		buffer->key = filename;
		buffer->bytes = sampleSize * sampleCount;

		ALenum alFormat = chooseALFormat(sampleSize, sampleHandle->actual.channels);

		AL::Buffer::uploadData(buffer->alBuffer, alFormat, sampleHandle->buffer,
							   buffer->bytes, sampleHandle->actual.rate);

		Sound_FreeSample(sampleHandle);

		uint32_t wouldBeBytes = bufferBytes + buffer->bytes;

		/* If memory limit is reached, delete lowest priority buffer
		 * until there is room or no buffers left */
		while (wouldBeBytes > SE_CACHE_MEM && !buffers.isEmpty())
		{
			SoundBuffer *last = buffers.tail();
			bufferHash.erase(last->key);
			buffers.remove(last->link);

			wouldBeBytes -= last->bytes;

			SoundBuffer::deref(last);
		}

		bufferHash.insert(filename, buffer);
		buffers.prepend(buffer->link);

		bufferBytes = wouldBeBytes;

		return buffer;
	}
}
