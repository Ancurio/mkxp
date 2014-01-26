/*
** config.cpp
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

#include "config.h"

#include <boost/program_options/options_description.hpp>
#include <boost/program_options/parsers.hpp>
#include <boost/program_options/variables_map.hpp>

#include <fstream>

#include "debugwriter.h"
#include "util.h"

typedef std::vector<std::string> StringVec;
namespace po = boost::program_options;

Config::Config()
    : debugMode(false),
      screenshots(false),
      winResizable(false),
      fullscreen(false),
      fixedAspectRatio(true),
      smoothScaling(false),
      vsync(false),
      defScreenW(640),
      defScreenH(480),
      fixedFramerate(0),
      frameSkip(true),
      solidFonts(false),
      forceBitmapBlit(false),
      gameFolder("."),
      anyAltToggleFS(false),
      allowSymlinks(false),
      pathCache(true)
{}

void Config::read(int argc, char *argv[])
{
#define PO_DESC_ALL \
	PO_DESC(debugMode, bool) \
	PO_DESC(screenshots, bool) \
	PO_DESC(winResizable, bool) \
	PO_DESC(fullscreen, bool) \
	PO_DESC(fixedAspectRatio, bool) \
	PO_DESC(smoothScaling, bool) \
	PO_DESC(vsync, bool) \
	PO_DESC(defScreenW, int) \
	PO_DESC(defScreenH, int) \
	PO_DESC(fixedFramerate, int) \
	PO_DESC(frameSkip, bool) \
	PO_DESC(solidFonts, bool) \
	PO_DESC(forceBitmapBlit, bool) \
	PO_DESC(gameFolder, std::string) \
	PO_DESC(anyAltToggleFS, bool) \
	PO_DESC(allowSymlinks, bool) \
	PO_DESC(iconPath, std::string) \
	PO_DESC(customScript, std::string) \
	PO_DESC(pathCache, bool)

// Not gonna take your shit boost
#define GUARD_ALL( exp ) try { exp } catch(...) {}

#define PO_DESC(key, type) (#key, po::value< type >()->default_value(key))

	po::options_description podesc;
	podesc.add_options()
	        PO_DESC_ALL
	        ("RTP", po::value<StringVec>()->composing())
	        ;

	po::variables_map vm;

	/* Parse command line options */
	po::parsed_options cmdPo =
	    po::command_line_parser(argc, argv).options(podesc)
	                                       .allow_unregistered()
	                                       .run();

	GUARD_ALL( po::store(cmdPo, vm); )

	/* Parse configuration file (mkxp.conf) */
	std::ifstream confFile;
	confFile.open("mkxp.conf");

	if (confFile)
	{
		GUARD_ALL( po::store(po::parse_config_file(confFile, podesc, true), vm); )
	}

	confFile.close();

	po::notify(vm);

#undef PO_DESC
#define PO_DESC(key, type) GUARD_ALL( key = vm[#key].as< type >(); )

	PO_DESC_ALL;

	GUARD_ALL( rtps = vm["RTP"].as<StringVec>(); );

#undef PO_DESC
#undef PO_DESC_ALL
}

static std::string baseName(const std::string &path)
{
	size_t pos = path.find_last_of("/\\");

	if (pos == path.npos)
		return path;

	return path.substr(pos + 1);
}

void Config::readGameINI()
{
	if (!customScript.empty())
	{
		game.title = baseName(customScript);
		return;
	}

	po::options_description podesc;
	podesc.add_options()
	        ("Game.Title", po::value<std::string>())
	        ("Game.Scripts", po::value<std::string>())
	        ;

	std::string iniPath = gameFolder + "/Game.ini";

	std::ifstream iniFile;
	iniFile.open((iniPath).c_str());

	po::variables_map vm;
	GUARD_ALL( po::store(po::parse_config_file(iniFile, podesc, true), vm); )
	po::notify(vm);

	iniFile.close();

	GUARD_ALL( game.title = vm["Game.Title"].as<std::string>(); );
	GUARD_ALL( game.scripts = vm["Game.Scripts"].as<std::string>(); );

	strReplace(game.scripts, '\\', '/');

	if (game.title.empty())
		game.title = baseName(gameFolder);
}
