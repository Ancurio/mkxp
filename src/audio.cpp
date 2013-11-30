/*
** audio.cpp
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
#include "util.h"
#include "intrulist.h"
#include "filesystem.h"
#include "exception.h"
#include "al-util.h"

#include <QByteArray>
#include <QHash>

#include <vector>
#include <string>

#include <SDL_audio.h>
#include <SDL_thread.h>
#include <SDL_endian.h>
#include <SDL_timer.h>
#include <SDL_sound.h>

#include <alc.h>

#include <QDebug>

#define AUDIO_SLEEP 10
#define SE_SOURCES 6
#define SE_CACHE_MEM (10*1024*1024) // 10 MB

static void genFloatSamples(int sampleCount, int sdlFormat, const void *in, float *out)
{
	(void) genFloatSamples;

	int i = sampleCount;

	/* Convert from the possible fixed point
	 * formats to [-1;1] floats */
	switch (sdlFormat)
	{
	case AUDIO_U8 :
	{
		const uint8_t *_in = static_cast<const uint8_t*>(in);

		while (i--)
			out[i] = (((float) _in[i] / 255.f) - 0.5) * 2;

		break;
	}
	case AUDIO_S8 :
	{
		const int8_t *_in = static_cast<const int8_t*>(in);

		while (i--)
			out[i] = (float) _in[i] / 128.f; // ???

		break;
	}
	case AUDIO_U16LSB :
	{
		const uint16_t *_in = static_cast<const uint16_t*>(in);

		while (i--)
			out[i] = (((float) _in[i] / 65536.f) - 0.5) * 2;

		break;
	}
	case AUDIO_U16MSB :
	{
		const int16_t *_in = static_cast<const int16_t*>(in);

		while (i--)
		{
			int16_t swp = SDL_Swap16(_in[i]);
			out[i] = (((float) swp / 65536.f) - 0.5) * 2;
		}

		break;
	}
	case AUDIO_S16LSB :
	{
		const int16_t *_in = static_cast<const int16_t*>(in);

		while (i--)
			out[i] = (float) _in[i] / 32768.f; // ???

		break;
	}
	case AUDIO_S16MSB :
	{
		const int16_t *_in = static_cast<const int16_t*>(in);

		while (i--)
		{
			int16_t swp = SDL_Swap16(_in[i]);
			out[i] = (float) swp / 32768.f;
		}

		break;
	}
	qDebug() << "Unhandled sample format";
	default: Q_ASSERT(0); // XXX
	}
}

static uint8_t formatSampleSize(int sdlFormat)
{
	switch (sdlFormat)
	{
	case AUDIO_U8 :
	case AUDIO_S8 :
		return 1;

	case AUDIO_U16LSB :
	case AUDIO_U16MSB :
	case AUDIO_S16LSB :
	case AUDIO_S16MSB :
		return 2;

	case AUDIO_F32 :
		return 4;

	default:
		qDebug() << "Unhandled sample format";
		Q_ASSERT(0);
	}

	return 0;
}

static ALenum chooseALFormat(int sampleSize, int channelCount)
{
	switch (sampleSize)
	{
	case 1 :
		switch (channelCount)
		{
		case 1 : return AL_FORMAT_MONO8;
		case 2 : return AL_FORMAT_STEREO8;
		default: Q_ASSERT(0);
		}
	case 2 :
		switch (channelCount)
		{
		case 1 : return AL_FORMAT_MONO16;
		case 2 : return AL_FORMAT_STEREO16;
		default : Q_ASSERT(0);
		}
	case 4 :
		switch (channelCount)
		{
		case 1 : return AL_FORMAT_MONO_FLOAT32;
		case 2 : return AL_FORMAT_STEREO_FLOAT32;
		default : Q_ASSERT(0);
		}
	default : Q_ASSERT(0);
	}

	return 0;
}

static const int streamBufSize = 32768;

struct SoundBuffer
{
	/* Filename, pitch.
	 * Uniquely identifies this or equal buffer */
	typedef QPair<QByteArray, int> Key;
	Key key;

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

