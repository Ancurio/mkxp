//
//  config.cpp
//  Player
//
//  Created by ゾロアーク on 11/21/20.
//

#include "config.h"
#include <SDL_filesystem.h>

#include <stdint.h>
#include <vector>

#include "filesystem/filesystem.h"
#include "util/exception.h"
#include "util/debugwriter.h"
#include "util/sdl-util.h"
#include "util/util.h"
#include "util/json5pp.hpp"
#include "util/mINI.h"

namespace json = json5pp;
namespace ini = mINI;

std::string prefPath(const char *org, const char *app) {
    char *path = SDL_GetPrefPath(org, app);
    if (!path)
        return std::string("");
    std::string ret(path);
    SDL_free(path);
    return ret;
}

void fillStringVec(json::value &item, std::vector<std::string> &vector) {
    if (!item.is_array()) {
        if (item.is_string()) {
            vector.push_back(item.as_string());
        }
        return;
    }
    auto &array = item.as_array();
    for (int i = 0; i < array.size(); i++) {
        if (!array[i].is_string())
            continue;
        
        vector.push_back(array[i].as_string());
    }
}

#define CONF_FILE "mkxp.json"

Config::Config() {}

void Config::read(int argc, char *argv[]) {
    auto opts = json::object({
        {"rgssVersion", 0},
        {"debugMode", false},
        {"printFPS", false},
        {"winResizable", true},
        {"fullscreen", false},
        {"fixedAspectRatio", true},
        {"smoothScaling", false},
        {"vsync", false},
        {"defScreenW", 0},
        {"defScreenH", 0},
        {"windowTitle", ""},
        {"fixedFramerate", 0},
        {"frameSkip", false},
        {"syncToRefreshrate", false},
        {"solidFonts", false},
        {"subImageFix", false},
        {"enableBlitting", true},
        {"maxTextureSize", 0},
        {"gameFolder", ""},
        {"anyAltToggleFS", false},
        {"enableReset", true},
        {"allowSymlinks", false},
        {"dataPathOrg", ""},
        {"dataPathApp", ""},
        {"iconPath", ""},
        {"execName", "Game"},
        {"midiSoundFont", ""},
        {"midiChorus", false},
        {"midiReverb", false},
        {"SESourceCount", 6},
        {"customScript", ""},
        {"pathCache", true},
        {"encryptedGraphics", false},
        {"useScriptNames", 1},
        {"preloadScript", json::array({})},
        {"RTP", json::array({})},
        {"fontSub", json::array({})},
        {"rubyLoadpath", json::array({})},
        {"JITEnable", false},
        {"JITVerboseLevel", 0},
        {"JITMaxCache", 100},
        {"JITMinCalls", 10000}
    }).as_object();
    
#define GUARD(exp) \
try { exp } catch (...) {}
    
    editor.debug = false;
    editor.battleTest = false;
    
    if (argc > 1) {
        if (!strcmp(argv[1], "debug") || !strcmp(argv[1], "test"))
            editor.debug = true;
        else if (!strcmp(argv[1], "btest"))
            editor.battleTest = true;
    }
    
    if (mkxp_fs::fileExists(CONF_FILE)) {
        
        json::value confData = json::value(0);
        try {
            confData = json::parse5(mkxp_fs::contentsOfFileAsString(CONF_FILE));
        } catch (...) {
            Debug() << "Failed to parse JSON configuration";
        }
        
        if (!confData.is_object())
            confData = json::object({});
        
        auto &cdObject = confData.as_object();
        
        for (auto it : cdObject) {
            if (it.second.is_array() && opts[it.first].is_array() ||
                it.second.is_number() && opts[it.first].is_number() ||
                it.second.is_string() && opts[it.first].is_string() ||
                it.second.is_boolean() && opts[it.first].is_boolean())
            {
                opts[it.first] = it.second;
            }
            else {
                Debug() << "Invalid or unrecognized variable in configuration:" << it.first;
            }
        }
    }
    
#define SET_OPT_CUSTOMKEY(var, key, type) GUARD(var = opts[#key].as_##type();)
#define SET_OPT(var, type) SET_OPT_CUSTOMKEY(var, var, type)
#define SET_STRINGOPT(var, key) GUARD(var = std::string(opts[#key].as_string());)
    
    SET_OPT(rgssVersion, integer);
    SET_OPT(debugMode, boolean);
    SET_OPT(printFPS, boolean);
    SET_OPT(fullscreen, boolean);
    SET_OPT(fixedAspectRatio, boolean);
    SET_OPT(smoothScaling, boolean);
    SET_OPT(winResizable, boolean);
    SET_OPT(vsync, boolean);
    SET_OPT(defScreenW, integer);
    SET_OPT(defScreenH, integer);
    SET_STRINGOPT(windowTitle, windowTitle);
    SET_OPT(fixedFramerate, integer);
    SET_OPT(frameSkip, boolean);
    SET_OPT(syncToRefreshrate, boolean);
    SET_OPT(solidFonts, boolean);
    SET_OPT(subImageFix, boolean);
    SET_OPT(enableBlitting, boolean);
    SET_OPT(maxTextureSize, integer);
    SET_STRINGOPT(gameFolder, gameFolder);
    SET_OPT(anyAltToggleFS, boolean);
    SET_OPT(enableReset, boolean);
    SET_OPT(allowSymlinks, boolean);
    SET_STRINGOPT(dataPathOrg, dataPathOrg);
    SET_STRINGOPT(dataPathApp, dataPathApp);
    SET_STRINGOPT(iconPath, iconPath);
    SET_STRINGOPT(execName, execName);
    SET_STRINGOPT(midi.soundFont, midiSoundFont);
    SET_OPT_CUSTOMKEY(midi.chorus, midiChorus, boolean);
    SET_OPT_CUSTOMKEY(midi.reverb, midiReverb, boolean);
    SET_OPT_CUSTOMKEY(SE.sourceCount, SESourceCount, integer);
    SET_STRINGOPT(customScript, customScript);
    SET_OPT(pathCache, boolean);
    SET_OPT(encryptedGraphics, boolean);
    SET_OPT(useScriptNames, boolean);
    SET_OPT_CUSTOMKEY(jit.enabled, JITEnable, boolean);
    SET_OPT_CUSTOMKEY(jit.verboseLevel, JITVerboseLevel, integer);
    SET_OPT_CUSTOMKEY(jit.maxCache, JITMaxCache, integer);
    SET_OPT_CUSTOMKEY(jit.minCalls, JITMinCalls, integer);
    
    fillStringVec(opts["preloadScript"], preloadScripts);
    fillStringVec(opts["RTP"], rtps);
    fillStringVec(opts["fontSub"], fontSubs);
    fillStringVec(opts["rubyLoadpath"], rubyLoadpaths);
    rgssVersion = clamp(rgssVersion, 0, 3);
    SE.sourceCount = clamp(SE.sourceCount, 1, 64);
    
}

