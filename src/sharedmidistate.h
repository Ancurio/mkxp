/*
** sharedmidistate.h
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

#ifndef SHAREDMIDISTATE_H
#define SHAREDMIDISTATE_H

#include "config.h"
#include "debugwriter.h"

#include <fluidsynth.h>

#include <assert.h>
#include <vector>
#include <string>

#define SYNTH_INIT_COUNT 2
#define SYNTH_SAMPLERATE 44100

struct Synth
{
	fluid_synth_t *synth;
	bool inUse;
};

struct SharedMidiState
{
	bool inited;
	std::vector<Synth> synths;
	const std::string &soundFont;
	fluid_settings_t *flSettings;

	SharedMidiState(const Config &conf)
	    : inited(false),
	      soundFont(conf.midi.soundFont)
	{
		flSettings = new_fluid_settings();
		fluid_settings_setnum(flSettings, "synth.gain", 1.0);
		fluid_settings_setnum(flSettings, "synth.sample-rate", SYNTH_SAMPLERATE);
		fluid_settings_setstr(flSettings, "synth.chorus.active", conf.midi.chorus ? "yes" : "no");
		fluid_settings_setstr(flSettings, "synth.reverb.active", conf.midi.reverb ? "yes" : "no");
	}

	~SharedMidiState()
	{
		delete_fluid_settings(flSettings);

		for (size_t i = 0; i < synths.size(); ++i)
		{
			assert(!synths[i].inUse);
			delete_fluid_synth(synths[i].synth);
		}
	}

	fluid_synth_t *addSynth(bool usedNow)
	{
		fluid_synth_t *syn = new_fluid_synth(flSettings);

		if (!soundFont.empty())
			fluid_synth_sfload(syn, soundFont.c_str(), 1);
		else
			Debug() << "Warning: No soundfont specified, sound might be mute";

		Synth synth;
		synth.inUse = usedNow;
		synth.synth = syn;
		synths.push_back(synth);

		return syn;
	}

	void initDefaultSynths()
	{
		if (inited)
			return;

		for (size_t i = 0; i < SYNTH_INIT_COUNT; ++i)
			addSynth(false);

		inited = true;
	}

	fluid_synth_t *allocateSynth()
	{
		size_t i;

		initDefaultSynths();

		for (i = 0; i < synths.size(); ++i)
			if (!synths[i].inUse)
				break;

		if (i < synths.size())
		{
			fluid_synth_t *syn = synths[i].synth;
			fluid_synth_system_reset(syn);
			synths[i].inUse = true;

			return syn;
		}
		else
		{
			return addSynth(true);
		}
	}

	void releaseSynth(fluid_synth_t *synth)
	{
		size_t i;

		for (i = 0; i < synths.size(); ++i)
			if (synths[i].synth == synth)
				break;

		assert(i < synths.size());

		synths[i].inUse = false;
	}
};

#endif // SHAREDMIDISTATE_H
