/*
** flashable.h
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

#ifndef FLASHABLE_H
#define FLASHABLE_H

#include "etc.h"
#include "etc-internal.h"

class Flashable
{
public:
	Flashable()
	    : flashColor(0, 0, 0, 0),
	      flashing(false),
	      emptyFlashFlag(false)
	{}

	virtual ~Flashable() {}

	void flash(const Vec4 *color, int duration)
	{
		if (duration < 1)
			return;

		flashing = true;
		this->duration = duration;
		counter = 0;

		if (!color)
		{
			emptyFlashFlag = true;
			return;
		}

		flashColor = *color;
		flashAlpha = flashColor.w;
	}

	virtual void update()
	{
		if (!flashing)
			return;

		if (++counter > duration)
		{
			/* Flash finished. Cleanup */
			flashColor = Vec4(0, 0, 0, 0);
			flashing = false;
			emptyFlashFlag = false;
			return;
		}

		/* No need to update flash color on empty flash */
		if (emptyFlashFlag)
			return;

		float prog = (float) counter / duration;
		flashColor.w = flashAlpha * (1 - prog);
	}

protected:
	Vec4 flashColor;
	bool flashing;
	bool emptyFlashFlag;
private:
	float flashAlpha;
	int duration;
	int counter;
};

#endif // FLASHABLE_H
