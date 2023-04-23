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
#include "eventthread.h"
#include "sdl-util.h"

#include <string>
#include <vector>

#include <SDL_thread.h>
#include <SDL_timer.h>

struct AudioPrivate
{
    
    // mkxp-z manages multiple streams of BGM.
    // play/pause states should be shared between them, so checking the state of the first
    // one should be enough to know what the rest are doing.
    std::vector<AudioStream*> bgmTracks;
	AudioStream bgs;
	AudioStream me;

	SoundEmitter se;

	SyncPoint &syncPoint;
    
    float volumeRatio;

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
		AtomicFlag termReq;
		MeWatchState state;
	} meWatch;

	AudioPrivate(RGSSThreadData &rtData)
	    : bgs(ALStream::Looped, "bgs"),
	      me(ALStream::NotLooped, "me"),
	      se(rtData.config),
	      syncPoint(rtData.syncPoint),
          volumeRatio(1)
	{
        for (int i = 0; i < rtData.config.BGM.trackCount; i++) {
            std::string id = std::string("bgm" + std::to_string(i));
            bgmTracks.push_back(new AudioStream(ALStream::Looped, id.c_str()));
        }
        
		meWatch.state = MeNotPlaying;
		meWatch.thread = createSDLThread
			<AudioPrivate, &AudioPrivate::meWatchFun>(this, "audio_mewatch");
	}

	~AudioPrivate()
	{
		meWatch.termReq.set();
		SDL_WaitThread(meWatch.thread, 0);
        for (auto track : bgmTracks)
            delete track;
	}

	void meWatchFun()
	{
		const float fadeOutStep = 1.f / (200  / AUDIO_SLEEP);
		const float fadeInStep  = 1.f / (1000 / AUDIO_SLEEP);

		while (true)
		{
			syncPoint.passSecondarySync();

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
                    for (auto track : bgmTracks)
                        track->extPaused = true;
                    
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
                
                bool shouldBreak = false;
                
                for (int i = 0; i < bgmTracks.size(); i++) {
                    AudioStream *track = bgmTracks[i];
                    
                    track->lockStream();
                    
                    float vol = track->getVolume(AudioStream::External);
                    vol -= fadeOutStep;
                    
                    if (vol < 0 || track->stream.queryState() != ALStream::Playing) {
                        /* Either BGM has fully faded out, or stopped midway. -> MePlaying */
                        track->setVolume(AudioStream::External, 0);
                        track->stream.pause();
                        track->unlockStream();
                        
                        // check to see if there are any tracks still playing,
                        // and if the last one was ended this round, this branch should exit
                        std::vector<AudioStream*> playingTracks;
                        for (auto t : bgmTracks)
                            if (t->stream.queryState() == ALStream::Playing)
                                playingTracks.push_back(t);
                        
                        
                        if (playingTracks.size() <= 0 && !shouldBreak) shouldBreak = true;
                        continue;
                    }
                    
                    track->setVolume(AudioStream::External, vol);
                    track->unlockStream();
                    
                }
                if (shouldBreak) {
                    meWatch.state = MePlaying;
                    me.unlockStream();
                    break;
                }
                
				me.unlockStream();

				break;
			}

			case MePlaying :
			{
				me.lockStream();

				if (me.stream.queryState() != ALStream::Playing)
                {
                    /* ME has ended */
                    for (auto track : bgmTracks) {
                        track->lockStream();
                        track->extPaused = false;
                        
                        ALStream::State sState = track->stream.queryState();
                        
                        if (sState == ALStream::Paused) {
                            /* BGM is paused. -> FadeInBGM */
                            track->stream.play();
                            meWatch.state = BgmFadingIn;
                        }
                        else {
                            /* BGM is stopped. -> MeNotPlaying */
                            track->setVolume(AudioStream::External, 1.0f);
                            
                            if (!track->noResumeStop)
                                track->stream.play();
                        }
                        
                        meWatch.state = MeNotPlaying;
                        
                        track->unlockStream();
                    }
				}

                me.unlockStream();

				break;
			}

			case BgmFadingIn :
			{
                for (auto track : bgmTracks)
                    track->lockStream();

				if (bgmTracks[0]->stream.queryState() == ALStream::Stopped)
				{
					/* BGM stopped midway fade in. -> MeNotPlaying */
                    for (auto track : bgmTracks)
                        track->setVolume(AudioStream::External, 1.0f);
					meWatch.state = MeNotPlaying;
                    for (auto track : bgmTracks)
                        track->unlockStream();

					break;
				}

				me.lockStream();

				if (me.stream.queryState() == ALStream::Playing)
				{
					/* ME started playing midway BGM fade in. -> FadeOutBGM */
                    for (auto track : bgmTracks)
                        track->extPaused = true;
					meWatch.state = BgmFadingOut;
					me.unlockStream();
                    for (auto track : bgmTracks)
                        track->unlockStream();

					break;
				}

				float vol = bgmTracks[0]->getVolume(AudioStream::External);
				vol += fadeInStep;

				if (vol >= 1)
				{
					/* BGM fully faded in. -> MeNotPlaying */
					vol = 1.0f;
					meWatch.state = MeNotPlaying;
				}

                for (auto track : bgmTracks)
                    track->setVolume(AudioStream::External, vol);

				me.unlockStream();
                for (auto track : bgmTracks)
                    track->unlockStream();

				break;
			}
			}

			SDL_Delay(AUDIO_SLEEP);
		}
	}
};

Audio::Audio(RGSSThreadData &rtData)
	: p(new AudioPrivate(rtData))
{}


void Audio::bgmPlay(const char *filename,
                    int volume,
                    int pitch,
                    float pos,
                    int channel)
{
    if (channel == -127) {
        for (auto track : p->bgmTracks)
            track->stop();
        
        channel = 0;
    }
	p->bgmTracks[clamp(channel, 0, (int)p->bgmTracks.size() - 1)]->play(filename, volume, pitch, pos);
}

void Audio::bgmStop(int channel)
{
    if (channel == -127) {
        for (auto track : p->bgmTracks)
            track->stop();
        
        return;
    }
    
    p->bgmTracks[clamp(channel, 0, (int)p->bgmTracks.size() - 1)]->stop();
}

void Audio::bgmFade(int time, int channel)
{
    if (channel == -127) {
        for (auto track : p->bgmTracks)
            track->fadeOut(time);
        
        return;
    }
    
    p->bgmTracks[clamp(channel, 0, (int)p->bgmTracks.size() - 1)]->fadeOut(time);
}

int Audio::bgmGetVolume(int channel)
{
    if (channel == -127)
        return p->bgmTracks[channel]->getVolume(AudioStream::BaseRatio) * 100;
    
    return p->bgmTracks[channel]->getVolume(AudioStream::Base) * 100;
}

void Audio::bgmSetVolume(int volume, int channel)
{
    float vol = volume / 100.0;
    if (channel == -127) {
        for (auto track : p->bgmTracks)
            track->setVolume(AudioStream::BaseRatio, vol);
        
        return;
    }
    channel = clamp(channel, 0, (int)p->bgmTracks.size() - 1);
    p->bgmTracks[channel]->setVolume(AudioStream::Base, vol);
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
	return p->bgmTracks[0]->playingOffset();
}

float Audio::bgsPos()
{
	return p->bgs.playingOffset();
}

void Audio::reset()
{
    for (auto track : p->bgmTracks)
        track->stop();
    
	p->bgs.stop();
	p->me.stop();
	p->se.stop();
}

Audio::~Audio() { delete p; }
