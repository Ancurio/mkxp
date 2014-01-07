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
#include "boost-hash.h"
#include "debugwriter.h"

#include <vector>
#include <string>
#include <assert.h>

#include <SDL_audio.h>
#include <SDL_thread.h>
#include <SDL_endian.h>
#include <SDL_timer.h>
#include <SDL_sound.h>

#ifdef RGSS2
#define OV_EXCLUDE_STATIC_CALLBACKS
#include <vorbis/vorbisfile.h>
#endif

#include <alc.h>

#define AUDIO_SLEEP 10
#define SE_SOURCES 6
#define SE_CACHE_MEM (10*1024*1024) // 10 MB

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
		Debug() << "Unhandled sample format";
		abort();
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
		default: abort();
		}
	case 2 :
		switch (channelCount)
		{
		case 1 : return AL_FORMAT_MONO16;
		case 2 : return AL_FORMAT_STEREO16;
		default : abort();
		}
	case 4 :
		switch (channelCount)
		{
		case 1 : return AL_FORMAT_MONO_FLOAT32;
		case 2 : return AL_FORMAT_STEREO_FLOAT32;
		default : abort();
		}
	default : abort();
	}

	return 0;
}

static const int streamBufSize = 32768;

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

struct SoundEmitter
{
	typedef BoostHash<std::string, SoundBuffer*> BufferHash;

	IntruList<SoundBuffer> buffers;
	BufferHash bufferHash;

	/* Byte count sum of all cached / playing buffers */
	uint32_t bufferBytes;

	AL::Source::ID alSrcs[SE_SOURCES];
	SoundBuffer *atchBufs[SE_SOURCES];
	/* Index of next source to be used */
	int srcIndex;

	SoundEmitter()
	    : bufferBytes(0),
	      srcIndex(0)
	{
		for (int i = 0; i < SE_SOURCES; ++i)
		{
			alSrcs[i] = AL::Source::gen();
			atchBufs[i] = 0;
		}
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

		BufferHash::const_iterator iter;
		for (iter = bufferHash.cbegin(); iter != bufferHash.cend(); ++iter)
			SoundBuffer::deref(iter->second);
	}

	void play(const std::string &filename,
	          int volume,
	          int pitch)
	{
		float _volume = clamp<int>(volume, 0, 100) / 100.f;
		float _pitch  = clamp<int>(pitch, 50, 150) / 100.f;

		SoundBuffer *buffer = allocateBuffer(filename);

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
		AL::Source::setPitch(src, _pitch);

		AL::Source::play(src);
	}

	void stop()
	{
		for (int i = 0; i < SE_SOURCES; i++)
			AL::Source::stop(alSrcs[i]);
	}

private:
	SoundBuffer *allocateBuffer(const std::string &filename)
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

			Sound_Sample *sampleHandle = Sound_NewSample(&dataSource, extension, 0, streamBufSize);

			if (!sampleHandle)
			{
				SDL_RWclose(&dataSource);
				throw Exception(Exception::SDLError, "SDL_sound: %s", Sound_GetError());
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

	virtual int sampleRate() = 0;

	virtual void seekToOffset(float seconds) = 0;

	/* Seek back to start */
	virtual void reset() = 0;

	/* The frame count right after wrap around */
	virtual uint32_t loopStartFrames() = 0;
};

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
		Sound_Seek(sample, static_cast<uint32_t>(seconds * 1000));
	}

	void reset()
	{
		Sound_Rewind(sample);
	}

	uint32_t loopStartFrames()
	{
		/* Loops from the beginning of the file */
		return 0;
	}
};

