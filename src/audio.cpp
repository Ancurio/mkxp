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

#include "audiostream.h"
#include "soundemitter.h"
#include "sharedstate.h"
#include "sharedmidistate.h"

#include <string>

#include <SDL_thread.h>
#include <SDL_timer.h>

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

	AudioPrivate(const Config &conf)
	    : bgm(ALStream::Looped, "bgm"),
	      bgs(ALStream::Looped, "bgs"),
	      me(ALStream::NotLooped, "me"),
	      se(conf)
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

Audio::Audio(const Config &conf)
	: p(new AudioPrivate(conf))
{}


void Audio::bgmPlay(const char *filename,
                    int volume,
                    int pitch,
                    float pos)
{
	p->bgm.play(filename, volume, pitch, pos);
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
                    int pitch,
                    float pos)
{
	p->bgs.play(filename, volume, pitch, pos);
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

void Audio::setupMidi()
{
	shState->midiState().initIfNeeded(shState->config());
}

float Audio::bgmPos()
{
	return p->bgm.playingOffset();
}

float Audio::bgsPos()
{
	return p->bgs.playingOffset();
}

Audio::~Audio() { delete p; }
