#import <ObjFW/ObjFW.h>
#import <SDL_filesystem.h>
#import "config.h"

#import <stdint.h>
#import <vector>

#import "debugwriter.h"
#import "util.h"
#import "sdl-util.h"

#ifdef HAVE_DISCORDSDK
#import <discord_game_sdk.h>
#import "discordstate.h"
#endif

OFString* prefPath(const char* org, const char* app)
{
    char* path = SDL_GetPrefPath(org, app);
    if (!path)
        return [OFString string];
    
    OFString* str = [OFString stringWithUTF8String:path];
    SDL_free(path);
    return str;
}

void fillStringVec(id array, std::vector<std::string>& vector)
{
    if (![array isKindOfClass:[OFArray class]])
    {
        if ([array isKindOfClass:[OFString class]])
            vector.push_back(std::string([array UTF8String]));

        return;
    }

    OFEnumerator* e = [array objectEnumerator];
    @autoreleasepool{
        for (id obj = [e nextObject]; obj != nil; obj = [e nextObject])
        {
            if (![obj isKindOfClass:[OFString class]])
                continue;
            
            vector.push_back(std::string([obj UTF8String]));
        }
    }
}

#define CONF_FILE "mkxp.json"

Config::Config() {}

void Config::read(int argc, char *argv[])
{
    OFMutableDictionary* opts = [OFMutableDictionary dictionaryWithDictionary:@{
        @"rgssVersion": @0,
        @"debugMode": @false,
        @"printFPS": @false,
        @"winResizable": @true,
        @"fullscreen": @false,
        @"fixedAspectRatio": @true,
        @"smoothScaling": @false,
        @"vsync": @false,
        @"defScreenW": @0,
        @"defScreenH": @0,
        @"windowTitle": @"",
        @"fixedFramerate": @false,
        @"frameSkip": @false,
        @"syncToRefreshRate": @false,
        @"solidFonts": @false,
        @"subImageFix": @false,
        @"enableBlitting": @true,
        @"maxTextureSize": @0,
        @"gameFolder": @".",
        @"anyAltToggleFS": @false,
        @"enableReset": @true,
        @"allowSymlinks": @false,
        @"dataPathOrg": @"",
        @"dataPathApp": @"",
        @"iconPath": @"",
        @"execName": @"Game",
        @"midiSoundFont": @"",
        @"midiChorus": @false,
        @"midiReverb": @false,
        @"SESourceCount": @6,
        @"customScript": @"",
        @"pathCache": @true,
#ifdef HAVE_DISCORDSDK
        @"discordClientId": @DEFAULT_CLIENT_ID,
#endif
        @"useScriptNames": @1,
        @"preloadScript": @[],
        @"RTP": @[],
        @"fontSub": @[],
        @"rubyLoadpath": @[]
    }];

#define GUARD( exp )  @try { exp } @catch(...) {}

    editor.debug = false;
    editor.battleTest = false;

    if (argc > 1)
    {
        OFString* argv1 = [OFString stringWithUTF8String:argv[1]];
        if ([argv1 isEqual: @"debug"] || [argv1 isEqual: @"test"])
            editor.debug = true;
        else if ([argv1 isEqual: @"btest"])
            editor.battleTest = true;
    }

    if ([[OFFileManager defaultManager] fileExistsAtPath:@CONF_FILE])
    {
        @autoreleasepool{
            @try {
                id confData = [
                    [OFString
                    stringWithContentsOfFile:@CONF_FILE
                    encoding: OF_STRING_ENCODING_UTF_8
                    ] JSONValue];
                
                if (![confData isKindOfClass:[OFDictionary class]])
                {
                    confData = @{};
                }

                OFEnumerator* e = [confData keyEnumerator];
                for (id key = [e nextObject]; key != nil; key = [e nextObject])
                {        
                    Debug() << [[key description] UTF8String] << [[key className] UTF8String];
                    opts[key] = confData[key];
                }
            }
            @catch(OFException *e){
                Debug() << [[e description] UTF8String];
            }
        }
#define SET_OPT_CUSTOMKEY(var, key, type) GUARD( var = [opts[@#key] type]; )
#define SET_OPT(var, type) SET_OPT_CUSTOMKEY(var, var, type)
#define SET_STRINGOPT(var, key) GUARD( var = std::string([opts[@#key] UTF8String]); )
    }
    SET_OPT(rgssVersion, intValue);
    SET_OPT(debugMode, boolValue);
    SET_OPT(printFPS, boolValue);
    SET_OPT(winResizable, boolValue);
    SET_OPT(fullscreen, boolValue);
    SET_OPT(fixedAspectRatio, boolValue);
    SET_OPT(smoothScaling, boolValue);
    SET_OPT(vsync, boolValue);
    SET_OPT(defScreenW, intValue);
    SET_OPT(defScreenH, intValue);
    SET_STRINGOPT(windowTitle, windowTitle);
    SET_OPT(fixedFramerate, intValue);
    SET_OPT(frameSkip, boolValue);
    SET_OPT(syncToRefreshrate, boolValue);
    SET_OPT(solidFonts, boolValue);
    SET_OPT(subImageFix, boolValue);
    SET_OPT(enableBlitting, boolValue);
    SET_OPT(maxTextureSize, intValue);
    SET_STRINGOPT(gameFolder, gameFolder);
    SET_OPT(anyAltToggleFS, boolValue);
    SET_OPT(enableReset, boolValue);
    SET_OPT(allowSymlinks, boolValue);
    SET_STRINGOPT(dataPathOrg, dataPathOrg);
    SET_STRINGOPT(dataPathApp, dataPathApp);
    SET_STRINGOPT(iconPath, iconPath);
    SET_STRINGOPT(execName, execName);
    SET_STRINGOPT(midi.soundFont, midiSoundFont);
    SET_OPT_CUSTOMKEY(midi.chorus, midiChorus, boolValue);
    SET_OPT_CUSTOMKEY(midi.reverb, midiReverb, boolValue);
    SET_OPT_CUSTOMKEY(SE.sourceCount, SESourceCount, intValue);
    SET_STRINGOPT(customScript, customScript);
    SET_OPT(pathCache, boolValue);
    SET_OPT(useScriptNames, boolValue);

    fillStringVec(opts[@"preloadScript"], preloadScripts);
    fillStringVec(opts[@"RTP"], rtps);
    fillStringVec(opts[@"fontSub"], fontSubs);
    fillStringVec(opts[@"rubyLoadpath"], rubyLoadpaths);
    rgssVersion = clamp(rgssVersion, 0, 3);
    SE.sourceCount = clamp(SE.sourceCount, 1, 64);
#ifdef HAVE_DISCORDSDK
    SET_OPT(discordClientId, longLongValue);
#endif 

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

		game.title = [[[[OFString stringWithUTF8String:customScript.c_str()] pathComponents] componentsJoinedByString:@"/"] UTF8String];

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
			GUARD( game.title = [[iniCat stringForKey:@"Title" defaultValue:@""] UTF8String]; )
			GUARD( game.scripts = [[iniCat stringForKey:@"Scripts" defaultValue:@""] UTF8String]; )

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
        customDataPath = [prefPath(dataPathOrg.c_str(), dataPathApp.c_str()) UTF8String];
    
    commonDataPath = [prefPath(".", "mkxpz") UTF8String];
    

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