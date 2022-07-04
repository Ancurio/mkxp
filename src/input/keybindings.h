/*
** keybindings.h
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

#ifndef KEYBINDINGS_H
#define KEYBINDINGS_H

#include "input.h"

#include <SDL_scancode.h>
#include <SDL_gamecontroller.h>
#include <stdint.h>
#include <assert.h>
#include <vector>

enum AxisDir
{
	Negative,
	Positive
};

enum SourceType
{
	Invalid,
	Key,
    CButton,
    CAxis
};

struct SourceDesc
{
	SourceType type;

	union Data
	{
		/* Keyboard scancode */
		SDL_Scancode scan;
		/* Joystick button index */
		SDL_GameControllerButton cb;
		struct
		{
			/* Joystick axis index */
			SDL_GameControllerAxis axis;
			/* Joystick axis direction */
			AxisDir dir;
		} ca;
	} d;

	bool operator==(const SourceDesc &o) const
	{
		if (type != o.type)
			return false;

		switch (type)
		{
		case Invalid:
			return true;
		case Key:
			return d.scan == o.d.scan;
        case CButton:
            return d.cb == o.d.cb;
		case CAxis:
			return (d.ca.axis == o.d.ca.axis) && (d.ca.dir == o.d.ca.dir);
		default:
			assert(!"unreachable");
			return false;
		}
	}

	bool operator!=(const SourceDesc &o) const
	{
		return !(*this == o);
	}
};

#define JAXIS_THRESHOLD 0x4000

struct BindingDesc
{
	SourceDesc src;
	Input::ButtonCode target;
};

typedef std::vector<BindingDesc> BDescVec;
struct Config;

BDescVec genDefaultBindings(const Config &conf);

void storeBindings(const BDescVec &d, const Config &conf);
BDescVec loadBindings(const Config &conf);

#endif // KEYBINDINGS_H
