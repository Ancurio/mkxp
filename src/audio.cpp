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

#include "globalstate.h"
#include "util.h"
#include "intrulist.h"
#include "filesystem.h"
#include "exception.h"

#include <SFML/Audio/Music.hpp>
#include <SFML/Audio/Sound.hpp>
#include <SFML/Audio/SoundBuffer.hpp>
#include <SFML/System/Thread.hpp>
#include <SFML/System/Clock.hpp>
#include <SFML/System/Sleep.hpp>
#include <QByteArray>
#include <QHash>

#include "SDL2/SDL_thread.h"

//#include "SDL2/SDL_mixer.h"

#include <QDebug>

#define FADE_SLEEP 10
#define SOUND_MAG_SIZE 10
#define SOUND_MAX_MEM (50*1000*1000) // 5 MB

struct MusicEntity
{
	sf::Music music;
	QByteArray filename;
	FileStream currentData;
	SDL_mutex *mutex;

	int pitch;

	volatile bool fading;
	struct
	{
		unsigned int duration;
		float msStep;
		volatile bool terminate;
		sf::Thread *thread;
	} fadeData;

	/* normalized */
	float intVolume;
	float extVolume;

	bool extPaused;
	bool noFadeInFlag;

	MusicEntity()
	    : currentData(0),
	      pitch(100),
	      fading(false),
	      intVolume(1),
	      extVolume(1),
	      extPaused(false),
	      noFadeInFlag(false)
	{
		music.setLoop(true);
		fadeData.thread = 0;
		mutex = SDL_CreateMutex();
	}

	~MusicEntity()
	{
		SDL_DestroyMutex(mutex);
	}

	void play(const QByteArray &filename,
	          int volume,
	          int pitch)
	{
		terminateFade();

		volume = clamp<int>(volume, 0, 100);
		pitch = clamp<int>(pitch, 50, 150);

		if (filename == this->filename
		&&  volume == (int)music.getVolume()
//		&&  pitch == music.getPitch()
		&&  music.getStatus() == sf::Music::Playing)
			return;

		if (filename == this->filename
		&&  music.getStatus() == sf::Music::Playing)
//		&&  pitch == music.getPitch())
		{
			intVolume = (float) volume / 100;
			updateVolumeSync();
			return;
		}

		SDL_LockMutex(mutex);

		this->music.stop();

		this->filename = filename;

		currentData.close();

		currentData =
		        gState->fileSystem().openRead(filename.constData(), FileSystem::Audio);

		if (!this->music.openFromStream(currentData))
			return;

		intVolume = (float) volume / 100;
		updateVolume();
//		this->music.setPitch((float) pitch / 100);

		if (!extPaused)
			this->music.play();
		else
			noFadeInFlag = true;

		SDL_UnlockMutex(mutex);
	}

	void stop()
	{
		terminateFade();

		stopPriv();
	}

	void fade(unsigned int ms)
	{
		if (music.getStatus() != sf::Music::Playing)
			return;

		if (fading)
			return;

		delete fadeData.thread;
		fadeData.thread = new sf::Thread(&MusicEntity::processFade, this);
		fadeData.duration = ms;
		fadeData.terminate = false;

		fading = true;
		fadeData.thread->launch();
	}

	void updateVolume()
	{
		music.setVolume(intVolume * extVolume * 100);
	}

	void updateVolumeSync()
	{
		SDL_LockMutex(mutex);
		updateVolume();
		SDL_UnlockMutex(mutex);
	}

private:
	void terminateFade()
	{
		if (!fading)
		{
			delete fadeData.thread;
			fadeData.thread = 0;
			return;
		}

		/* Tell our thread to wrap up and wait for it */
		fadeData.terminate = true;
		fadeData.thread->wait();

		fading = false;

		delete fadeData.thread;
		fadeData.thread = 0;
	}

	void stopPriv()
	{
		SDL_LockMutex(mutex);
		music.stop();
		SDL_UnlockMutex(mutex);

		filename = QByteArray();
	}

	void processFade()
	{
		float msStep = music.getVolume() / fadeData.duration;
		sf::Clock timer;
		sf::Time sleepTime = sf::milliseconds(FADE_SLEEP);
		unsigned int currentDur = 0;

		while (true)
		{
			int elapsed = timer.getElapsedTime().asMilliseconds();
			timer.restart();
			currentDur += elapsed;

			if (music.getStatus() != sf::Music::Playing)
				break;

			if (fadeData.terminate)
				break;

			if (currentDur >= fadeData.duration)
				break;

			intVolume = (float) (music.getVolume() - (elapsed * msStep)) / 100;
			updateVolumeSync();

			sf::sleep(sleepTime);
		}

		stopPriv();
		fading = false;
	}
};

//struct SoundBuffer
//{
//	Mix_Chunk *sdlBuffer;
//	QByteArray filename;
//	IntruListLink<SoundBuffer> link;

