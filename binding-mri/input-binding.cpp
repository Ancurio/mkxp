/*
** input-binding.cpp
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
#include "globalstate.h"
#include "exception.h"
#include "binding-util.h"

RB_METHOD(inputUpdate)
{
	RB_UNUSED_PARAM;

	gState->input().update();

	return Qnil;
}

RB_METHOD(inputPress)
{
	RB_UNUSED_PARAM;

	int num;
	rb_get_args(argc, argv, "i", &num);

	Input::ButtonCode bc = (Input::ButtonCode) num;

	return rb_bool_new(gState->input().isPressed(bc));
}

RB_METHOD(inputTrigger)
{
	RB_UNUSED_PARAM;

	int num;
	rb_get_args(argc, argv, "i", &num);

	Input::ButtonCode bc = (Input::ButtonCode) num;

	return rb_bool_new(gState->input().isTriggered(bc));
}

RB_METHOD(inputRepeat)
{
	RB_UNUSED_PARAM;

	int num;
	rb_get_args(argc, argv, "i", &num);

	Input::ButtonCode bc = (Input::ButtonCode) num;

	return rb_bool_new(gState->input().isRepeated(bc));
}

RB_METHOD(inputDir4)
{
	RB_UNUSED_PARAM;

	return rb_fix_new(gState->input().dir4Value());
}

RB_METHOD(inputDir8)
{
	RB_UNUSED_PARAM;

	return rb_fix_new(gState->input().dir8Value());
}

/* Non-standard extensions */
RB_METHOD(inputMouseX)
{
	RB_UNUSED_PARAM;

	return rb_fix_new(gState->input().mouseX());
}

RB_METHOD(inputMouseY)
{
	RB_UNUSED_PARAM;

	return rb_fix_new(gState->input().mouseY());
}

#define DEF_CONST_I(name, value) \
	rb_const_set(module, rb_intern(name), rb_fix_new(value))

void
inputBindingInit()
{
	VALUE module = rb_define_module("Input");

	_rb_define_module_function(module, "update", inputUpdate);
	_rb_define_module_function(module, "press?", inputPress);
	_rb_define_module_function(module, "trigger?", inputTrigger);
	_rb_define_module_function(module, "repeat?", inputRepeat);
	_rb_define_module_function(module, "dir4", inputDir4);
	_rb_define_module_function(module, "dir8", inputDir8);

	DEF_CONST_I("DOWN",  Input::Down );
	DEF_CONST_I("LEFT",  Input::Left );
	DEF_CONST_I("RIGHT", Input::Right);
	DEF_CONST_I("UP",    Input::Up   );

	DEF_CONST_I("A",     Input::A    );
	DEF_CONST_I("B",     Input::B    );
	DEF_CONST_I("C",     Input::C    );
	DEF_CONST_I("X",     Input::X    );
	DEF_CONST_I("Y",     Input::Y    );
	DEF_CONST_I("Z",     Input::Z    );
	DEF_CONST_I("L",     Input::L    );
	DEF_CONST_I("R",     Input::R    );

	DEF_CONST_I("SHIFT", Input::Shift);
	DEF_CONST_I("CTRL",  Input::Ctrl );
	DEF_CONST_I("ALT",   Input::Alt  );

	DEF_CONST_I("F5",    Input::F5   );
	DEF_CONST_I("F6",    Input::F6   );
	DEF_CONST_I("F7",    Input::F7   );
	DEF_CONST_I("F8",    Input::F8   );
	DEF_CONST_I("F9",    Input::F9   );

	_rb_define_module_function(module, "mouse_x", inputMouseX);
	_rb_define_module_function(module, "mouse_y", inputMouseY);

	DEF_CONST_I("MOUSELEFT",   Input::MouseLeft  );
	DEF_CONST_I("MOUSEMIDDLE", Input::MouseMiddle);
	DEF_CONST_I("MOUSERIGHT",  Input::MouseRight );
}
