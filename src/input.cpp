/*
** input.cpp
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

#include "input.h"
#include "sharedstate.h"
#include "eventthread.h"
#include "exception.h"
#include "util.h"

#include "SDL2/SDL_scancode.h"
#include "SDL2/SDL_mouse.h"

#include <QVector>
#include "string.h"

#include <QDebug>

const int Input::buttonCodeSize = 24;

struct ButtonState
{
	bool pressed;
	bool triggered;
	bool repeated;

	ButtonState()
		: pressed(false),
		  triggered(false),
		  repeated(false)
	{}
};

struct KbBindingData
{
	SDL_Scancode source;
	Input::ButtonCode target;
};

struct JsBindingData
{
	int source;
	Input::ButtonCode target;
};

struct Binding
{
	Binding(Input::ButtonCode target = Input::None)
		: target(target)
	{}

	virtual bool sourceActive() const = 0;
	virtual bool sourceRepeatable() const = 0;

	Input::ButtonCode target;
};

/* Keyboard binding */
struct KbBinding : public Binding
{
	KbBinding() {}

	KbBinding(const KbBindingData &data)
		: Binding(data.target),
		  source(data.source)
	{}

	bool sourceActive() const
	{
		return EventThread::keyStates[source];
	}

	bool sourceRepeatable() const
	{
		return (source >= SDL_SCANCODE_A     && source <= SDL_SCANCODE_0)    ||
		       (source >= SDL_SCANCODE_RIGHT && source <= SDL_SCANCODE_UP)   ||
		       (source >= SDL_SCANCODE_F1    && source <= SDL_SCANCODE_F12);
	}

	SDL_Scancode source;
};

/* Joystick button binding */
struct JsButtonBinding : public Binding
{
	JsButtonBinding() {}

	JsButtonBinding(const JsBindingData &data)
		: Binding(data.target),
		  source(data.source)
	{}

	bool sourceActive() const
	{
		return EventThread::joyState.buttons[source];
	}

	bool sourceRepeatable() const
	{
		return true;
	}

	int source;
};

/* Joystick axis binding */
struct JsAxisBinding : public Binding
{
	JsAxisBinding() {}

	JsAxisBinding(int *source,
	              int compareValue,
                  Input::ButtonCode target)
	    : Binding(target),
	      source(source),
	      compareValue(compareValue)
	{}

	bool sourceActive() const
	{
		return (*source == compareValue);
	}

	bool sourceRepeatable() const
	{
		return true;
	}

	int *source;
	int compareValue;
};

/* Mouse button binding */
struct MsBinding : public Binding
{
	MsBinding() {}

	MsBinding(int buttonIndex,
	          Input::ButtonCode target)
	    : Binding(target),
	      index(buttonIndex)
	{}

	bool sourceActive() const
	{
		return EventThread::mouseState.buttons[index];
	}

	bool sourceRepeatable() const
	{
		return false;
	}

	int index;
};

/* Not rebindable */
static const KbBindingData staticKbBindings[] =
{
	{ SDL_SCANCODE_LEFT,   Input::Left  },
	{ SDL_SCANCODE_RIGHT,  Input::Right },
	{ SDL_SCANCODE_UP,     Input::Up    },
	{ SDL_SCANCODE_DOWN,   Input::Down  },
	{ SDL_SCANCODE_LSHIFT, Input::Shift },
	{ SDL_SCANCODE_RSHIFT, Input::Shift },
	{ SDL_SCANCODE_LCTRL,  Input::Ctrl  },
	{ SDL_SCANCODE_RCTRL,  Input::Ctrl  },
	{ SDL_SCANCODE_LALT,   Input::Alt   },
	{ SDL_SCANCODE_RALT,   Input::Alt   },
	{ SDL_SCANCODE_F5,     Input::F5    },
	{ SDL_SCANCODE_F6,     Input::F6    },
	{ SDL_SCANCODE_F7,     Input::F7    },
	{ SDL_SCANCODE_F8,     Input::F8    },
	{ SDL_SCANCODE_F9,     Input::F9    }
};