//	SoundBuffer()
//	    : link(this)
//	{}
//};

//struct SoundEntity
//{
//	IntruList<SoundBuffer> buffers;
//	QHash<QByteArray, SoundBuffer*> bufferHash;
//	uint cacheSize; // in chunks, for now
//	int channelIndex;

//	SoundEntity()
//	    : cacheSize(0),
//	      channelIndex(0)
//	{
//		Mix_AllocateChannels(SOUND_MAG_SIZE);
//	}

//	~SoundEntity()
//	{
//		IntruListLink<SoundBuffer> *iter = buffers.iterStart();
//		iter = iter->next;

//		for (int i = 0; i < buffers.getSize(); ++i)
//		{
//			SoundBuffer *buffer = iter->data;
//			iter = iter->next;
//			delete buffer;
//		}
//	}

//	void play(const char *filename,
//	          int volume,
//	          int pitch)
//	{
//		(void) pitch;

//		volume = bound(volume, 0, 100);

//		Mix_Chunk *buffer = requestBuffer(filename);

//		int nextChIdx = channelIndex++;
//		if (channelIndex > SOUND_MAG_SIZE-1)
//			channelIndex = 0;

//		Mix_HaltChannel(nextChIdx);
//		Mix_Volume(nextChIdx, ((float) MIX_MAX_VOLUME / 100) * volume);
//		Mix_PlayChannelTimed(nextChIdx, buffer, 0, -1);
//	}

//	void stop()
//	{
//		/* Stop all channels */
//		Mix_HaltChannel(-1);
//	}

//private:
//	Mix_Chunk *requestBuffer(const char *filename)
//	{
//		SoundBuffer *buffer = bufferHash.value(filename, 0);

//		if (buffer)
//		{
//			/* Buffer still in cashe.
//			 * Move to front of priority list */
//			buffers.remove(buffer->link);
//			buffers.append(buffer->link);

//			return buffer->sdlBuffer;
//		}
//		else
//		{
//			/* Buffer not in cashe, needs to be loaded */
//			SDL_RWops ops;
//			gState->fileSystem().openRead(ops, filename, FileSystem::Audio);

//			Mix_Chunk *sdlBuffer = Mix_LoadWAV_RW(&ops, 1);

//			if (!sdlBuffer)
//			{
//				SDL_RWclose(&ops);
//				throw Exception(Exception::RGSSError, "Unable to read sound file");
//			}

//			buffer = new SoundBuffer;
//			buffer->sdlBuffer = sdlBuffer;
//			buffer->filename = filename;

//			++cacheSize;

//			bufferHash.insert(filename, buffer);
//			buffers.prepend(buffer->link);

//			if (cacheSize > 20)
//			{
//				SoundBuffer *last = buffers.tail();
//				bufferHash.remove(last->filename);
//				buffers.remove(last->link);
//				--cacheSize;

//				qDebug() << "Deleted buffer" << last->filename;

//				delete last;
//			}

//			return buffer->sdlBuffer;
//		}
//	}
//};

struct SoundBuffer
{
	sf::SoundBuffer sfBuffer;
	QByteArray filename;
	IntruListLink<SoundBuffer> link;

	SoundBuffer()
	    : link(this)
	{}
};

struct SoundEntity
{
	IntruList<SoundBuffer> buffers;
	QHash<QByteArray, SoundBuffer*> bufferHash;
	unsigned int bufferSamples;

	sf::Sound soundMag[SOUND_MAG_SIZE];
	int magIndex;

	SoundEntity()
	    : bufferSamples(0),
	      magIndex(0)
	{}

	~SoundEntity()
	{
		QHash<QByteArray, SoundBuffer*>::iterator iter;
		for (iter = bufferHash.begin(); iter != bufferHash.end(); ++iter)
			delete iter.value();
	}

	void play(const char *filename,
	          int volume,
	          int pitch)
	{
		(void) pitch;

		volume = clamp<int>(volume, 0, 100);

		sf::SoundBuffer &buffer = allocateBuffer(filename);

		int soundIndex = magIndex++;
		if (magIndex > SOUND_MAG_SIZE-1)
			magIndex = 0;

		sf::Sound &sound = soundMag[soundIndex];
		sound.stop();
		sound.setBuffer(buffer);
		sound.setVolume(volume);

		sound.play();
	}

