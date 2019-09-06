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

RB_METHOD(inputUpdate)
{
	RB_UNUSED_PARAM;

	shState->input().update();

	return Qnil;
}

static int getButtonArg(int argc, VALUE *argv)
{
	int num;

	rb_check_argc(argc, 1);

	if (FIXNUM_P(argv[0]))
	{
		num = FIX2INT(argv[0]);
	}
	else if (SYMBOL_P(argv[0]) && rgssVer >= 3)
	{
		VALUE symHash = getRbData()->buttoncodeHash;
#ifndef OLD_RUBY
		num = FIX2INT(rb_hash_lookup2(symHash, argv[0], INT2FIX(Input::None)));
#else
        VALUE res = rb_hash_aref(symHash, argv[0]);
        if (!NIL_P(res))
            num = FIX2INT(res);
        else
            num = Input::None;
#endif
	}
	else
	{
		// FIXME: RMXP allows only few more types that
		// don't make sense (symbols in pre 3, floats)
		num = 0;
	}

	return num;
}

RB_METHOD(inputPress)
{
	RB_UNUSED_PARAM;

	int num = getButtonArg(argc, argv);

	return rb_bool_new(shState->input().isPressed(num));
}

RB_METHOD(inputTrigger)
{
	RB_UNUSED_PARAM;

	int num = getButtonArg(argc, argv);

	return rb_bool_new(shState->input().isTriggered(num));
}

RB_METHOD(inputRepeat)
{
	RB_UNUSED_PARAM;

	int num = getButtonArg(argc, argv);

	return rb_bool_new(shState->input().isRepeated(num));
}

RB_METHOD(inputPressEx)
{
    RB_UNUSED_PARAM;
    
    int num;
    rb_get_args(argc, argv, "i", &num RB_ARG_END);
    
    return rb_bool_new(shState->input().isPressedEx(num));
}

RB_METHOD(inputTriggerEx)
{
    RB_UNUSED_PARAM;
    
    int num;
    rb_get_args(argc, argv, "i", &num RB_ARG_END);
    
    return rb_bool_new(shState->input().isTriggeredEx(num));
}

RB_METHOD(inputRepeatEx)
{
    RB_UNUSED_PARAM;
    
    int num;
    rb_get_args(argc, argv, "i", &num RB_ARG_END);
    
    return rb_bool_new(shState->input().isRepeatedEx(num));
}

RB_METHOD(inputDir4)
{
	RB_UNUSED_PARAM;

	return rb_fix_new(shState->input().dir4Value());
}

RB_METHOD(inputDir8)
{
	RB_UNUSED_PARAM;

	return rb_fix_new(shState->input().dir8Value());
}

/* Non-standard extensions */
RB_METHOD(inputMouseX)
{
	RB_UNUSED_PARAM;

	return rb_fix_new(shState->input().mouseX());
}

RB_METHOD(inputMouseY)
{
	RB_UNUSED_PARAM;

	return rb_fix_new(shState->input().mouseY());
}

RB_METHOD(inputGetMode)
{
    RB_UNUSED_PARAM;
    
    return rb_bool_new(shState->input().getTextInputMode());
}

RB_METHOD(inputSetMode)
{
    RB_UNUSED_PARAM;
    
    bool mode;
    rb_get_args(argc, argv, "b", &mode RB_ARG_END);
    
    shState->input().setTextInputMode(mode);
    
    return mode;
}

RB_METHOD(inputGets)
{
    RB_UNUSED_PARAM;
    
    VALUE ret = rb_str_new_cstr(shState->input().getText());
    shState->input().clearText();
    
    return ret;
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
inputBindingInit()
{
	VALUE module = rb_define_module("Input");

	_rb_define_module_function(module, "update", inputUpdate);
	_rb_define_module_function(module, "press?", inputPress);
	_rb_define_module_function(module, "trigger?", inputTrigger);
	_rb_define_module_function(module, "repeat?", inputRepeat);
    _rb_define_module_function(module, "pressex?", inputPressEx);
    _rb_define_module_function(module, "triggerex?", inputTriggerEx);
    _rb_define_module_function(module, "repeatex?", inputRepeatEx);
	_rb_define_module_function(module, "dir4", inputDir4);
	_rb_define_module_function(module, "dir8", inputDir8);

	_rb_define_module_function(module, "mouse_x", inputMouseX);
	_rb_define_module_function(module, "mouse_y", inputMouseY);
    
    _rb_define_module_function(module, "text_input", inputGetMode);
    _rb_define_module_function(module, "text_input=", inputSetMode);
    _rb_define_module_function(module, "gets", inputGets);

	if (rgssVer >= 3)
	{
		VALUE symHash = rb_hash_new();

		for (size_t i = 0; i < buttonCodesN; ++i)
		{
			ID sym = rb_intern(buttonCodes[i].str);
			VALUE val = INT2FIX(buttonCodes[i].val);

			/* In RGSS3 all Input::XYZ constants are equal to :XYZ symbols,
			 * to be compatible with the previous convention */
			rb_const_set(module, sym, ID2SYM(sym));
			rb_hash_aset(symHash, ID2SYM(sym), val);
		}

		rb_iv_set(module, "buttoncodes", symHash);
		getRbData()->buttoncodeHash = symHash;
	}
	else
	{
		for (size_t i = 0; i < buttonCodesN; ++i)
		{
			ID sym = rb_intern(buttonCodes[i].str);
			VALUE val = INT2FIX(buttonCodes[i].val);

			rb_const_set(module, sym, val);
		}
	}
}