static elementsN(staticKbBindings);

/* Rebindable */
static const KbBindingData defaultKbBindings[] =
{
	{ SDL_SCANCODE_SPACE,  Input::C    },
	{ SDL_SCANCODE_RETURN, Input::C    },
	{ SDL_SCANCODE_ESCAPE, Input::B    },
	{ SDL_SCANCODE_KP_0,   Input::B    },
	{ SDL_SCANCODE_LSHIFT, Input::A    },
	{ SDL_SCANCODE_RSHIFT, Input::A    },
	{ SDL_SCANCODE_Z,      Input::A    },
	{ SDL_SCANCODE_X,      Input::B    },
	{ SDL_SCANCODE_C,      Input::C    },
	{ SDL_SCANCODE_V,      Input::None },
	{ SDL_SCANCODE_B,      Input::None },
	{ SDL_SCANCODE_S,      Input::X    },
	{ SDL_SCANCODE_A,      Input::Y    },
	{ SDL_SCANCODE_D,      Input::Z    },
	{ SDL_SCANCODE_Q,      Input::L    },
	{ SDL_SCANCODE_W,      Input::R    }
};

static elementsN(defaultKbBindings);

/* Rebindable */
static const JsBindingData defaultJsBindings[] =
{
	{ 0, Input::A    },
	{ 1, Input::B    },
	{ 2, Input::C    },
	{ 3, Input::X    },
	{ 4, Input::Y    },
	{ 5, Input::Z    },
	{ 6, Input::L    },
	{ 7, Input::R    },
	{ 8, Input::None },
	{ 9, Input::None }
};

static elementsN(defaultJsBindings);

/* Maps ButtonCode enum values to indices
 * in the button state array */
static const int mapToIndex[] =
{
	0, 0,
	1, 0, 2, 0, 3, 0, 4, 0,
	0,
	5, 6, 7, 8, 9, 10, 11, 12,
	0, 0,
	13, 14, 15,
	0,
	16, 17, 18, 19, 20,
	0, 0, 0, 0, 0, 0, 0, 0,
	21, 22, 23
};

static elementsN(mapToIndex);

static const Input::ButtonCode dirs[] =
{ Input::Down, Input::Left, Input::Right, Input::Up };

static const int dirFlags[] =
{
	1 << Input::Down,
	1 << Input::Left,
	1 << Input::Right,
	1 << Input::Up
};

/* Dir4 is always zero on these combinations */
static const int deadDirFlags[] =
{
	dirFlags[0] | dirFlags[3],
	dirFlags[1] | dirFlags[2]
};

static const Input::ButtonCode otherDirs[4][3] =
{
	{ Input::Left, Input::Right, Input::Up    }, /* Down  */
	{ Input::Down, Input::Up,    Input::Right }, /* Left  */
	{ Input::Down, Input::Up,    Input::Left  }, /* Right */
	{ Input::Left, Input::Right, Input::Up    }  /* Up    */
};

struct InputPrivate
{
	QVector<KbBinding> kbBindings;
	QVector<JsAxisBinding> jsABindings;
	QVector<JsButtonBinding> jsBBindings;
	QVector<MsBinding> msBindings;

	/* Collective binding array */
	QVector<Binding*> bindings;

	ButtonState stateArray[Input::buttonCodeSize*2];

	ButtonState *states;
	ButtonState *statesOld;

	Input::ButtonCode repeating;
	unsigned int repeatCount;

	struct
	{
		int active;
		Input::ButtonCode previous;
	} dir4Data;

	struct
	{
		int active;
	} dir8Data;


