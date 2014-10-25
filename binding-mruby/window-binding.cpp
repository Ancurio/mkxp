/*
** window-binding.cpp
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

#include "window.h"
#include "disposable-binding.h"
#include "viewportelement-binding.h"
#include "binding-util.h"

DEF_TYPE(Window);

MRB_METHOD(windowInitialize)
{
	Window *w = viewportElementInitialize<Window>(mrb, self);

	setPrivateData(self, w, WindowType);

	w->initDynAttribs();
	wrapProperty(mrb, self, &w->getCursorRect(), CScursor_rect, RectType);

	return self;
}

MRB_METHOD(windowUpdate)
{
	Window *w = getPrivateData<Window>(mrb, self);

	w->update();

	return mrb_nil_value();
}

DEF_PROP_OBJ_REF(Window, Bitmap, Windowskin, CSwindowskin)
DEF_PROP_OBJ_REF(Window, Bitmap, Contents,   CScontents)
DEF_PROP_OBJ_VAL(Window, Rect,   CursorRect, CScursor_rect)

DEF_PROP_B(Window, Stretch)
DEF_PROP_B(Window, Active)
DEF_PROP_B(Window, Pause)

DEF_PROP_I(Window, X)
DEF_PROP_I(Window, Y)
DEF_PROP_I(Window, Width)
DEF_PROP_I(Window, Height)
DEF_PROP_I(Window, OX)
DEF_PROP_I(Window, OY)
DEF_PROP_I(Window, Opacity)
DEF_PROP_I(Window, BackOpacity)
DEF_PROP_I(Window, ContentsOpacity)


void
windowBindingInit(mrb_state *mrb)
{
	RClass *klass = defineClass(mrb, "Window");

	disposableBindingInit     <Window>(mrb, klass);
	viewportElementBindingInit<Window>(mrb, klass);

	mrb_define_method(mrb, klass, "initialize", windowInitialize, MRB_ARGS_REQ(1));
	mrb_define_method(mrb, klass, "update",     windowUpdate,     MRB_ARGS_NONE());

	INIT_PROP_BIND( Window, Windowskin,      "windowskin"       );
	INIT_PROP_BIND( Window, Contents,        "contents"         );
	INIT_PROP_BIND( Window, Stretch,         "stretch"          );
	INIT_PROP_BIND( Window, CursorRect,      "cursor_rect"      );
	INIT_PROP_BIND( Window, Active,          "active"           );
	INIT_PROP_BIND( Window, Pause,           "pause"            );
	INIT_PROP_BIND( Window, X,               "x"                );
	INIT_PROP_BIND( Window, Y,               "y"                );
	INIT_PROP_BIND( Window, Width,           "width"            );
	INIT_PROP_BIND( Window, Height,          "height"           );
	INIT_PROP_BIND( Window, OX,              "ox"               );
	INIT_PROP_BIND( Window, OY,              "oy"               );
	INIT_PROP_BIND( Window, Opacity,         "opacity"          );
	INIT_PROP_BIND( Window, BackOpacity,     "back_opacity"     );
	INIT_PROP_BIND( Window, ContentsOpacity, "contents_opacity" );

	mrb_define_method(mrb, klass, "inspect", inspectObject, MRB_ARGS_NONE());
}