	~SoundBuffer()
	{
		AL::Buffer::del(alBuffer);
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
};

struct SoundEmitter
{
	IntruList<SoundBuffer> buffers;
	QHash<SoundBuffer::Key, SoundBuffer*> bufferHash;

	/* Byte count sum of all cached / playing buffers */
	uint32_t bufferBytes;

	AL::Source::ID alSrcs[SE_SOURCES];
	SoundBuffer *atchBufs[SE_SOURCES];
	/* Index of next source to be used */
	int srcIndex;

#ifdef RUBBERBAND
	std::vector<float> floatBuf;
#endif

	SoundEmitter()
	    : bufferBytes(0),
	      srcIndex(0)
	{
		for (int i = 0; i < SE_SOURCES; ++i)
		{
			alSrcs[i] = AL::Source::gen();
			atchBufs[i] = 0;
		}
#ifdef RUBBERBAND
		floatBuf.resize(streamBufSize/4);
#endif
	}

	~SoundEmitter()
	{
		for (int i = 0; i < SE_SOURCES; ++i)
		{
			AL::Source::stop(alSrcs[i]);
			AL::Source::del(alSrcs[i]);

			if (atchBufs[i])
				SoundBuffer::deref(atchBufs[i]);
		}

		QHash<SoundBuffer::Key, SoundBuffer*>::iterator iter;
		for (iter = bufferHash.begin(); iter != bufferHash.end(); ++iter)
			delete iter.value();
	}

	void play(const QByteArray &filename,
	          int volume,
	          int pitch)
	{
		float _volume = clamp<int>(volume, 0, 100) / 100.f;

		SoundBuffer *buffer = allocateBuffer(filename, pitch);

		int soundIndex = srcIndex++;
		if (srcIndex > SE_SOURCES-1)
			srcIndex = 0;

		AL::Source::ID src = alSrcs[soundIndex];
		AL::Source::stop(src);
		AL::Source::detachBuffer(src);

		SoundBuffer *old = atchBufs[soundIndex];

		if (old)
			SoundBuffer::deref(old);

		atchBufs[soundIndex] = SoundBuffer::ref(buffer);

		AL::Source::attachBuffer(src, buffer->alBuffer);
		AL::Source::setVolume(src, _volume);

#ifndef RUBBERBAND
		AL::Source::setPitch(src, clamp<int>(pitch, 50, 150) / 100.f);
#endif

		AL::Source::play(src);
	}

	void stop()
	{
		for (int i = 0; i < SE_SOURCES; i++)
			AL::Source::stop(alSrcs[i]);
	}

private:
#ifdef RUBBERBAND
	void ensureFloatBufSize(uint32_t size)
	{
		if (size <= floatBuf.size())
			return;

		floatBuf.resize(size);
	}
#endif

