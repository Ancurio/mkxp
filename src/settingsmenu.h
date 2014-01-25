/*
** settingsmenu.h
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

#ifndef SETTINGSMENU_H
#define SETTINGSMENU_H

#include <stdint.h>

struct SettingsMenuPrivate;
struct RGSSThreadData;
union SDL_Event;

class SettingsMenu
{
public:
	SettingsMenu(RGSSThreadData &rtData);
	~SettingsMenu();

	/* Returns true if the event was consumed */
	bool onEvent(const SDL_Event &event);
	void raise();
	bool destroyReq() const;

private:
	SettingsMenuPrivate *p;
};

#endif // SETTINGSMENU_H
