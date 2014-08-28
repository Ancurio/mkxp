/*
** config.h
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

#ifndef CONFIG_H
#define CONFIG_H

#include <string>
#include <vector>

struct Config
{
	int rgssVersion;

	bool debugMode;

	bool winResizable;
	bool fullscreen;
	bool fixedAspectRatio;
	bool smoothScaling;
	bool vsync;

	int defScreenW;
	int defScreenH;

	int fixedFramerate;
	bool frameSkip;

	bool solidFonts;

	std::string gameFolder;
	bool anyAltToggleFS;
	bool allowSymlinks;
	bool pathCache;

	std::string iconPath;
	std::string titleLanguage;

	struct
	{
		std::string soundFont;
		bool chorus;
		bool reverb;
	} midi;

	struct
	{
		int sourceCount;
	} SE;

	bool useScriptNames;

	std::string customScript;
	std::vector<std::string> preloadScripts;
	std::vector<std::string> rtps;

	std::vector<std::string> fontSubs;

	std::vector<std::string> rubyLoadpaths;

	/* Game INI contents */
	struct {
		std::string scripts;
		std::string title;
	} game;

	Config();

	void read(int argc, char *argv[]);
	void readGameINI();
};

#endif // CONFIG_H