	SoundBuffer *allocateBuffer(const QByteArray &filename,
	                            int pitch)
	{
#ifndef RUBBERBAND
		pitch = 100;
#endif

		SoundBuffer::Key soundKey(filename, pitch);

		SoundBuffer *buffer = bufferHash.value(soundKey, 0);

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

			shState->fileSystem().openRead(dataSource, filename.constData(),
			                               FileSystem::Audio, false, &extension);

			Sound_Sample *sampleHandle = Sound_NewSample(&dataSource, extension, 0, streamBufSize);

			if (!sampleHandle)
			{
				SDL_RWclose(&dataSource);
				throw Exception(Exception::SDLError, "SDL_sound: %s", Sound_GetError());
			}

			uint32_t decBytes = Sound_DecodeAll(sampleHandle);
			uint8_t sampleSize = formatSampleSize(sampleHandle->actual.format);
			uint32_t sampleCount = decBytes / sampleSize;

			uint32_t actSampleSize;
#ifdef RUBBERBAND
			actSampleSize = sizeof(float);
#else
			actSampleSize = sampleSize;
#endif

			buffer = new SoundBuffer;
			buffer->key = soundKey;
			buffer->bytes = actSampleSize * sampleCount;

			ALenum alFormat = chooseALFormat(actSampleSize, sampleHandle->actual.channels);

#ifdef RUBBERBAND
			/* Fill float buffer */
			ensureFloatBufSize(sampleCount);
			genFloatSamples(sampleCount, sampleHandle->actual.format,
			                sampleHandle->buffer, floatBuf.data());

			// XXX apply stretcher

			/* Upload data to AL */
			AL::Buffer::uploadData(buffer->alBuffer, alFormat, floatBuf.data(),
			                       buffer->bytes, sampleHandle->actual.rate);
#else
			AL::Buffer::uploadData(buffer->alBuffer, alFormat, sampleHandle->buffer,
			                       buffer->bytes, sampleHandle->actual.rate);
#endif
			Sound_FreeSample(sampleHandle);

			uint32_t wouldBeBytes = bufferBytes + buffer->bytes;

			/* If memory limit is reached, delete lowest priority buffer
			 * until there is room or no buffers left */
			while (wouldBeBytes > SE_CACHE_MEM && !buffers.isEmpty())
			{
				SoundBuffer *last = buffers.tail();
				bufferHash.remove(last->key);
				buffers.remove(last->link);

				wouldBeBytes -= last->bytes;

				SoundBuffer::deref(last);
			}

			bufferHash.insert(soundKey, buffer);
			buffers.prepend(buffer->link);

			bufferBytes = wouldBeBytes;

			return buffer;
		}
	}
};

static const int streamBufs = 3;

struct ALDataSource
{
	enum Status
	{
		NoError,
		EndOfStream,
		WrapAround,
		Error
	};

	virtual ~ALDataSource() {}

	/* Read/process next chunk of data, and attach it
	 * to provided AL buffer */
	virtual Status fillBuffer(AL::Buffer::ID alBuffer) = 0;

	virtual float samplesToOffset(uint32_t samples) = 0;

	virtual void seekToOffset(float offset) = 0;

	/* Seek back to start */
	virtual void reset() = 0;
};

struct SDLSoundSource : ALDataSource
{
	Sound_Sample *sample;
	SDL_RWops ops;
	uint8_t sampleSize;
	bool looped;

	ALenum alFormat;
	ALsizei alFreq;

	SDLSoundSource(const std::string &filename,
	               uint32_t maxBufSize,
	               bool looped,
	               float pitch)
	    : looped(looped)
	{
		(void) pitch;

		const char *extension;
		shState->fileSystem().openRead(ops,
		                               filename.c_str(),
		                               FileSystem::Audio,
		                               false, &extension);

		sample = Sound_NewSample(&ops, extension, 0, maxBufSize);

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

		// XXX apply stretcher here

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

	float samplesToOffset(uint32_t samples)
	{
		uint32_t frames = samples / sample->actual.channels;

		return static_cast<float>(frames) / sample->actual.rate;
	}

	void seekToOffset(float offset)
	{
		Sound_Seek(sample, static_cast<uint32_t>(offset * 1000));
	}

	void reset()
	{
		Sound_Rewind(sample);
	}
};

/* State-machine like audio playback stream.
 * This class is NOT thread safe */
struct ALStream
{
	enum State
	{
		Closed,
		Stopped,
		Playing,
		Paused
	};

	bool looped;
	State state;

	ALDataSource *source;
	SDL_Thread *thread;

	SDL_mutex *pauseMut;
	bool preemptPause;

	/* When this flag isn't set and alSrc is
	 * in 'STOPPED' state, stream isn't over
	 * (it just hasn't started yet) */
	bool streamInited;
	bool sourceExhausted;

	bool threadTermReq;

	bool needsRewind;

	AL::Source::ID alSrc;
	AL::Buffer::ID alBuf[streamBufs];

	uint64_t procSamples;
	AL::Buffer::ID lastBuf;

	struct
	{
		ALenum format;
		ALsizei freq;
	} stream;

	enum LoopMode
	{
		Looped,
		NotLooped
	};

