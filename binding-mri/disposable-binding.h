/*
** disposable-binding.h
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

#ifndef DISPOSABLEBINDING_H
#define DISPOSABLEBINDING_H

#include "disposable.h"
#include "binding-util.h"

/* 'Children' are disposables that are disposed together
 * with their parent. Currently this is only used by Viewport
 * in RGSS1.
 * FIXME: Disable this behavior when RGSS2 or 3 */
inline void
disposableAddChild(VALUE disp, VALUE child)
{
	VALUE children = rb_iv_get(disp, "children");

	if (NIL_P(children))
	{
		children = rb_ary_new();
		rb_iv_set(disp, "children", children);
	}

	/* Assumes children are never removed until destruction */
	rb_ary_push(children, child);
}

inline void
disposableDisposeChildren(VALUE disp)
{
	VALUE children = rb_iv_get(disp, "children");

	if (NIL_P(children))
		return;

	ID dispFun = rb_intern("dispose");

	/* Note: RMXP doesn't call overridden 'dispose' methods here */
	for (long i = 0; i < RARRAY_LEN(children); ++i)
		rb_funcall2(rb_ary_entry(children, i), dispFun, 0, 0);
}

template<class C>
RB_METHOD(disposableDispose)
{
	RB_UNUSED_PARAM;

	C *c = static_cast<C*>(RTYPEDDATA_DATA(self));

	/* Nothing to do if already disposed */
	if (!c)
		return Qnil;

	/* Inform core */
	c->wasDisposed();

	disposableDisposeChildren(self);

	delete c;
	setPrivateData(self, 0);

	return Qnil;
}

RB_METHOD(disposableIsDisposed)
{
	RB_UNUSED_PARAM;

	return rb_bool_new(RTYPEDDATA_DATA(self) == 0);
}

template<class C>
static void disposableBindingInit(VALUE klass)
{
	_rb_define_method(klass, "dispose", disposableDispose<C>);
	_rb_define_method(klass, "disposed?", disposableIsDisposed);
}

#endif // DISPOSABLEBINDING_H
