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
class Disposable;
struct RGSSThreadData;
struct GraphicsPrivate;
struct AtomicFlag;
struct THEORAPLAY_VideoFrame;
struct Movie;

class Graphics
{
public:
    unsigned long long getDelta();
    unsigned long long lastUpdate();
    
	void update(bool checkForShutdown = true);
	void freeze();
	void transition(int duration = 8,
	                const char *filename = "",
	                int vague = 40);
	void frameReset();

	DECL_ATTR( FrameRate,  int )
	DECL_ATTR( FrameCount, int )
	DECL_ATTR( Brightness, int )

	void wait(int duration);
	void fadeout(int duration);
	void fadein(int duration);

	Bitmap *snapToBitmap();

	int width() const;
	int height() const;
	void resizeScreen(int width, int height);
    void resizeWindow(int width, int height, bool center=false);
	void drawMovieFrame(const THEORAPLAY_VideoFrame* video, Bitmap *videoBitmap);
	bool updateMovieInput(Movie *movie);
	void playMovie(const char *filename, int volume, bool skippable);
	void screenshot(const char *filename);

	void reset();
    void center();

    /* Non-standard extension */
    DECL_ATTR( Fullscreen, bool )
    DECL_ATTR( ShowCursor, bool )
    DECL_ATTR( Scale,    double )
    DECL_ATTR( Frameskip, bool )
    DECL_ATTR( FixedAspectRatio, bool )
    DECL_ATTR( SmoothScaling, bool )
    DECL_ATTR( IntegerScaling, bool )
    DECL_ATTR( LastMileScaling, bool )
    DECL_ATTR( Threadsafe, bool )
    double averageFrameRate();

	/* <internal> */
	Scene *getScreen() const;
	/* Repaint screen with static image until exitCond
	 * is set. Observes reset flag on top of shutdown
	 * if "checkReset" */
	void repaintWait(const AtomicFlag &exitCond,
	                 bool checkReset = true);
    
    void lock(bool force = false);
    void unlock(bool force = false);

private:
	Graphics(RGSSThreadData *data);
	~Graphics();

	void addDisposable(Disposable *);
	void remDisposable(Disposable *);

	friend struct SharedStatePrivate;
	friend class Disposable;

	GraphicsPrivate *p;
};

#define GFX_LOCK shState->graphics().lock()
#define GFX_UNLOCK shState->graphics().unlock()

#endif // GRAPHICS_H