	ALStream(LoopMode loopMode)
	    : looped(loopMode == Looped),
	      state(Closed),
	      source(0),
	      thread(0),
	      preemptPause(false),
	      streamInited(false),
	      needsRewind(false)
	{
		alSrc = AL::Source::gen();

		AL::Source::setVolume(alSrc, 1.0);
		AL::Source::setPitch(alSrc, 1.0);
		AL::Source::detachBuffer(alSrc);

		for (int i = 0; i < streamBufs; ++i)
			alBuf[i] = AL::Buffer::gen();

		pauseMut = SDL_CreateMutex();
	}

	~ALStream()
	{
		close();

		clearALQueue();

		AL::Source::del(alSrc);

		for (int i = 0; i < streamBufs; ++i)
			AL::Buffer::del(alBuf[i]);

		SDL_DestroyMutex(pauseMut);
	}

	void close()
	{
		checkStopped();

		switch (state)
		{
		case Playing:
		case Paused:
			stopStream();
		case Stopped:
			closeSource();
			state = Closed;
		case Closed:
			return;
		}
	}

	void open(const std::string &filename,
	          float pitch)
	{
		checkStopped();

		switch (state)
		{
		case Playing:
		case Paused:
			stopStream();
		case Stopped:
			closeSource();
		case Closed:
			openSource(filename, pitch);
		}

		state = Stopped;
	}

	void stop()
	{
		checkStopped();

		switch (state)
		{
		case Closed:
		case Stopped:
			return;
		case Playing:
		case Paused:
			stopStream();
		}

		state = Stopped;
	}

	void play()
	{
		checkStopped();

		switch (state)
		{
		case Closed:
		case Playing:
			return;
		case Stopped:
			startStream();
			break;
		case Paused :
			resumeStream();
		}

		state = Playing;
	}

	void pause()
	{
		checkStopped();

		switch (state)
		{
		case Closed:
		case Stopped:
		case Paused:
			return;
		case Playing:
			pauseStream();
		}

		state = Paused;
	}

	void setOffset(float value)
	{
		if (state == Closed)
			return;
		// XXX needs more work. protect source with mutex
		source->seekToOffset(value);
		needsRewind = false;
	}

	void setVolume(float value)
	{
		AL::Source::setVolume(alSrc, value);
	}

	State queryState()
	{
		checkStopped();

		return state;
	}

	float queryOffset()
	{
		if (state == Closed)
			return 0;

		uint32_t sampleSum =
			procSamples + AL::Source::getSampleOffset(alSrc);

		return source->samplesToOffset(sampleSum);
	}

private:
	void closeSource()
	{
		delete source;
	}

	void openSource(const std::string &filename,
	                float pitch)
	{
		source = new SDLSoundSource(filename, streamBufSize, looped, pitch);
		needsRewind = false;

#ifndef RUBBERBAND
		AL::Source::setPitch(alSrc, pitch);
#endif
	}

	void stopStream()
	{
		threadTermReq = true;

		AL::Source::stop(alSrc);

		if (thread)
		{
			SDL_WaitThread(thread, 0);
			thread = 0;
			needsRewind = true;
		}

		procSamples = 0;
	}

	void startStream()
	{
		clearALQueue();

		procSamples = 0;

		preemptPause = false;
		streamInited = false;
		sourceExhausted = false;
		threadTermReq = false;
		thread = SDL_CreateThread(streamDataFun, "al_stream", this);
	}

	void pauseStream()
	{
		SDL_LockMutex(pauseMut);

		if (AL::Source::getState(alSrc) != AL_PLAYING)
			preemptPause = true;
		else
			AL::Source::pause(alSrc);

		SDL_UnlockMutex(pauseMut);
	}

	void resumeStream()
	{
		SDL_LockMutex(pauseMut);

		if (preemptPause)
			preemptPause = false;
		else
			AL::Source::play(alSrc);

		SDL_UnlockMutex(pauseMut);
	}