	InputPrivate()
	{
		initKbBindings();
		initJsBindings();
		initMsBindings();

		states    = stateArray;
		statesOld = stateArray + Input::buttonCodeSize;

		/* Clear buffers */
		clearBuffer();
		swapBuffers();
		clearBuffer();

		repeating = Input::None;
		repeatCount = 0;

		dir4Data.active = 0;
		dir4Data.previous = Input::None;

		dir8Data.active = 0;
	}

	inline ButtonState &getStateCheck(int code)
	{
		int index;

		if (code < 0 || code > mapToIndexN-1)
			index = 0;
		else
			index = mapToIndex[code];

		return states[index];
	}

	inline ButtonState &getState(Input::ButtonCode code)
	{
		return states[mapToIndex[code]];
	}

	inline ButtonState &getOldState(Input::ButtonCode code)
	{
		return statesOld[mapToIndex[code]];
	}

	void swapBuffers()
	{
		ButtonState *tmp = states;
		states = statesOld;
		statesOld = tmp;
	}

	void clearBuffer()
	{
		const size_t size = sizeof(ButtonState) * Input::buttonCodeSize;
		memset(states, 0, size);
	}

	void initKbBindings()
	{
		kbBindings.resize(staticKbBindingsN+defaultKbBindingsN);

		int n = 0;
		for (int i = 0; i < staticKbBindingsN; ++i)
			kbBindings[n++] = KbBinding(staticKbBindings[i]);

		for (int i = 0; i < defaultKbBindingsN; ++i)
			kbBindings[n++] = KbBinding(defaultKbBindings[i]);

		/* Add to binging array */
		for (int i = 0; i < kbBindings.count(); ++i)
			bindings.append(&kbBindings[i]);
	}

	void initJsBindings()
	{
		/* Create axis bindings */
		jsABindings.resize(4);

		int i = 0;
		jsABindings[i++] = JsAxisBinding(&EventThread::joyState.xAxis,  0x7FFF, Input::Right);
		jsABindings[i++] = JsAxisBinding(&EventThread::joyState.xAxis, -0x8000, Input::Left);
		jsABindings[i++] = JsAxisBinding(&EventThread::joyState.yAxis,  0x7FFF, Input::Down);
		jsABindings[i++] = JsAxisBinding(&EventThread::joyState.yAxis, -0x8000, Input::Up);

		/* Create button bindings */
		jsBBindings.resize(defaultJsBindingsN);

		for (int i = 0; i < defaultJsBindingsN; ++i)
			jsBBindings[i] = JsButtonBinding(defaultJsBindings[i]);

		/* Add to binging array */
		for (int i = 0; i < jsABindings.count(); ++i)
			bindings.append(&jsABindings[i]);

		for (int i = 0; i < jsBBindings.count(); ++i)
			bindings.append(&jsBBindings[i]);
	}

	void initMsBindings()
	{
		msBindings.resize(3);

		int i = 0;
		msBindings[i++] = MsBinding(SDL_BUTTON_LEFT,   Input::MouseLeft);
		msBindings[i++] = MsBinding(SDL_BUTTON_MIDDLE, Input::MouseMiddle);
		msBindings[i++] = MsBinding(SDL_BUTTON_RIGHT,  Input::MouseRight);

		/* Add to binding array */
		for (int i = 0; i < msBindings.count(); ++i)
			bindings.append(&msBindings[i]);
	}

	void pollBindings(Input::ButtonCode &repeatCand)
	{
		for (int i = 0; i < bindings.count(); ++i)
			pollBindingPriv(*bindings[i], repeatCand);

		updateDir4();
		updateDir8();
	}

	void pollBindingPriv(const Binding &b,
	                     Input::ButtonCode &repeatCand)
	{	
		if (!b.sourceActive())
			return;

		if (b.target == Input::None)
			return;

		ButtonState &state = getState(b.target);
		ButtonState &oldState = getOldState(b.target);

		state.pressed = true;

		/* Must have been released before to trigger */
		if (!oldState.pressed)
			state.triggered = true;

		/* Unbound keys don't create/break repeat */
		if (repeatCand != Input::None)
			return;

		if (repeating != b.target &&
			!oldState.pressed)
		{
			if (b.sourceRepeatable())
				repeatCand = b.target;
			else
				/* Unrepeatable keys still break current repeat */
				repeating = Input::None;
		}
	}

