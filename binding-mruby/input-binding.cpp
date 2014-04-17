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
#include "sharedstate.h"
#include "exception.h"
#include "binding-util.h"

MRB_FUNCTION(inputUpdate)
{
	MRB_FUN_UNUSED_PARAM;

	shState->input().update();

	return mrb_nil_value();
}

static mrb_int getButtonArg(mrb_state *mrb, mrb_value self)
{
	mrb_int num;

#ifdef RGSS3
	mrb_sym sym;
	mrb_get_args(mrb, "n", &sym);

	if (mrb_const_defined(mrb, self, sym))
		num = mrb_fixnum(mrb_const_get(mrb, self, sym));
	else
		num = 0;
#else
	mrb_get_args(mrb, "i", &num);
#endif

	return num;
}

MRB_METHOD(inputPress)
{
	mrb_int num = getButtonArg(mrb, self);

	return mrb_bool_value(shState->input().isPressed(num));
}

MRB_METHOD(inputTrigger)
{
	mrb_int num = getButtonArg(mrb, self);

	return mrb_bool_value(shState->input().isTriggered(num));
}

MRB_METHOD(inputRepeat)
{
	mrb_int num = getButtonArg(mrb, self);

	return mrb_bool_value(shState->input().isRepeated(num));
}

MRB_FUNCTION(inputDir4)
{
	MRB_FUN_UNUSED_PARAM;

	return mrb_fixnum_value(shState->input().dir4Value());
}

MRB_FUNCTION(inputDir8)
{
	MRB_FUN_UNUSED_PARAM;

	return mrb_fixnum_value(shState->input().dir8Value());
}

/* Non-standard extensions */
MRB_FUNCTION(inputMouseX)
{
	MRB_FUN_UNUSED_PARAM;

	return mrb_fixnum_value(shState->input().mouseX());
}

MRB_FUNCTION(inputMouseY)
{
	MRB_FUN_UNUSED_PARAM;

	return mrb_fixnum_value(shState->input().mouseY());
}

#define DEF_CONST_I(name, value) \
	mrb_const_set(mrb, mrb_obj_value(module), mrb_intern_lit(mrb, name), mrb_fixnum_value(value))

void
inputBindingInit(mrb_state *mrb)
{
	RClass *module = mrb_define_module(mrb, "Input");

	mrb_define_module_function(mrb, module, "update", inputUpdate, MRB_ARGS_NONE());
	mrb_define_module_function(mrb, module, "press?", inputPress, MRB_ARGS_REQ(1));
	mrb_define_module_function(mrb, module, "trigger?", inputTrigger, MRB_ARGS_REQ(1));
	mrb_define_module_function(mrb, module, "repeat?", inputRepeat, MRB_ARGS_REQ(1));
	mrb_define_module_function(mrb, module, "dir4", inputDir4, MRB_ARGS_NONE());
	mrb_define_module_function(mrb, module, "dir8", inputDir8, MRB_ARGS_NONE());

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

	mrb_define_module_function(mrb, module, "mouse_x", inputMouseX, MRB_ARGS_NONE());
	mrb_define_module_function(mrb, module, "mouse_y", inputMouseY, MRB_ARGS_NONE());

	DEF_CONST_I("MOUSELEFT",   Input::MouseLeft  );
	DEF_CONST_I("MOUSEMIDDLE", Input::MouseMiddle);
	DEF_CONST_I("MOUSERIGHT",  Input::MouseRight );
}
