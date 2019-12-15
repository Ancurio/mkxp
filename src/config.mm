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

#import <ObjFW/ObjFW.h>

#import "config.h"

#import <boost/program_options/options_description.hpp>
#import <boost/program_options/parsers.hpp>
#import <boost/program_options/variables_map.hpp>

#import <SDL_filesystem.h>

#import <fstream>
#import <stdint.h>

#import "debugwriter.h"
#import "util.h"
#import "sdl-util.h"

#ifdef HAVE_DISCORDSDK
#import <discord_game_sdk.h>
#import "discordstate.h"
#endif

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
#define GUARD_ALL_OBJ( exp ) @try { exp } @catch(...) {}

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
#ifdef HAVE_DISCORDSDK
            PO_DESC(discordClientId, DiscordClientId, DEFAULT_CLIENT_ID)
#endif
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
    
#ifdef HAVE_DISCORDSDK
    PO_DESC(discordClientId, DiscordClientId, DEFAULT_CLIENT_ID)
#endif

	GUARD_ALL( preloadScripts = setFromVec(vm["preloadScript"].as<StringVec>()); );

	GUARD_ALL( rtps = vm["RTP"].as<StringVec>(); );

	GUARD_ALL( fontSubs = vm["fontSub"].as<StringVec>(); );

	GUARD_ALL( rubyLoadpaths = vm["rubyLoadpath"].as<StringVec>(); );

#undef PO_DESC
#undef PO_DESC_ALL

	rgssVersion = clamp(rgssVersion, 0, 3);

	SE.sourceCount = clamp(SE.sourceCount, 1, 64);
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

	OFString* iniFilename = [OFString stringWithFormat:@"%s.ini", execName.c_str()];
	if ([[OFFileManager defaultManager] fileExistsAtPath:iniFilename])
	{
		@try{
			OFINIFile* iniFile = [OFINIFile fileWithPath:iniFilename];
			OFINICategory* iniCat = [iniFile categoryForName:@"Game"];
			GUARD_ALL_OBJ( game.title = [[iniCat stringForKey:@"Title" defaultValue:@""] UTF8String]; )
			GUARD_ALL_OBJ( game.scripts = [[iniCat stringForKey:@"Scripts" defaultValue:@""] UTF8String]; )

			strReplace(game.scripts, '\\', '/');
		}
		@catch(OFException* exc){
			Debug() << "Failed to parse INI:" << [[exc description] UTF8String];
		}
		if (game.title.empty())
			Debug() << [iniFilename UTF8String] << ": Could not find Game.Title property";
		
		if (game.scripts.empty())
			Debug() << [iniFilename UTF8String] << ": Could not find Game.Scripts property";
		
	}

/*
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
*/

	if (game.title.empty())
    {
        game.title = "mkxp-z";
    }
    else
    {
        if (dataPathOrg.empty()) dataPathOrg = ".";
        if (dataPathApp.empty()) dataPathApp = game.title;
    }
    
    if (!dataPathOrg.empty() && !dataPathApp.empty())
        customDataPath = prefPath(dataPathOrg.c_str(), dataPathApp.c_str());
    
    commonDataPath = prefPath(".", "mkxpz");
    

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