#ifdef RGSS2
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

		sampleBuf.resize(streamBufSize);

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
					reset();
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

	void reset()
	{
		ov_raw_seek(&vf, 0);
		currentFrame = 0;
	}

	uint32_t loopStartFrames()
	{
		if (loop.valid)
			return loop.start;
		else
			return 0;
	}
};
#endif

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

	std::string threadName;

	SDL_mutex *pauseMut;
	bool preemptPause;

	/* When this flag isn't set and alSrc is
	 * in 'STOPPED' state, stream isn't over
	 * (it just hasn't started yet) */
	bool streamInited;
	bool sourceExhausted;

	bool threadTermReq;

	bool needsRewind;
	float startOffset;

	AL::Source::ID alSrc;
	AL::Buffer::ID alBuf[streamBufs];

	uint64_t procFrames;
	AL::Buffer::ID lastBuf;

	SDL_RWops srcOps;

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

	ALStream(LoopMode loopMode,
	         const std::string &threadId)
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

		threadName = std::string("al_stream (") + threadId + ")";
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

	void open(const std::string &filename)
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
			openSource(filename);
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

	void play(float offset = 0)
	{
		checkStopped();

		switch (state)
		{
		case Closed:
		case Playing:
			return;
		case Stopped:
			startStream(offset);
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

	void setVolume(float value)
	{
		AL::Source::setVolume(alSrc, value);
	}

	void setPitch(float value)
	{
		AL::Source::setPitch(alSrc, value);
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

		float procOffset = static_cast<float>(procFrames) / source->sampleRate();

		return procOffset + AL::Source::getSecOffset(alSrc);
	}

private:
	void closeSource()
	{
		delete source;
	}

	void openSource(const std::string &filename)
	{
		const char *ext;
		shState->fileSystem().openRead(srcOps, filename.c_str(), FileSystem::Audio, false, &ext);

#ifdef RGSS2
		/* Try to read ogg file signature */
		char sig[5];
		memset(sig, '\0', sizeof(sig));
		SDL_RWread(&srcOps, sig, 1, 4);
		SDL_RWseek(&srcOps, 0, RW_SEEK_SET);

		if (!strcmp(sig, "OggS"))
			source = new VorbisSource(srcOps, looped);
		else
			source = new SDLSoundSource(srcOps, ext, streamBufSize, looped);
#else
		source = new SDLSoundSource(srcOps, ext, streamBufSize, looped);
#endif

		needsRewind = false;
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

		procFrames = 0;
	}

	void startStream(float offset)
	{
		clearALQueue();

		preemptPause = false;
		streamInited = false;
		sourceExhausted = false;
		threadTermReq = false;

		startOffset = offset;
		procFrames = offset * source->sampleRate();

		thread = SDL_CreateThread(streamDataFun, threadName.c_str(), this);
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
		{
			if (startOffset > 0)
				source->seekToOffset(startOffset);
			else
				source->reset();
		}

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

				/* If something went wrong, try again later */
				if (buf == AL::Buffer::ID(0))
					break;

				if (buf == lastBuf)
				{
					/* Reset the processed sample count so
					 * querying the playback offset returns 0.0 again */
					procFrames = source->loopStartFrames();
					lastBuf = AL::Buffer::ID(0);
				}
				else
				{
					/* Add the frame count contained in this
					 * buffer to the total count */
					ALint bits = AL::Buffer::getBits(buf);
					ALint size = AL::Buffer::getSize(buf);
					ALint chan = AL::Buffer::getChannels(buf);

					if (bits != 0 && chan != 0)
						procFrames += ((size / (bits / 8)) / chan);
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
	 * playback volume.
	 * fadeVolume: used by fade-out thread.
	 * extVolume: used by MeWatch. */
	float fadeVolume;
	float extVolume;

	/* Note that 'extPaused' and 'noResumeStop' are
	 * effectively only used with the AudioStream
	 * instance representing the BGM */

	/* Flag indicating that the MeWatch paused this
	 * (BGM) stream because a ME started playing.
	 * While this flag is set, calls to 'play()'
	 * might open another file, but will not start
	 * the playback stream (the MeWatch will start
	 * it as soon as the ME finished playing). */
	bool extPaused;

	/* Flag indicating that this stream shouldn't be
	 * started by the MeWatch when it is in stopped
	 * state (eg. because the BGM stream was explicitly
	 * stopped by the user script while the ME was playing.
	 * When a new BGM is started (via 'play()') while an ME
	 * is playing, the file will be loaded without starting
	 * the stream, but we want the MeWatch to start it as
	 * soon as the ME ends, so we unset this flag. */
	bool noResumeStop;

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
		std::string threadName;

		/* Amount of reduced absolute volume
		 * per ms of fade time */
		float msStep;

		/* Ticks at start of fade */
		uint32_t startTicks;
	} fade;

	AudioStream(ALStream::LoopMode loopMode,
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
	          int pitch,
	          float offset = 0)
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

	void stop()
	{
		finiFadeInt();

		lockStream();

		noResumeStop = true;

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

		fade.thread = SDL_CreateThread(fadeThreadFun, fade.threadName.c_str(), this);

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

	float playingOffset()
	{
		return stream.queryOffset();
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
	    : bgm(ALStream::Looped, "bgm"),
	      bgs(ALStream::Looped, "bgs"),
	      me(ALStream::NotLooped, "me")
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

						if (!bgm.noResumeStop)
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
                    int pitch
                    #ifdef RGSS3
                    ,float pos
                    #endif
                    )
{
#ifdef RGSS3
	p->bgm.play(filename, volume, pitch, pos);
#else
	p->bgm.play(filename, volume, pitch);
#endif
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
                    int pitch
                    #ifdef RGSS3
                    ,float pos
                    #endif
                    )
{
#ifdef RGSS3
	p->bgs.play(filename, volume, pitch, pos);
#else
	p->bgs.play(filename, volume, pitch);
#endif
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

#ifdef RGSS3

void Audio::setupMidi()
{

}

float Audio::bgmPos()
{
	return p->bgm.playingOffset();
}

float Audio::bgsPos()
{
	return p->bgs.playingOffset();
}

#endif

Audio::~Audio() { delete p; }
