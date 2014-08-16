/*
** disposable.h
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

#ifndef DISPOSABLE_H
#define DISPOSABLE_H

#include "exception.h"

#include <sigc++/signal.h>
#include <sigc++/connection.h>

class Disposable
{
public:
	~Disposable()
	{
		wasDisposed();
	}

	sigc::signal<void> wasDisposed;
};

/* A helper struct which monitors the dispose signal of
 * properties, and automatically sets the prop pointer to
 * null. Can call an optional notify method when prop is
 * nulled */
template<class C, typename P>
struct DisposeWatch
{
	typedef void (C::*NotifyFun)();

	DisposeWatch(C &owner, P *&propLocation, NotifyFun notify = 0)
	    : owner(owner),
	      notify(notify),
	      propLocation(propLocation)
	{}

	~DisposeWatch()
	{
		dispCon.disconnect();
	}

	/* Call this when a new object was set for the prop */
	void update(Disposable *prop)
	{
		dispCon.disconnect();

		if (!prop)
			return;

		dispCon = prop->wasDisposed.connect
			(sigc::mem_fun(this, &DisposeWatch::onDisposed));
	}

private:
	/* The object owning the prop (and this helper) */
	C  &owner;
	/* Optional notify method */
	const NotifyFun notify;
	/* Location of the prop pointer inside the owner */
	P * &propLocation;
	sigc::connection dispCon;

	void onDisposed()
	{
		dispCon.disconnect();
		propLocation = 0;

		if (notify)
			(owner.*notify)();
	}
};

#endif // DISPOSABLE_H