	void checkStopped()
	{
		/* This only concerns the scenario where
		 * state is still 'Playing', but the stream
		 * has already ended on its own (EOF, Error) */
		if (state != Playing)
			return;

		/* If streaming thread hasn't queued up
		 * buffers yet there's not point in querying
		 * the AL source */
		if (!streamInited)
			return;

		/* If alSrc isn't playing, but we haven't
		 * exhausted the data source yet, we're just
		 * having a buffer underrun */
		if (!sourceExhausted)
			return;

		if (AL::Source::getState(alSrc) == AL_PLAYING)
			return;

		stopStream();
		state = Stopped;
	}

	void clearALQueue()
	{
		/* Unqueue all buffers */
		ALint queuedBufs = AL::Source::getProcBufferCount(alSrc);

		while (queuedBufs--)
			AL::Source::unqueueBuffer(alSrc);
	}

	/* thread func */
	void streamData()
	{
		/* Fill up queue */
		bool firstBuffer = true;
		ALDataSource::Status status;

		if (needsRewind)
			source->reset();

		for (int i = 0; i < streamBufs; ++i)
		{
			AL::Buffer::ID buf = alBuf[i];

			status = source->fillBuffer(buf);

			if (status == ALDataSource::Error)
				return;

			AL::Source::queueBuffer(alSrc, buf);

			if (firstBuffer)
			{
				resumeStream();

				firstBuffer = false;
				streamInited = true;
			}

			if (threadTermReq)
				return;

			if (status == ALDataSource::EndOfStream)
			{
				sourceExhausted = true;
				break;
			}
		}

		/* Wait for buffers to be consumed, then
		 * refill and queue them up again */
		while (true)
		{
			ALint procBufs = AL::Source::getProcBufferCount(alSrc);

			while (procBufs--)
			{
				if (threadTermReq)
					break;

				AL::Buffer::ID buf = AL::Source::unqueueBuffer(alSrc);

				if (buf == lastBuf)
				{
					/* Reset the processed sample count so
					 * querying the playback offset returns 0.0 again */
					procSamples = 0;
					lastBuf = AL::Buffer::ID(0);
				}
				else
				{
					/* Add the sample count contained in this
					 * buffer to the total count */
					ALint bits = AL::Buffer::getBits(buf);
					ALint size = AL::Buffer::getSize(buf);

					procSamples += (size / (bits / 8));
				}

				if (sourceExhausted)
					continue;

				status = source->fillBuffer(buf);

				if (status == ALDataSource::Error)
				{
					sourceExhausted = true;
					return;
				}

				AL::Source::queueBuffer(alSrc, buf);

				/* In case of buffer underrun,
				 * start playing again */
				if (AL::Source::getState(alSrc) == AL_STOPPED)
					AL::Source::play(alSrc);

				/* If this was the last buffer before the data
				 * source loop wrapped around again, mark it as
				 * such so we can catch it and reset the processed
				 * sample count once it gets unqueued */
				if (status == ALDataSource::WrapAround)
					lastBuf = buf;

				if (status == ALDataSource::EndOfStream)
					sourceExhausted = true;
			}

			if (threadTermReq)
				break;

			SDL_Delay(AUDIO_SLEEP);
		}
	}

	static int streamDataFun(void *_self)
	{
		ALStream &self = *static_cast<ALStream*>(_self);
		self.streamData();
		return 0;
	}
};

struct AudioStream
{
	struct
	{
		std::string filename;
		float volume;
		float pitch;
	} current;

	/* Volume set with 'play()' */
	float baseVolume;

	/* Volume set by external threads,
	 * such as for fade-in/out.
	 * Multiplied with intVolume for final
	 * playback volume */
	float fadeVolume;
	float extVolume;

	bool extPaused;

	ALStream stream;
	SDL_mutex *streamMut;

	struct
	{
		/* Fade is in progress */
		bool active;

		/* Request fade thread to finish and
		 * cleanup (like it normally would) */
		bool reqFini;

		/* Request fade thread to terminate
		 * immediately */
		bool reqTerm;

		SDL_Thread *thread;

		/* Amount of reduced absolute volume
		 * per ms of fade time */
		float msStep;

