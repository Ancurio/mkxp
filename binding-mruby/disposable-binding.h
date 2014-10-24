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

#include "mruby/array.h"

#include <string.h>

/* 'Children' are disposables that are disposed together
 * with their parent. Currently this is only used by Viewport
 * in RGSS1.
 * FIXME: Disable this behavior when RGSS2 or 3 */
inline void
disposableAddChild(mrb_state *mrb, mrb_value disp, mrb_value child)
{
	mrb_sym sym = getMrbData(mrb)->symbols[CSchildren];
	mrb_value children = mrb_iv_get(mrb, disp, sym);

	if (mrb_nil_p(children))
	{
		children = mrb_ary_new(mrb);
		mrb_iv_set(mrb, disp, sym, children);
	}

	/* Assumes children are never removed until destruction */
	mrb_ary_push(mrb, children, child);
}

inline void
disposableDisposeChildren(mrb_state *mrb, mrb_value disp)
{
	MrbData *mrbData = getMrbData(mrb);
	mrb_value children = mrb_iv_get(mrb, disp, mrbData->symbols[CSchildren]);

	if (mrb_nil_p(children))
		return;

	for (mrb_int i = 0; i < RARRAY_LEN(children); ++i)
		mrb_funcall_argv(mrb, mrb_ary_entry(children, i),
		                 mrbData->symbols[CS_mkxp_dispose_alias], 0, 0);
}

template<class C>
MRB_METHOD(disposableDispose)
{
	C *d = static_cast<C*>(DATA_PTR(self));

	if (!d)
		return mrb_nil_value();

	if (d->isDisposed())
		return mrb_nil_value();

	if (rgssVer == 1)
		disposableDisposeChildren(mrb, self);

	d->dispose();

	return mrb_nil_value();
}

template<class C>
MRB_METHOD(disposableIsDisposed)
{
	MRB_UNUSED_PARAM;

	C *d = static_cast<C*>(DATA_PTR(self));

	if (!d)
		return mrb_true_value();

	return mrb_bool_value(d->isDisposed());
}

template<class C>
static void disposableBindingInit(mrb_state *mrb, RClass *klass)
{
	mrb_define_method(mrb, klass, "dispose", disposableDispose<C>, MRB_ARGS_NONE());
	mrb_define_method(mrb, klass, "disposed?", disposableIsDisposed<C>, MRB_ARGS_NONE());

	if (rgssVer == 1)
		mrb_alias_method(mrb, klass, getMrbData(mrb)->symbols[CS_mkxp_dispose_alias],
		                 mrb_intern_lit(mrb, "dispose"));
}

template<class C>
inline void
checkDisposed(mrb_state *mrb, mrb_value self)
{
	if (mrb_test(disposableIsDisposed<C>(0, self)))
		raiseDisposedAccess(mrb, self);
}

#endif // DISPOSABLEBINDING_H
