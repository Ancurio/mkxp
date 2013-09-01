/*
** audio.h
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

#ifndef AUDIO_H
#define AUDIO_H

struct AudioPrivate;

class Audio
{
public:
	Audio();
	~Audio();

	void bgmPlay(const char *filename, int volume = 100, int pitch = 100);
	void bgmStop();
	void bgmFade(int time);

	void bgsPlay(const char *filename, int volume = 100, int pitch = 100);
	void bgsStop();
	void bgsFade(int time);

	void mePlay(const char *filename, int volume = 100, int pitch = 100);
	void meStop();
	void meFade(int time);

	void sePlay(const char *filename, int volume = 100, int pitch = 100);
	void seStop();

private:
	AudioPrivate *p;
};

#endif // AUDIO_H
