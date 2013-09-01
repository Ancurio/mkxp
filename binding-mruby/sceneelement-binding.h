/*
** sceneelement-binding.h
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

#ifndef SCENEELEMENTBINDING_H
#define SCENEELEMENTBINDING_H

#include "scene.h"
#include "binding-util.h"

template<class C>
MRB_METHOD(sceneElementGetZ)
{
	SceneElement *se = getPrivateData<C>(mrb, self);

	mrb_int value = 0;
	GUARD_EXC( value = se->getZ(); )

	return mrb_fixnum_value(value);
}

template<class C>
MRB_METHOD(sceneElementSetZ)
{
	SceneElement *se = getPrivateData<C>(mrb, self);

	mrb_int z;
	mrb_get_args(mrb, "i", &z);

	GUARD_EXC( se->setZ(z); )

	return mrb_nil_value();
}

template<class C>
MRB_METHOD(sceneElementGetVisible)
{
	SceneElement *se = getPrivateData<C>(mrb, self);

	bool value = false;
	GUARD_EXC( value = se->getVisible(); )

	return mrb_bool_value(value);
}

template<class C>
MRB_METHOD(sceneElementSetVisible)
{
	SceneElement *se = getPrivateData<C>(mrb, self);

	mrb_value visibleObj;
	bool visible;

	mrb_get_args(mrb, "o", &visibleObj);

	visible = mrb_test(visibleObj);

	GUARD_EXC( se->setVisible(visible); )

	return mrb_nil_value();
}

template<class C>
void
sceneElementBindingInit(mrb_state *mrb, RClass *klass)
{
	mrb_define_method(mrb, klass, "z",        sceneElementGetZ<C>,       MRB_ARGS_NONE());
	mrb_define_method(mrb, klass, "z=",       sceneElementSetZ<C>,       MRB_ARGS_REQ(1));
	mrb_define_method(mrb, klass, "visible",  sceneElementGetVisible<C>, MRB_ARGS_NONE());
	mrb_define_method(mrb, klass, "visible=", sceneElementSetVisible<C>, MRB_ARGS_REQ(1));
}

#endif // SCENEELEMENTBINDING_H