		/* Ticks at start of fade */
		uint32_t startTicks;
	} fade;

	AudioStream(ALStream::LoopMode loopMode)
	    : baseVolume(1.0),
	      fadeVolume(1.0),
	      extVolume(1.0),
	      extPaused(false),
	      stream(loopMode)
	{
		current.volume = 1.0;
		current.pitch = 1.0;

		fade.active = false;
		fade.thread = 0;

		streamMut = SDL_CreateMutex();
	}

	~AudioStream()
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

	void play(const std::string &filename,
	          int volume,
	          int pitch)
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

		switch (sState)
		{
		case ALStream::Paused :
		case ALStream::Playing :
			stream.stop();
		case ALStream::Stopped :
			stream.close();
		case ALStream::Closed :
			break;
		}

		stream.open(filename, _pitch);
		setBaseVolume(_volume);

		current.filename = filename;
		current.volume = _volume;
		current.pitch = _pitch;

		if (!extPaused)
			stream.play();

		unlockStream();
	}

	void stop()
	{
		finiFadeInt();

		lockStream();

		stream.stop();

		unlockStream();
	}

	void fadeOut(int duration)
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

		fade.thread = SDL_CreateThread(fadeThreadFun, "audio_fade", this);

		unlockStream();
	}

	/* Any access to this classes 'stream' member,
	 * whether state query or modification, must be
	 * protected by a 'lock'/'unlock' pair */
	void lockStream()
	{
		SDL_LockMutex(streamMut);
	}

	void unlockStream()
	{
		SDL_UnlockMutex(streamMut);
	}

	void setFadeVolume(float value)
	{
		fadeVolume = value;
		updateVolume();
	}

	void setExtVolume1(float value)
	{
		extVolume = value;
		updateVolume();
	}

private:
	void finiFadeInt()
	{
		if (!fade.thread)
			return;

		fade.reqFini = true;
		SDL_WaitThread(fade.thread, 0);
		fade.thread = 0;
	}

	void updateVolume()
	{
		stream.setVolume(baseVolume * fadeVolume * extVolume);
	}

	void setBaseVolume(float value)
	{
		baseVolume = value;
		updateVolume();
	}

	void fadeThread()
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

	static int fadeThreadFun(void *self)
	{
		static_cast<AudioStream*>(self)->fadeThread();

		return 0;
	}
};

struct AudioPrivate
{
	AudioStream bgm;
	AudioStream bgs;
	AudioStream me;

	SoundEmitter se;

	/* The 'MeWatch' is responsible for detecting
	 * a playing ME, quickly fading out the BGM and
	 * keeping it paused/stopped while the ME plays,
	 * and unpausing/fading the BGM back in again
	 * afterwards */
	enum MeWatchState
	{
		MeNotPlaying,
		BgmFadingOut,
		MePlaying,
		BgmFadingIn
	};

	struct
	{
		SDL_Thread *thread;
		bool active;
		bool termReq;
		MeWatchState state;
	} meWatch;

	AudioPrivate()
	    : bgm(ALStream::Looped),
	      bgs(ALStream::Looped),
	      me(ALStream::NotLooped)
	{
		meWatch.active = false;
		meWatch.termReq = false;
		meWatch.state = MeNotPlaying;
		meWatch.thread = SDL_CreateThread(meWatchFun, "audio_mewatch", this);
	}

	~AudioPrivate()
	{
		meWatch.termReq = true;
		SDL_WaitThread(meWatch.thread, 0);
	}

