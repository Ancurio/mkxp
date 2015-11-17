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

#include <SDL_filesystem.h>

#include <fstream>
#include <stdint.h>

#include "debugwriter.h"
#include "util.h"
#include "sdl-util.h"

/* http://stackoverflow.com/a/1031773 */
static bool validUtf8(const char *string)
{
	const uint8_t *bytes = (uint8_t*) string;

	while(*bytes)
	{
		if( (/* ASCII
			  * use bytes[0] <= 0x7F to allow ASCII control characters */
				bytes[0] == 0x09 ||
				bytes[0] == 0x0A ||
				bytes[0] == 0x0D ||
				(0x20 <= bytes[0] && bytes[0] <= 0x7E)
			)
		) {
			bytes += 1;
			continue;
		}

		if( (/* non-overlong 2-byte */
				(0xC2 <= bytes[0] && bytes[0] <= 0xDF) &&
				(0x80 <= bytes[1] && bytes[1] <= 0xBF)
			)
		) {
			bytes += 2;
			continue;
		}

		if( (/* excluding overlongs */
				bytes[0] == 0xE0 &&
				(0xA0 <= bytes[1] && bytes[1] <= 0xBF) &&
				(0x80 <= bytes[2] && bytes[2] <= 0xBF)
			) ||
			(/* straight 3-byte */
				((0xE1 <= bytes[0] && bytes[0] <= 0xEC) ||
					bytes[0] == 0xEE ||
					bytes[0] == 0xEF) &&
				(0x80 <= bytes[1] && bytes[1] <= 0xBF) &&
				(0x80 <= bytes[2] && bytes[2] <= 0xBF)
			) ||
			(/* excluding surrogates */
				bytes[0] == 0xED &&
				(0x80 <= bytes[1] && bytes[1] <= 0x9F) &&
				(0x80 <= bytes[2] && bytes[2] <= 0xBF)
			)
		) {
			bytes += 3;
			continue;
		}

		if( (/* planes 1-3 */
				bytes[0] == 0xF0 &&
				(0x90 <= bytes[1] && bytes[1] <= 0xBF) &&
				(0x80 <= bytes[2] && bytes[2] <= 0xBF) &&
				(0x80 <= bytes[3] && bytes[3] <= 0xBF)
			) ||
			(/* planes 4-15 */
				(0xF1 <= bytes[0] && bytes[0] <= 0xF3) &&
				(0x80 <= bytes[1] && bytes[1] <= 0xBF) &&
				(0x80 <= bytes[2] && bytes[2] <= 0xBF) &&
				(0x80 <= bytes[3] && bytes[3] <= 0xBF)
			) ||
			(/* plane 16 */
				bytes[0] == 0xF4 &&
				(0x80 <= bytes[1] && bytes[1] <= 0x8F) &&
				(0x80 <= bytes[2] && bytes[2] <= 0xBF) &&
				(0x80 <= bytes[3] && bytes[3] <= 0xBF)
			)
		) {
			bytes += 4;
			continue;
		}

		return false;
	}

	return true;
}

static std::string prefPath(const char *org, const char *app)
{
	char *path = SDL_GetPrefPath(org, app);

	if (!path)
		return std::string();

	std::string str(path);
	SDL_free(path);

	return str;
}

typedef std::vector<std::string> StringVec;
namespace po = boost::program_options;

#define CONF_FILE "mkxp.conf"

Config::Config()
{}

void Config::read(int argc, char *argv[])
{
#define PO_DESC_ALL \
	PO_DESC(debugMode, bool, false) \
	PO_DESC(printFPS, bool, false) \
	PO_DESC(fullscreen, bool, false) \
	PO_DESC(fixedAspectRatio, bool, true) \
	PO_DESC(smoothScaling, bool, false) \
	PO_DESC(vsync, bool, false) \
	PO_DESC(defScreenW, int, 0) \
	PO_DESC(defScreenH, int, 0) \
	PO_DESC(fixedFramerate, int, 0) \
	PO_DESC(frameSkip, bool, true) \
	PO_DESC(syncToRefreshrate, bool, false) \
	PO_DESC(solidFonts, bool, false) \
	PO_DESC(subImageFix, bool, false) \
	PO_DESC(gameFolder, std::string, ".") \
	PO_DESC(anyAltToggleFS, bool, false) \
	PO_DESC(allowSymlinks, bool, false) \
	PO_DESC(dataPathOrg, std::string, "") \
	PO_DESC(dataPathApp, std::string, "") \
	PO_DESC(iconPath, std::string, "") \
	PO_DESC(execName, std::string, "Game") \
	PO_DESC(midi.soundFont, std::string, "") \
	PO_DESC(midi.chorus, bool, false) \
	PO_DESC(midi.reverb, bool, false) \
	PO_DESC(SE.sourceCount, int, 6) \
	PO_DESC(pathCache, bool, true)

// Not gonna take your shit boost
#define GUARD_ALL( exp ) try { exp } catch(...) {}

#define PO_DESC(key, type, def) (#key, po::value< type >()->default_value(def))

	po::options_description podesc;
	podesc.add_options()
	        PO_DESC_ALL
	        ("preloadScript", po::value<StringVec>()->composing())
	        ("RTP", po::value<StringVec>()->composing())
	        ("fontSub", po::value<StringVec>()->composing())
	        ("rubyLoadpath", po::value<StringVec>()->composing())
	        ;

	po::variables_map vm;

	/* Parse command line options */
	try
	{
		po::parsed_options cmdPo =
			po::command_line_parser(argc, argv).options(podesc).run();
		po::store(cmdPo, vm);
	}
	catch (po::error &error)
	{
		Debug() << "Command line:" << error.what();
	}

	/* Parse configuration file */
	SDLRWStream confFile(CONF_FILE, "r");

	if (confFile)
	{
		try
		{
			po::store(po::parse_config_file(confFile.stream(), podesc, true), vm);
			po::notify(vm);
		}
		catch (po::error &error)
		{
			Debug() << CONF_FILE":" << error.what();
		}
	}

#undef PO_DESC
#define PO_DESC(key, type, def) GUARD_ALL( key = vm[#key].as< type >(); )

	PO_DESC_ALL;

	GUARD_ALL( preloadScripts = vm["preloadScript"].as<StringVec>(); );

	GUARD_ALL( rtps = vm["RTP"].as<StringVec>(); );

	GUARD_ALL( fontSubs = vm["fontSub"].as<StringVec>(); );

	GUARD_ALL( rubyLoadpaths = vm["rubyLoadpath"].as<StringVec>(); );

#undef PO_DESC
#undef PO_DESC_ALL

	rgssVersion = clamp(rgssVersion, 0, 3);

	SE.sourceCount = clamp(SE.sourceCount, 1, 64);

	if (!dataPathOrg.empty() && !dataPathApp.empty())
		customDataPath = prefPath(dataPathOrg.c_str(), dataPathApp.c_str());

	commonDataPath = prefPath(".", "Oneshot");

	//Hardcode some ini/version settings
	rgssVersion = 1;
	game.title = "Oneshot";
	game.scripts = "Data/Scripts.rxdata";
	if (defScreenW <= 0)
		defScreenW = 640;
	if (defScreenH <= 0)
		defScreenH = 480;
}

static std::string baseName(const std::string &path)
{
	size_t pos = path.find_last_of("/\\");

	if (pos == path.npos)
		return path;

	return path.substr(pos + 1);
}
