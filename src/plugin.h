/*
** plugin.h
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

#ifndef PLUGIN_H
#define PLUGIN_H

/* Note: Plugins are meant to be compiled in unison
 * with mkxp; there is no API stability whatsoever. */

/* Use one of these for 'binding' */
#define PLUGIN_MRI   "mri"
#define PLUGIN_MRUBY "mruby"

/* Interface that plugins implement via global symbol named 'plugin'*/
struct Plugin
{
	/* Initialize plugin. May throw 'Exception' instance on error.
	 * The interpreted environment is set up at this point. */
	void (*init)();

	/* Finalize plugin. The interpreted environment
	 * is already torn down at this point. */
	void (*fini)();

	/* Binding that this plugin applies to (see above) */
	const char *binding;
};


/* For use in main mkxp */
struct PluginLoaderPrivate;

class PluginLoader
{
public:
	PluginLoader(const char *binding);
	~PluginLoader();

private:
	PluginLoaderPrivate *p;
};

#endif // PLUGIN_H
