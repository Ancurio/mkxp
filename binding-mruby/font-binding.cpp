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
#include "sharedstate.h"
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
	f->initDynAttribs();

	wrapProperty(mrb, self, &f->getColor(), CScolor, ColorType);

	if (rgssVer >= 3)
		wrapProperty(mrb, self, &f->getOutColor(), CSout_color, ColorType);

	return self;
}

MRB_METHOD(fontInitializeCopy)
{
	mrb_value origObj;
	mrb_get_args(mrb, "o", &origObj);

	Font *orig = getPrivateData<Font>(mrb, origObj);
	Font *f = new Font(*orig);
	setPrivateData(self, f, FontType);

	/* Wrap property objects */
	f->initDynAttribs();

	wrapProperty(mrb, self, &f->getColor(), CScolor, ColorType);

	if (rgssVer >= 3)
		wrapProperty(mrb, self, &f->getOutColor(), CSout_color, ColorType);

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

template<class C>
static void checkDisposed(mrb_state *, mrb_value) {}

DEF_PROP_I(Font, Size)
DEF_PROP_B(Font, Bold)
DEF_PROP_B(Font, Italic)
DEF_PROP_B(Font, Outline)
DEF_PROP_B(Font, Shadow)
DEF_PROP_OBJ_VAL(Font, Color, Color,    CScolor)
DEF_PROP_OBJ_VAL(Font, Color, OutColor, CSout_color)

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

DEF_KLASS_PROP(Font, mrb_int,  DefaultSize,    "i", fixnum)
DEF_KLASS_PROP(Font, mrb_bool, DefaultBold,    "b", bool)
DEF_KLASS_PROP(Font, mrb_bool, DefaultItalic,  "b", bool)
DEF_KLASS_PROP(Font, mrb_bool, DefaultOutline, "b", bool)
DEF_KLASS_PROP(Font, mrb_bool, DefaultShadow,  "b", bool)

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
	MRB_UNUSED_PARAM;

	mrb_value colorObj;
	mrb_get_args(mrb, "o", &colorObj);

	Color *c = getPrivateDataCheck<Color>(mrb, colorObj, ColorType);

	Font::setDefaultColor(*c);

	return colorObj;
}

MRB_METHOD(FontGetDefaultOutColor)
{
	return getProperty(mrb, self, CSdefault_out_color);
}

MRB_METHOD(FontSetDefaultOutColor)
{
	MRB_UNUSED_PARAM;

	mrb_value colorObj;
	mrb_get_args(mrb, "o", &colorObj);

	Color *c = getPrivateDataCheck<Color>(mrb, colorObj, ColorType);

	Font::setDefaultOutColor(*c);

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

	Font::initDefaultDynAttribs();
	wrapProperty(mrb, mrb_obj_value(klass), &Font::getDefaultColor(), CSdefault_color, ColorType);

	mrb_define_class_method(mrb, klass, "exist?", fontDoesExist, MRB_ARGS_REQ(1));

	INIT_KLASS_PROP_BIND(Font, DefaultName,     "default_name");
	INIT_KLASS_PROP_BIND(Font, DefaultSize,     "default_size");
	INIT_KLASS_PROP_BIND(Font, DefaultBold,     "default_bold");
	INIT_KLASS_PROP_BIND(Font, DefaultItalic,   "default_italic");
	INIT_KLASS_PROP_BIND(Font, DefaultColor,    "default_color");

	if (rgssVer >= 2)
	{
	INIT_KLASS_PROP_BIND(Font, DefaultShadow,   "default_shadow");
	}

	if (rgssVer >= 3)
	{
	INIT_KLASS_PROP_BIND(Font, DefaultOutline,  "default_outline");
	INIT_KLASS_PROP_BIND(Font, DefaultOutColor, "default_out_color");
	wrapProperty(mrb, mrb_obj_value(klass), &Font::getDefaultOutColor(), CSdefault_out_color, ColorType);
	}

	mrb_define_method(mrb, klass, "initialize",      fontInitialize,     MRB_ARGS_OPT(2));
	mrb_define_method(mrb, klass, "initialize_copy", fontInitializeCopy, MRB_ARGS_REQ(1));

	INIT_PROP_BIND(Font, Name,     "name");
	INIT_PROP_BIND(Font, Size,     "size");
	INIT_PROP_BIND(Font, Bold,     "bold");
	INIT_PROP_BIND(Font, Italic,   "italic");
	INIT_PROP_BIND(Font, Color,    "color");

	if (rgssVer >= 2)
	{
	INIT_PROP_BIND(Font, Shadow,   "shadow");
	}

	if (rgssVer >= 3)
	{
	INIT_PROP_BIND(Font, Outline,  "outline");
	INIT_PROP_BIND(Font, OutColor, "out_color");
	}

	mrb_define_method(mrb, klass, "inspect", inspectObject, MRB_ARGS_NONE());
}
