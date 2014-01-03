/*
** plugin.cpp
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

#include "plugin.h"

#include "sharedstate.h"
#include "config.h"
#include "exception.h"

#include <SDL_loadso.h>

#include <string.h>

struct LoadedPlugin
{
	void *handle;
	Plugin *iface;
	bool inited;
};

struct PluginLoaderPrivate
{
	std::vector<LoadedPlugin> loadedPlugins;

	/* Load so files / lookup interfaces */
	void loadPlugins(const std::vector<std::string> &plgList,
	                 const char *binding)
	{
		for (size_t i = 0; i < plgList.size(); ++i)
		{
			const std::string &plgName = plgList[i];

			void *handle = SDL_LoadObject(plgName.c_str());

			if (!handle)
				throw Exception(Exception::SDLError, "%s", SDL_GetError());

			Plugin *iface = static_cast<Plugin*>(SDL_LoadFunction(handle, "plugin"));

			if (!iface)
				throw Exception(Exception::SDLError, "Error loading plugin '%s' interface: %s",
				                plgName.c_str(), SDL_GetError());

			if (strcmp(binding, iface->binding))
				throw Exception(Exception::MKXPError, "Plugin '%s' is incompatible with '%s' binding",
				                plgName.c_str(), binding);

			LoadedPlugin loaded;
			loaded.handle = handle;
			loaded.iface  = iface;
			loaded.inited = false;

			loadedPlugins.push_back(loaded);
		}
	}

	/* Initialize loaded plugins */
	void initPlugins()
	{
		for (size_t i = 0; i < loadedPlugins.size(); ++i)
		{
			LoadedPlugin &plugin = loadedPlugins[i];

			plugin.iface->init();
			plugin.inited = true;
		}
	}

	/* Finalize loaded plugins */
	void finiPlugins()
	{
		for (size_t i = 0; i < loadedPlugins.size(); ++i)
		{
			LoadedPlugin &plugin = loadedPlugins[i];

			if (plugin.inited)
				plugin.iface->fini();
		}
	}

	/* Unload so files */
	void unloadPlugins()
	{
		for (size_t i = 0; i < loadedPlugins.size(); ++i)
			SDL_UnloadObject(loadedPlugins[i].handle);
	}
};

PluginLoader::PluginLoader(const char *binding)
{
	p = new PluginLoaderPrivate;

	const std::vector<std::string> &plgList = shState->config().plugins;

	try
	{
		p->loadPlugins(plgList, binding);
	}
	catch (const Exception &exc)
	{
		p->unloadPlugins();
		delete p;

		throw exc;
	}

	try
	{
		p->initPlugins();
	}
	catch (const Exception &exc)
	{
		p->finiPlugins();
		p->unloadPlugins();
		delete p;

		throw exc;
	}
}

PluginLoader::~PluginLoader()
{
	p->finiPlugins();
	p->unloadPlugins();
	delete p;
}
