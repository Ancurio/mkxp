/*
** graphics.h
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

#ifndef GRAPHICS_H
#define GRAPHICS_H

#include "util.h"

class Scene;
class Bitmap;
struct RGSSThreadData;
struct GraphicsPrivate;

class Graphics
{
public:
	Graphics(RGSSThreadData *data);
	~Graphics();

	void update();
	void freeze();
	void transition(int duration = 8,
	                const char *filename = 0,
	                int vague = 40);

	void wait(int duration);
	void fadeout(int duration);
	void fadein(int duration);

	Bitmap *snapToBitmap();

	void frameReset();

	int width() const;
	int height() const;
	void resizeScreen(int width, int height);

	DECL_ATTR( FrameRate,  int )
	DECL_ATTR( FrameCount, int )
	DECL_ATTR( Brightness, int )

	/* Non-standard extension */
	DECL_ATTR( Fullscreen, bool )
	DECL_ATTR( ShowCursor, bool )

	/* <internal> */
	Scene *getScreen() const;
	/* Repaint screen with static image until exitCond
	 * turns true. Used in EThread::showMessageBox() */
	void repaintWait(volatile bool *exitCond);

private:
	GraphicsPrivate *p;
};

#endif // GRAPHICS_H