static void setupScreenSize(Config &conf) {
  if (conf.defScreenW <= 0)
    conf.defScreenW = (conf.rgssVersion == 1 ? 640 : 544);

  if (conf.defScreenH <= 0)
    conf.defScreenH = (conf.rgssVersion == 1 ? 480 : 416);
}


void Config::readGameINI() {
    if (!customScript.empty()) {
        game.title = customScript.c_str();
        
        if (rgssVersion == 0)
            rgssVersion = 1;
        
        setupScreenSize(*this);
        
        return;
    }
    
    std::string iniFileName(execName + ".ini");
    
    if (mkxp_fs::fileExists(iniFileName.c_str())) {
        ini::INIStructure iniStruct;
        
        if (!ini::INIFile(iniFileName).read(iniStruct)) {
            Debug() << "Failed to read INI file" << iniFileName;
        }
        else if (!iniStruct.has("Game")){
            Debug() << "INI is missing [Game] section";
        }
        
        game.title = iniStruct["Game"]["Title"];
        game.scripts = iniStruct["Game"]["Scripts"];
    }
    
    if (game.title.empty()) {
        Debug() << "INI is missing Game.Title property";
        game.title = "mkxp-z";
    }
    
    if (game.scripts.empty())
        Debug() << "INI is missing Game.Scripts property";
    
    if (dataPathOrg.empty()) {
        dataPathOrg = ".";
    }
    
    if (dataPathApp.empty()) {
        dataPathApp = game.title;
    }
    
    if (!dataPathApp.empty()) {
        customDataPath = prefPath(dataPathOrg.c_str(), dataPathApp.c_str());
    }
    
    commonDataPath = prefPath(".", "mkxpz");
    
    if (rgssVersion == 0) {
        /* Try to guess RGSS version based on Data/Scripts extension */
        rgssVersion = 1;

        if (!game.scripts.empty()) {
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
