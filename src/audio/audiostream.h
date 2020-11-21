/*
** audiostream.h
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

#ifndef AUDIOSTREAM_H
#define AUDIOSTREAM_H

#include "al-util.h"
#include "alstream.h"
#include "sdl-util.h"

#include <string>

struct AudioStream
{
	struct
	{
		std::string filename;
		float volume;
		float pitch;
	} current;

	/* Volumes set by external threads,
	 * such as for fade-in/out.
	 * Multiplied together for final
	 * playback volume. Used with setVolume().
	 * Base is set by play().
	 * External is used by MeWatch */
	enum VolumeType
	{
		Base = 0,
		FadeOut,
		FadeIn,
		External,

		VolumeTypeCount
	};

	/* Note that 'extPaused' and 'noResumeStop' are
	 * effectively only used with the AudioStream
	 * instance representing the BGM.
	 * They are not AtomicFlags because they're regarded
	 * as part of the underlying stream state, and
	 * always accessed with the stream lock held */

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

	/* Fade out */
	struct
	{
		/* Fade out is in progress */
		AtomicFlag active;

		/* Request fade thread to finish and
		 * cleanup (like it normally would) */
		AtomicFlag reqFini;

		/* Request fade thread to terminate
		 * immediately */
		AtomicFlag reqTerm;

		SDL_Thread *thread;
		std::string threadName;

		/* Amount of reduced absolute volume
		 * per ms of fade time */
		float msStep;

		/* Ticks at start of fade */
		uint32_t startTicks;
	} fade;

	/* Fade in */
	struct
	{
		AtomicFlag rqFini;
		AtomicFlag rqTerm;

		SDL_Thread *thread;
		std::string threadName;

		uint32_t startTicks;
	} fadeIn;

	AudioStream(ALStream::LoopMode loopMode,
	            const std::string &threadId);
	~AudioStream();

	void play(const std::string &filename,
	          int volume,
	          int pitch,
	          float offset = 0);
	void stop();
	void fadeOut(int duration);

	/* Any access to this classes 'stream' member,
	 * whether state query or modification, must be
	 * protected by a 'lock'/'unlock' pair */
	void lockStream();
	void unlockStream();

	void setVolume(VolumeType type, float value);
	float getVolume(VolumeType type);

	float playingOffset();

private:
	float volumes[VolumeTypeCount];
	void updateVolume();

	void finiFadeOutInt();
	void startFadeIn();

	void fadeOutThread();
	void fadeInThread();
};

#endif // AUDIOSTREAM_H