	void stop()
	{
		for (int i = 0; i < SOUND_MAG_SIZE; i++)
			soundMag[i].stop();
	}

private:
	sf::SoundBuffer &allocateBuffer(const char *filename)
	{
		SoundBuffer *buffer = bufferHash.value(filename, 0);

		if (buffer)
		{
			/* Buffer still in cashe.
			 * Move to front of priority list */

			buffers.remove(buffer->link);
			buffers.append(buffer->link);

			return buffer->sfBuffer;
		}
		else
		{
			/* Buffer not in cashe, needs to be loaded */

			FileStream data =
				gState->fileSystem().openRead(filename, FileSystem::Audio);

			buffer = new SoundBuffer;
			buffer->sfBuffer.loadFromStream(data);
			bufferSamples += buffer->sfBuffer.getSampleCount();

			data.close();

//			qDebug() << "SoundCache: Current memory consumption:" << bufferSamples/2;

			buffer->filename = filename;

			bufferHash.insert(filename, buffer);
			buffers.prepend(buffer->link);

			// FIXME this part would look better if it actually looped until enough memory is freed
			/* If memory limit is reached, delete lowest priority buffer.
			 * Samples are 2 bytes big */
			if ((bufferSamples/2) > SOUND_MAX_MEM && !buffers.isEmpty())
			{
				SoundBuffer *last = buffers.tail();
				bufferHash.remove(last->filename);
				buffers.remove(last->link);
				bufferSamples -= last->sfBuffer.getSampleCount();

				qDebug() << "Deleted buffer" << last->filename;

				delete last;
			}

			return buffer->sfBuffer;
		}
	}
};

struct AudioPrivate
{
	MusicEntity bgm;
	MusicEntity bgs;
	MusicEntity me;

	SoundEntity se;

	sf::Thread *meWatchThread;
	bool meWatchRunning;
	bool meWatchThreadTerm;

	AudioPrivate()
	    : meWatchThread(0),
	      meWatchRunning(false),
	      meWatchThreadTerm(false)
	{
		me.music.setLoop(false);
	}

	~AudioPrivate()
	{
		if (meWatchThread)
		{
			meWatchThreadTerm = true;
			meWatchThread->wait();
			delete meWatchThread;
		}

		bgm.stop();
		bgs.stop();
		me.stop();
		se.stop();
	}

	void scheduleMeWatch()
	{
		if (meWatchRunning)
			return;

		meWatchRunning = true;

		if (meWatchThread)
			meWatchThread->wait();

		bgm.extPaused = true;

		delete meWatchThread;
		meWatchThread = new sf::Thread(&AudioPrivate::meWatchFunc, this);
		meWatchThread->launch();
	}

	void meWatchFunc()
	{
		// FIXME Need to catch the case where an ME is started while
		// the BGM is still being faded in from this function
		sf::Time sleepTime = sf::milliseconds(FADE_SLEEP);
		const int bgmFadeOutSteps = 20;
		const int bgmFadeInSteps = 100;

		/* Fade out BGM */
		for (int i = bgmFadeOutSteps; i > 0; --i)
		{
			if (meWatchThreadTerm)
				return;

			if (bgm.music.getStatus() != sf::Music::Playing)
			{
				bgm.extVolume = 0;
				bgm.updateVolumeSync();
				break;
			}

			bgm.extVolume = (1.0 / bgmFadeOutSteps) * (i-1);
			bgm.updateVolumeSync();
			sf::sleep(sleepTime);
		}

		SDL_LockMutex(bgm.mutex);
		if (bgm.music.getStatus() == sf::Music::Playing)
			bgm.music.pause();
		SDL_UnlockMutex(bgm.mutex);

		/* Linger while ME plays */
		while (me.music.getStatus() == sf::Music::Playing)
		{
			if (meWatchThreadTerm)
				return;

			sf::sleep(sleepTime);
		}

		SDL_LockMutex(bgm.mutex);
		bgm.extPaused = false;

		if (bgm.music.getStatus() == sf::Music::Paused)
			bgm.music.play();
		SDL_UnlockMutex(bgm.mutex);

		/* Fade in BGM again */
		for (int i = 0; i < bgmFadeInSteps; ++i)
		{
			if (meWatchThreadTerm)
				return;

			SDL_LockMutex(bgm.mutex);

			if (bgm.music.getStatus() != sf::Music::Playing || bgm.noFadeInFlag)
			{
				bgm.noFadeInFlag = false;
				bgm.extVolume = 1;
				bgm.updateVolume();
				bgm.music.play();
				SDL_UnlockMutex(bgm.mutex);
				break;
			}

			bgm.extVolume = (1.0 / bgmFadeInSteps) * (i+1);
			bgm.updateVolume();

			SDL_UnlockMutex(bgm.mutex);

			sf::sleep(sleepTime);
		}

		meWatchRunning = false;
	}
};

Audio::Audio()
	: p(new AudioPrivate)
{
}


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
	p->bgm.fade(time);
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
	p->bgs.fade(time);
}


void Audio::mePlay(const char *filename,
                   int volume,
                   int pitch)
{
	p->me.play(filename, volume, pitch);
	p->scheduleMeWatch();
}

void Audio::meStop()
{
	p->me.stop();
}

void Audio::meFade(int time)
{
	p->me.fade(time);
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
