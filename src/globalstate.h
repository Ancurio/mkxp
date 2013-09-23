/*
** globalstate.h
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

#ifndef GLOBALSTATE_H
#define GLOBALSTATE_H

#include "sigc++/signal.h"

#define gState GlobalState::instance
#define glState gState->_glState()

struct GlobalStatePrivate;
struct RGSSThreadData;
struct GlobalIBO;
struct mrb_state;
struct SDL_Window;
struct TEXFBO;
struct Quad;

class Scene;
class FileSystem;
class EventThread;
class Graphics;
class Input;
class Audio;
class GLState;
class SimpleShader;
class SimpleColorShader;
class SimpleAlphaShader;
class SimpleSpriteShader;
class SpriteShader;
class PlaneShader;
class TransShader;
class SimpleTransShader;
class HueShader;
class BltShader;
class TexPool;
class FontPool;
class Font;
struct GlobalIBO;
struct Config;
struct Vec2i;

struct GlobalState
{
	void *bindingData();
	void setBindingData(void *data);

	SDL_Window *sdlWindow();

	Scene *screen();
	void setScreen(Scene &screen);

	FileSystem &fileSystem();

	EventThread &eThread();
	RGSSThreadData &rtData();
	Config &config();

	Graphics &graphics();
	Input &input();
	Audio &audio();

	GLState &_glState();

	SimpleShader &simpleShader();
	SimpleColorShader &simpleColorShader();
	SimpleAlphaShader &simpleAlphaShader();
	SimpleSpriteShader &simpleSpriteShader();
	SpriteShader &spriteShader();
	PlaneShader &planeShader();
	TransShader &transShader();
	SimpleTransShader &sTransShader();
	HueShader &hueShader();
	BltShader &bltShader();

	TexPool &texPool();
	FontPool &fontPool();

	Font &defaultFont();

	sigc::signal<void> prepareDraw;

	unsigned int genTimeStamp();

	/* Returns global quad IBO, and ensures it has indices
	 * for at least minSize quads */
	void ensureQuadIBO(int minSize);
	void bindQuadIBO();

	/* Global general purpose texture */
	void bindTex();
	void ensureTexSize(int minW, int minH, Vec2i &currentSizeOut);

	TEXFBO &gpTexFBO(int minW, int minH);

	Quad &gpQuad();

	/* Checks EventThread's shutdown request flag and if set,
	 * requests the binding to terminate. In this case, this
	 * function will most likely not return */
	void checkShutdown();

	static GlobalState *instance;
	static void initInstance(RGSSThreadData *threadData);
	static void finiInstance();

private:
	GlobalState(RGSSThreadData *threadData);
	~GlobalState();

	GlobalStatePrivate *p;
};

#endif // GLOBALSTATE_H