	void meWatchFunInt()
	{
		const float fadeOutStep = 1.f / (200  / AUDIO_SLEEP);
		const float fadeInStep  = 1.f / (1000 / AUDIO_SLEEP);

		while (true)
		{
			if (meWatch.termReq)
				return;

			switch (meWatch.state)
			{
			case MeNotPlaying:
			{
				me.lockStream();

				if (me.stream.queryState() == ALStream::Playing)
				{
					/* ME playing detected. -> FadeOutBGM */
					bgm.extPaused = true;
					meWatch.state = BgmFadingOut;
				}

				me.unlockStream();

				break;
			}

			case BgmFadingOut :
			{
				me.lockStream();

				if (me.stream.queryState() != ALStream::Playing)
				{
					/* ME has ended while fading OUT BGM. -> FadeInBGM */
					me.unlockStream();
					meWatch.state = BgmFadingIn;

					break;
				}

				bgm.lockStream();

				float vol = bgm.extVolume;
				vol -= fadeOutStep;

				if (vol < 0 || bgm.stream.queryState() != ALStream::Playing)
				{
					/* Either BGM has fully faded out, or stopped midway. -> MePlaying */
					bgm.setExtVolume1(0);
					bgm.stream.pause();
					meWatch.state = MePlaying;
					bgm.unlockStream();
					me.unlockStream();

					break;
				}

				bgm.setExtVolume1(vol);
				bgm.unlockStream();
				me.unlockStream();

				break;
			}

			case MePlaying :
			{
				me.lockStream();

				if (me.stream.queryState() != ALStream::Playing)
				{
					/* ME has ended */
					bgm.lockStream();

					bgm.extPaused = false;

					ALStream::State sState = bgm.stream.queryState();

					if (sState == ALStream::Paused)
					{
						/* BGM is paused. -> FadeInBGM */
						bgm.stream.play();
						meWatch.state = BgmFadingIn;
					}
					else
					{
						/* BGM is stopped. -> MeNotPlaying */
						bgm.setExtVolume1(1.0);
						bgm.stream.play();
						meWatch.state = MeNotPlaying;
					}

					bgm.unlockStream();
				}

				me.unlockStream();

				break;
			}

			case BgmFadingIn :
			{
				bgm.lockStream();

				if (bgm.stream.queryState() == ALStream::Stopped)
				{
					/* BGM stopped midway fade in. -> MeNotPlaying */
					bgm.setExtVolume1(1.0);
					meWatch.state = MeNotPlaying;
					bgm.unlockStream();

					break;
				}

				me.lockStream();

				if (me.stream.queryState() == ALStream::Playing)
				{
					/* ME started playing midway BGM fade in. -> FadeOutBGM */
					bgm.extPaused = true;
					meWatch.state = BgmFadingOut;
					me.unlockStream();
					bgm.unlockStream();

					break;
				}

				float vol = bgm.extVolume;
				vol += fadeInStep;

				if (vol >= 1)
				{
					/* BGM fully faded in. -> MeNotPlaying */
					vol = 1.0;
					meWatch.state = MeNotPlaying;
				}

				bgm.setExtVolume1(vol);

				me.unlockStream();
				bgm.unlockStream();

				break;
			}
			}

			SDL_Delay(AUDIO_SLEEP);
		}
	}

	static int meWatchFun(void *self)
	{
		static_cast<AudioPrivate*>(self)->meWatchFunInt();

		return 0;
	}
};

Audio::Audio()
	: p(new AudioPrivate)
{}


void Audio::bgmPlay(const char *filename,
					int volume,
                    int pitch)
{
	p->bgm.play(filename, volume, pitch);
}

void Audio::bgmStop()
{
	p->bgm.stop();
}

void Audio::bgmFade(int time)
{
	p->bgm.fadeOut(time);
}


void Audio::bgsPlay(const char *filename,
					int volume,
                    int pitch)
{
	p->bgs.play(filename, volume, pitch);
}

void Audio::bgsStop()
{
	p->bgs.stop();
}

void Audio::bgsFade(int time)
{
	p->bgs.fadeOut(time);
}


void Audio::mePlay(const char *filename,
                   int volume,
                   int pitch)
{
	p->me.play(filename, volume, pitch);
}

void Audio::meStop()
{
	p->me.stop();
}

void Audio::meFade(int time)
{
	p->me.fadeOut(time);
}


void Audio::sePlay(const char *filename,
                   int volume,
                   int pitch)
{
	p->se.play(filename, volume, pitch);
}

void Audio::seStop()
{
	p->se.stop();
}

Audio::~Audio() { delete p; }
