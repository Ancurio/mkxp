/*
** binding.h
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

#ifndef BINDING_H
#define BINDING_H

struct ScriptBinding
{
	/* Starts the part where the binding takes over,
	 * loading the compressed scripts and executing them.
	 * This function returns as soon as the scripts finish
	 * execution or an error is encountered */
	void (*execute) (void);

	/* Instructs the binding
	 * to immediately terminate script execution. This
	 * function will perform a longjmp instead of returning,
	 * so be careful about any variables with local storage */
	void (*terminate) (void);

	/* Instructs the binding to issue a game reset.
	 * Same conditions as for terminate apply */
	void (*reset) (void);
};

/* VTable defined in the binding source */
extern ScriptBinding *scriptBinding;

#endif // BINDING_H
