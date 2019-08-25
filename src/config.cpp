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
#include "iniconfig.h"

#ifdef INI_ENCODING
extern "C" {
#include <libguess.h>
}
#include <iconv.h>
#include <errno.h>
#endif

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

template<typename T>
std::set<T> setFromVec(const std::vector<T> &vec)
{
	return std::set<T>(vec.begin(), vec.end());
}

typedef std::vector<std::string> StringVec;
namespace po = boost::program_options;

#define CONF_FILE "mkxp.conf"

Config::Config()
{}

void Config::read(int argc, char *argv[])
{
#define PO_DESC_ALL \
	PO_DESC(rgssVersion, int, 0) \
	PO_DESC(debugMode, bool, false) \
	PO_DESC(printFPS, bool, false) \
	PO_DESC(winResizable, bool, true) \
	PO_DESC(fullscreen, bool, false) \
	PO_DESC(fixedAspectRatio, bool, true) \
	PO_DESC(smoothScaling, bool, false) \
	PO_DESC(vsync, bool, false) \
	PO_DESC(defScreenW, int, 0) \
	PO_DESC(defScreenH, int, 0) \
	PO_DESC(windowTitle, std::string, "") \
	PO_DESC(fixedFramerate, int, 0) \
	PO_DESC(frameSkip, bool, false) \
	PO_DESC(syncToRefreshrate, bool, false) \
	PO_DESC(solidFonts, bool, false) \
	PO_DESC(subImageFix, bool, false) \
	PO_DESC(enableBlitting, bool, true) \
	PO_DESC(maxTextureSize, int, 0) \
	PO_DESC(gameFolder, std::string, ".") \
	PO_DESC(anyAltToggleFS, bool, false) \
	PO_DESC(enableReset, bool, true) \
	PO_DESC(allowSymlinks, bool, false) \
	PO_DESC(dataPathOrg, std::string, "") \
	PO_DESC(dataPathApp, std::string, "") \
	PO_DESC(iconPath, std::string, "") \
	PO_DESC(execName, std::string, "Game") \
	PO_DESC(titleLanguage, std::string, "") \
	PO_DESC(midi.soundFont, std::string, "") \
	PO_DESC(midi.chorus, bool, false) \
	PO_DESC(midi.reverb, bool, false) \
	PO_DESC(SE.sourceCount, int, 6) \
	PO_DESC(customScript, std::string, "") \
	PO_DESC(pathCache, bool, true) \
	PO_DESC(useScriptNames, bool, true)

// Not gonna take your shit boost
#define GUARD_ALL( exp ) try { exp } catch(...) {}

	editor.debug = false;
	editor.battleTest = false;

	/* Read arguments sent from the editor */
	if (argc > 1)
	{
		std::string argv1 = argv[1];
		/* RGSS1 uses "debug", 2 and 3 use "test" */
		if (argv1 == "debug" || argv1 == "test")
			editor.debug = true;
		else if (argv1 == "btest")
			editor.battleTest = true;

		/* Fix offset */
		if (editor.debug || editor.battleTest)
		{
			argc--;
			argv++;
		}
	}

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

	GUARD_ALL( preloadScripts = setFromVec(vm["preloadScript"].as<StringVec>()); );

	GUARD_ALL( rtps = vm["RTP"].as<StringVec>(); );

	GUARD_ALL( fontSubs = vm["fontSub"].as<StringVec>(); );

	GUARD_ALL( rubyLoadpaths = vm["rubyLoadpath"].as<StringVec>(); );

#undef PO_DESC
#undef PO_DESC_ALL

	rgssVersion = clamp(rgssVersion, 0, 3);

	SE.sourceCount = clamp(SE.sourceCount, 1, 64);

	if (!dataPathOrg.empty() && !dataPathApp.empty())
		customDataPath = prefPath(dataPathOrg.c_str(), dataPathApp.c_str());

	commonDataPath = prefPath(".", "mkxp");
}

static std::string baseName(const std::string &path)
{
	size_t pos = path.find_last_of("/\\");

	if (pos == path.npos)
		return path;

	return path.substr(pos + 1);
}

static void setupScreenSize(Config &conf)
{
	if (conf.defScreenW <= 0)
		conf.defScreenW = (conf.rgssVersion == 1 ? 640 : 544);

	if (conf.defScreenH <= 0)
		conf.defScreenH = (conf.rgssVersion == 1 ? 480 : 416);
}

void Config::readGameINI()
{
	if (!customScript.empty())
	{
		game.title = baseName(customScript);

		if (rgssVersion == 0)
			rgssVersion = 1;

		setupScreenSize(*this);

		return;
	}

	std::string iniFilename = execName + ".ini";
	SDLRWStream iniFile(iniFilename.c_str(), "r");

	if (iniFile)
	{
		INIConfiguration ic;
		if(ic.load(iniFile.stream()))
		{
			GUARD_ALL( game.title = ic.getStringProperty("Game", "Title"); );
			GUARD_ALL( game.scripts = ic.getStringProperty("Game", "Scripts"); );

			strReplace(game.scripts, '\\', '/');

			if (game.title.empty())
			{
				Debug() << iniFilename + ": Could not find Game.Title property";
			}

			if (game.scripts.empty())
			{
				Debug() << iniFilename + ": Could not find Game.Scripts property";
			}
		}
		else
		{
			Debug() << iniFilename + ": Failed to parse ini file";
		}
	}
	else
	{
		Debug() << "FAILED to open" << iniFilename;
	}

#ifdef INI_ENCODING
	/* Can add more later */
	const char *languages[] =
	{
		titleLanguage.c_str(),
		GUESS_REGION_JP, /* Japanese */
		GUESS_REGION_KR, /* Korean */
		GUESS_REGION_CN, /* Chinese */
		0
	};

	bool convSuccess = true;

	/* Verify that the game title is UTF-8, and if not,
	 * try to determine the encoding and convert to UTF-8 */
	if (!validUtf8(game.title.c_str()))
	{
		const char *encoding = 0;
		convSuccess = false;

		for (size_t i = 0; languages[i]; ++i)
		{
			encoding = libguess_determine_encoding(game.title.c_str(),
			                                       game.title.size(),
			                                       languages[i]);
			if (encoding)
				break;
		}

		if (encoding)
		{
			iconv_t cd = iconv_open("UTF-8", encoding);

			size_t inLen = game.title.size();
			size_t outLen = inLen * 4;
			std::string buf(outLen, '\0');
			char *inPtr = const_cast<char*>(game.title.c_str());
			char *outPtr = const_cast<char*>(buf.c_str());

			errno = 0;
			size_t result = iconv(cd, &inPtr, &inLen, &outPtr, &outLen);

			iconv_close(cd);

			if (result != (size_t) -1 && errno == 0)
			{
				buf.resize(buf.size()-outLen);
				game.title = buf;
				convSuccess = true;
			}
		}
	}

	if (!convSuccess)
		game.title.clear();
#else
	if (!validUtf8(game.title.c_str()))
		game.title.clear();
#endif

	if (game.title.empty())
		game.title = baseName(gameFolder);

	if (rgssVersion == 0)
	{
		/* Try to guess RGSS version based on Data/Scripts extension */
		rgssVersion = 1;

		if (!game.scripts.empty())
		{
			const char *p = &game.scripts[game.scripts.size()];
			const char *head = &game.scripts[0];

			while (--p != head)
				if (*p == '.')
					break;

			if (!strcmp(p, ".rvdata"))
				rgssVersion = 2;
			else if (!strcmp(p, ".rvdata2"))
				rgssVersion = 3;
		}
	}

	setupScreenSize(*this);
}
