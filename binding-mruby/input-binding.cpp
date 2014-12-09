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
#include "util.h"

#include <mruby/hash.h>
#include <string.h>

MRB_FUNCTION(inputUpdate)
{
	MRB_FUN_UNUSED_PARAM;

	shState->input().update();

	return mrb_nil_value();
}

static mrb_int getButtonArg(mrb_state *mrb)
{
	mrb_int num;
	mrb_value arg;

	mrb_get_args(mrb, "o", &arg);

	if (mrb_fixnum_p(arg))
	{
		num = mrb_fixnum(arg);
	}
	else if (mrb_symbol_p(arg) && rgssVer >= 3)
	{
		mrb_value symHash = getMrbData(mrb)->buttoncodeHash;
		mrb_value numVal = mrb_hash_fetch(mrb, symHash, arg,
										  mrb_fixnum_value(Input::None));
		num = mrb_fixnum(numVal);
	}
	else
	{
		// FIXME: RMXP allows only few more types that
		// don't make sense (symbols in pre 3, floats)
		num = 0;
	}

	return num;
}

MRB_METHOD(inputPress)
{
	MRB_UNUSED_PARAM;

	mrb_int num = getButtonArg(mrb);

	return mrb_bool_value(shState->input().isPressed(num));
}

MRB_METHOD(inputTrigger)
{
	MRB_UNUSED_PARAM;

	mrb_int num = getButtonArg(mrb);

	return mrb_bool_value(shState->input().isTriggered(num));
}

MRB_METHOD(inputRepeat)
{
	MRB_UNUSED_PARAM;

	mrb_int num = getButtonArg(mrb);

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

struct
{
	const char *str;
	Input::ButtonCode val;
}
static buttonCodes[] =
{
	{ "DOWN",  Input::Down  },
	{ "LEFT",  Input::Left  },
	{ "RIGHT", Input::Right },
	{ "UP",    Input::Up    },

	{ "A",     Input::A     },
	{ "B",     Input::B     },
	{ "C",     Input::C     },
	{ "X",     Input::X     },
	{ "Y",     Input::Y     },
	{ "Z",     Input::Z     },
	{ "L",     Input::L     },
	{ "R",     Input::R     },

	{ "SHIFT", Input::Shift },
	{ "CTRL",  Input::Ctrl  },
	{ "ALT",   Input::Alt   },

	{ "F5",    Input::F5    },
	{ "F6",    Input::F6    },
	{ "F7",    Input::F7    },
	{ "F8",    Input::F8    },
	{ "F9",    Input::F9    },

	{ "MOUSELEFT",   Input::MouseLeft   },
	{ "MOUSEMIDDLE", Input::MouseMiddle },
	{ "MOUSERIGHT",  Input::MouseRight  }
};

static elementsN(buttonCodes);

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

	mrb_define_module_function(mrb, module, "mouse_x", inputMouseX, MRB_ARGS_NONE());
	mrb_define_module_function(mrb, module, "mouse_y", inputMouseY, MRB_ARGS_NONE());

	mrb_value modVal = mrb_obj_value(module);

	if (rgssVer >= 3)
	{
		mrb_value symHash = mrb_hash_new_capa(mrb, buttonCodesN);

		for (size_t i = 0; i < buttonCodesN; ++i)
		{
			const char *str = buttonCodes[i].str;
			mrb_sym sym = mrb_intern_static(mrb, str, strlen(str));
			mrb_value symVal = mrb_symbol_value(sym);
			mrb_value val = mrb_fixnum_value(buttonCodes[i].val);

			/* In RGSS3 all Input::XYZ constants are equal to :XYZ symbols,
			 * to be compatible with the previous convention */
			mrb_const_set(mrb, modVal, sym, symVal);
			mrb_hash_set(mrb, symHash, symVal, val);
		}

		mrb_iv_set(mrb, modVal, mrb_intern_lit(mrb, "buttoncodes"), symHash);
		getMrbData(mrb)->buttoncodeHash = symHash;
	}
	else
	{
		for (size_t i = 0; i < buttonCodesN; ++i)
		{
			const char *str = buttonCodes[i].str;
			mrb_sym sym = mrb_intern_static(mrb, str, strlen(str));
			mrb_value val = mrb_fixnum_value(buttonCodes[i].val);

			mrb_const_set(mrb, modVal, sym, val);
		}
	}
}
