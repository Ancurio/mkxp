/*
** font-binding.cpp
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

#include "font.h"
#include "binding-util.h"
#include "binding-types.h"
#include "exception.h"

#include <mruby/string.h>

DEF_TYPE(Font);

MRB_FUNCTION(fontDoesExist)
{
	const char *name = 0;
	mrb_value nameObj;

	mrb_get_args(mrb, "o", &nameObj);

	if (mrb_string_p(nameObj))
		name = mrb_string_value_cstr(mrb, &nameObj);

	return mrb_bool_value(Font::doesExist(name));
}

MRB_METHOD(fontInitialize)
{
	char *name = 0;
	mrb_int size = 0;

	mrb_get_args(mrb, "|zi", &name, &size);

	Font *f = new Font(name, size);

	setPrivateData(self, f, FontType);

	/* Wrap property objects */
	f->setColor(new Color(*f->getColor()));
	wrapProperty(mrb, self, f->getColor(), CScolor, ColorType);

	return self;
}

MRB_METHOD(FontGetName)
{
	Font *f = getPrivateData<Font>(mrb, self);

	return mrb_str_new_cstr(mrb, f->getName());
}

MRB_METHOD(FontSetName)
{
	Font *f = getPrivateData<Font>(mrb, self);

	mrb_value name;
	mrb_get_args(mrb, "S", &name);

	f->setName(RSTRING_PTR(name));

	return name;
}

#undef DEF_PROP_CHK_DISP
#define DEF_PROP_CHK_DISP

DEF_PROP_I(Font, Size)
DEF_PROP_B(Font, Bold)
DEF_PROP_B(Font, Italic)
DEF_PROP_OBJ(Font, Color, Color, CScolor)

#define DEF_KLASS_PROP(Klass, mrb_type, PropName, arg_type, conv_t) \
	static mrb_value \
	Klass##Get##PropName(mrb_state *, mrb_value) \
	{ \
		return mrb_##conv_t##_value(Klass::get##PropName()); \
	} \
	static mrb_value \
	Klass##Set##PropName(mrb_state *mrb, mrb_value) \
	{ \
		mrb_type value; \
		mrb_get_args(mrb, arg_type, &value); \
		Klass::set##PropName(value); \
		return mrb_##conv_t##_value(value); \
	}

DEF_KLASS_PROP(Font, mrb_int, DefaultSize, "i", fixnum)
DEF_KLASS_PROP(Font, mrb_bool, DefaultBold, "b", bool)
DEF_KLASS_PROP(Font, mrb_bool, DefaultItalic, "b", bool)

MRB_FUNCTION(FontGetDefaultName)
{
	return mrb_str_new_cstr(mrb, Font::getDefaultName());
}

MRB_FUNCTION(FontSetDefaultName)
{
	mrb_value nameObj;
	mrb_get_args(mrb, "S", &nameObj);

	Font::setDefaultName(RSTRING_PTR(nameObj));

	return nameObj;
}

MRB_METHOD(FontGetDefaultColor)
{
	return getProperty(mrb, self, CSdefault_color);
}

MRB_METHOD(FontSetDefaultColor)
{
	mrb_value colorObj;
	mrb_get_args(mrb, "o", &colorObj);

	Color *c = getPrivateDataCheck<Color>(mrb, colorObj, ColorType);

	Font::setDefaultColor(c);
	setProperty(mrb, self, CSdefault_color, colorObj);

	return colorObj;
}

#define INIT_KLASS_PROP_BIND(Klass, PropName, prop_name_s) \
{ \
	mrb_define_class_method(mrb, klass, prop_name_s, Klass##Get##PropName, MRB_ARGS_NONE()); \
	mrb_define_class_method(mrb, klass, prop_name_s "=", Klass##Set##PropName, MRB_ARGS_REQ(1)); \
}

void
fontBindingInit(mrb_state *mrb)
{
	RClass *klass = defineClass(mrb, "Font");

	Font::setDefaultColor(new Color(*Font::getDefaultColor()));
	wrapProperty(mrb, mrb_obj_value(klass), Font::getDefaultColor(), CSdefault_color, ColorType);

	mrb_define_class_method(mrb, klass, "exist?", fontDoesExist, MRB_ARGS_REQ(1));

	INIT_KLASS_PROP_BIND(Font, DefaultName, "default_name");
	INIT_KLASS_PROP_BIND(Font, DefaultSize, "default_size");
	INIT_KLASS_PROP_BIND(Font, DefaultBold, "default_bold");
	INIT_KLASS_PROP_BIND(Font, DefaultItalic, "default_italic");
	INIT_KLASS_PROP_BIND(Font, DefaultColor, "default_color");

	mrb_define_method(mrb, klass, "initialize", fontInitialize, MRB_ARGS_OPT(2));

	INIT_PROP_BIND(Font, Name, "name");
	INIT_PROP_BIND(Font, Size, "size");
	INIT_PROP_BIND(Font, Bold, "bold");
	INIT_PROP_BIND(Font, Italic, "italic");
	INIT_PROP_BIND(Font, Color, "color");

	mrb_define_method(mrb, klass, "inspect", inspectObject, MRB_ARGS_NONE());
}
