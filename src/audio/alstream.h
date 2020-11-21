/*
** alstream.h
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

#ifndef ALSTREAM_H
#define ALSTREAM_H

#include "al-util.h"
#include "sdl-util.h"

#include <string>
#include <SDL_rwops.h>

struct ALDataSource;

#define STREAM_BUFS 3

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
	AtomicFlag streamInited;
	AtomicFlag sourceExhausted;

	AtomicFlag threadTermReq;

	AtomicFlag needsRewind;
	float startOffset;

	float pitch;

	AL::Source::ID alSrc;
	AL::Buffer::ID alBuf[STREAM_BUFS];

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
	         const std::string &threadId);
	~ALStream();

	void close();
	void open(const std::string &filename);
	void stop();
	void play(float offset = 0);
	void pause();

	void setVolume(float value);
	void setPitch(float value);
	State queryState();
	float queryOffset();
	bool queryNativePitch();

private:
	void closeSource();
	void openSource(const std::string &filename);

	void stopStream();
	void startStream(float offset);
	void pauseStream();
	void resumeStream();

	void checkStopped();

	/* thread func */
	void streamData();
};

#endif // ALSTREAM_H
