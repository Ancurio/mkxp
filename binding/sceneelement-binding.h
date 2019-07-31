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
RB_METHOD(sceneElementGetZ)
{
	RB_UNUSED_PARAM;

	SceneElement *se = getPrivateData<C>(self);

	int value = 0;
	GUARD_EXC( value = se->getZ(); );

	return rb_fix_new(value);
}

template<class C>
RB_METHOD(sceneElementSetZ)
{
	SceneElement *se = getPrivateData<C>(self);

	int z;
	rb_get_args(argc, argv, "i", &z RB_ARG_END);

	GUARD_EXC( se->setZ(z); );

	return rb_fix_new(z);
}

template<class C>
RB_METHOD(sceneElementGetVisible)
{
	RB_UNUSED_PARAM;

	SceneElement *se = getPrivateData<C>(self);

	bool value = false;
	GUARD_EXC( value = se->getVisible(); );

	return rb_bool_new(value);
}

template<class C>
RB_METHOD(sceneElementSetVisible)
{
	SceneElement *se = getPrivateData<C>(self);

	bool visible;
	rb_get_args(argc, argv, "b", &visible RB_ARG_END);

	GUARD_EXC( se->setVisible(visible); );

    return rb_bool_new(visible);
}

template<class C>
void
sceneElementBindingInit(VALUE klass)
{
	_rb_define_method(klass, "z",        sceneElementGetZ<C>);
	_rb_define_method(klass, "z=",       sceneElementSetZ<C>);
	_rb_define_method(klass, "visible",  sceneElementGetVisible<C>);
	_rb_define_method(klass, "visible=", sceneElementSetVisible<C>);
}

#endif // SCENEELEMENTBINDING_H