	void updateDir4()
	{
		int dirFlag = 0;

		for (int i = 0; i < 4; ++i)
			dirFlag |= (getState(dirs[i]).pressed ? dirFlags[i] : 0);

		if (dirFlag == deadDirFlags[0] || dirFlag == deadDirFlags[1])
		{
			dir4Data.active = Input::None;
			return;
		}

		if (dir4Data.previous != Input::None)
		{
			/* Check if prev still pressed */
			if (getState(dir4Data.previous).pressed)
			{
				for (int i = 0; i < 3; ++i)
				{
					Input::ButtonCode other =
							otherDirs[(dir4Data.previous/2)-1][i];

					if (!getState(other).pressed)
						continue;

					dir4Data.active = other;
					return;
				}
			}
		}

		for (int i = 0; i < 4; ++i)
		{
			if (!getState(dirs[i]).pressed)
				continue;

			dir4Data.active = dirs[i];
			dir4Data.previous = dirs[i];
			return;
		}

		dir4Data.active   = Input::None;
		dir4Data.previous = Input::None;
	}

	void updateDir8()
	{
		static const int combos[4][4] =
		{
			{ 2, 1, 3, 0 },
			{ 1, 4, 0, 7 },
			{ 3, 0, 6, 9 },
			{ 0, 7, 9, 8 }
		};

		dir8Data.active = 0;

		for (int i = 0; i < 4; ++i)
		{
			Input::ButtonCode one = dirs[i];

			if (!getState(one).pressed)
				continue;

			for (int j = 0; j < 3; ++j)
			{
				Input::ButtonCode other = otherDirs[i][j];

				if (!getState(other).pressed)
					continue;

				dir8Data.active = combos[(one/2)-1][(other/2)-1];
				return;
			}

			dir8Data.active = one;
			return;
		}
	}
};


Input::Input()
{
	p = new InputPrivate;
}

void Input::update()
{
	shState->checkShutdown();

	p->swapBuffers();
	p->clearBuffer();

	ButtonCode repeatCand = None;

	/* Poll all bindings */
	p->pollBindings(repeatCand);

	/* Check for new repeating key */
	if (repeatCand != None && repeatCand != p->repeating)
	{
		p->repeating = repeatCand;
		p->repeatCount = 0;
		p->getState(repeatCand).repeated = true;

		return;
	}

	/* Check if repeating key is still pressed */
	if (p->getState(p->repeating).pressed)
	{
		p->repeatCount++;

		/* Repeatsequence is [r...............(r...)+] */
		if (p->repeatCount > 15 && ((p->repeatCount % 4) == 0))
			p->getState(p->repeating).repeated = true;

		return;
	}

	p->repeating = None;
}

bool Input::isPressed(int button)
{
	return p->getStateCheck(button).pressed;
}

bool Input::isTriggered(int button)
{
	return p->getStateCheck(button).triggered;
}

bool Input::isRepeated(int button)
{
	return p->getStateCheck(button).repeated;
}

int Input::dir4Value()
{
	return p->dir4Data.active;
}

int Input::dir8Value()
{
	return p->dir8Data.active;
}

int Input::mouseX()
{
	RGSSThreadData &rtData = shState->rtData();
	return (EventThread::mouseState.x - rtData.screenOffset.x) * rtData.sizeResoRatio.x;
}

int Input::mouseY()
{
	RGSSThreadData &rtData = shState->rtData();
	return (EventThread::mouseState.y - rtData.screenOffset.y) * rtData.sizeResoRatio.y;
}

Input::~Input()
{
	delete p;
}
