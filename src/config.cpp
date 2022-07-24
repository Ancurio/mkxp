//
//  config.cpp
//  Player
//
//  Created by ゾロアーク on 11/21/20.
//

#include "config.h"
#include <SDL_filesystem.h>
#include <assert.h>

#include <stdint.h>
#include <vector>

#include "filesystem/filesystem.h"
#include "util/exception.h"
#include "util/debugwriter.h"
#include "util/sdl-util.h"
#include "util/util.h"

#include "util/json5pp.hpp"

#include "util/iniconfig.h"
#include "util/encoding.h"

#include "system/system.h"


namespace json = json5pp;

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
    for (size_t i = 0; i < array.size(); i++) {
        if (!array[i].is_string())
            continue;
        
        vector.push_back(array[i].as_string());
    }
}

bool copyObject(json::value &dest, json::value &src, const char *objectName = "") {
    assert(dest.is_object());
    if (src.is_null())
        return false;
    
    if (!src.is_object())
        return false;
    
    auto &srcVec = src.as_object();
    auto &destVec = dest.as_object();
    
    for (auto it : srcVec) {
        // Specifically processs this object later.
        if (it.second.is_object() && destVec[it.first].is_object())
            continue;
        
        if ((it.second.is_array() && destVec[it.first].is_array())    ||
            (it.second.is_number() && destVec[it.first].is_number())  ||
            (it.second.is_string() && destVec[it.first].is_string())  ||
            (it.second.is_boolean() && destVec[it.first].is_boolean()) ||
            (destVec[it.first].is_null()))
        {
            destVec[it.first] = it.second;
        }
        else {
            Debug() << "Invalid variable in configuration:" << objectName << it.first;
        }
    }
    return true;
}

bool getEnvironmentBool(const char *env, bool defaultValue) {
    const char *e = SDL_getenv(env);
    if (!e)
        return defaultValue;
    
    if (!strcmp(e, "0"))
        return false;
    else if (!strcmp(e, "1"))
        return true;
    
    return defaultValue;
}

json::value readConfFile(const char *path) {
    
    json::value ret(0);
    if (!mkxp_fs::fileExists(path)) {
        return json::object({});
    }
    
    try {
        std::string cfg = mkxp_fs::contentsOfFileAsString(path);
        ret = json::parse5(Encoding::convertString(cfg));
    }
    catch (const std::exception &e) {
        Debug() << "Failed to parse" << path << ":" << e.what();
    }
    catch (const Exception &e) {
        Debug() << "Failed to parse" << path << ":" << "Unknown encoding";
    }
    
    if (!ret.is_object())
        ret = json::object({});
    
    return ret;
}

#define CONF_FILE "mkxp.json"

Config::Config() {}

void Config::read(int argc, char *argv[]) {
    auto optsJ = json::object({
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
#if defined(__APPLE__) && defined(__aarch64__)
        {"preferMetalRenderer", true},
#else
        {"preferMetalRenderer", false},
#endif
        {"subImageFix", false},
#ifdef __WIN32__
        {"enableBlitting", false},
#else
        {"enableBlitting", true},
#endif
        {"integerScalingActive", false},
        {"integerScalingLastMile", true},
        {"maxTextureSize", 0},
        {"gameFolder", ""},
        {"anyAltToggleFS", false},
        {"enableReset", true},
        {"enableSettings", true},
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
        {"useScriptNames", 1},
        {"preloadScript", json::array({})},
        {"RTP", json::array({})},
        {"fontSub", json::array({})},
        {"rubyLoadpath", json::array({})},
        {"JITEnable", false},
        {"JITVerboseLevel", 0},
        {"JITMaxCache", 100},
        {"JITMinCalls", 10000},
        {"bindingNames", json::object({
            {"a", "A"},
            {"b", "B"},
            {"c", "C"},
            {"x", "X"},
            {"y", "Y"},
            {"z", "Z"},
            {"l", "L"},
            {"r", "R"}
        })}
    });
    
    auto &opts = optsJ.as_object();
    
#define GUARD(exp) \
try { exp } catch (...) {}
    
    editor.debug = false;
    editor.battleTest = false;
    
    if (argc > 1) {
        if (!strcmp(argv[1], "debug") || !strcmp(argv[1], "test"))
            editor.debug = true;
        else if (!strcmp(argv[1], "btest"))
            editor.battleTest = true;
        
        for (int i = 1; i < argc; i++) {
            if (strcmp(argv[i], "debug"))
                launchArgs.push_back(argv[i]);
        }
    }
    
    json::value baseConf = readConfFile(CONF_FILE);
    copyObject(optsJ, baseConf);
    copyObject(opts["bindingNames"], baseConf.as_object()["bindingNames"], "bindingNames .");
    
#define SET_OPT_CUSTOMKEY(var, key, type) GUARD(var = opts[#key].as_##type();)
#define SET_OPT(var, type) SET_OPT_CUSTOMKEY(var, var, type)
#define SET_STRINGOPT(var, key) GUARD(var = std::string(opts[#key].as_string());)
    
    SET_STRINGOPT(gameFolder, gameFolder);
    SET_STRINGOPT(dataPathOrg, dataPathOrg);
    SET_STRINGOPT(dataPathApp, dataPathApp);
    SET_STRINGOPT(iconPath, iconPath);
    SET_STRINGOPT(execName, execName);
    SET_OPT(allowSymlinks, boolean);
    SET_OPT(pathCache, boolean);
    SET_OPT_CUSTOMKEY(jit.enabled, JITEnable, boolean);
    SET_OPT_CUSTOMKEY(jit.verboseLevel, JITVerboseLevel, integer);
    SET_OPT_CUSTOMKEY(jit.maxCache, JITMaxCache, integer);
    SET_OPT_CUSTOMKEY(jit.minCalls, JITMinCalls, integer);
    SET_OPT(rgssVersion, integer);
    SET_OPT(defScreenW, integer);
    SET_OPT(defScreenH, integer);
    
    // Take a break real quick and witch to set game folder and read the game's ini
    if (!gameFolder.empty() && !mkxp_fs::setCurrentDirectory(gameFolder.c_str())) {
        throw Exception(Exception::MKXPError, "Unable to switch into gameFolder %s", gameFolder.c_str());
    }
    
    readGameINI();
    
    // Now check for an extra mkxp.conf in the user's save directory and merge anything else from that
    userConfPath = customDataPath + "/" CONF_FILE;
    json::value userConf = readConfFile(userConfPath.c_str());
    copyObject(optsJ, userConf);
    
    // now RESUME
    
    SET_OPT(debugMode, boolean);
    SET_OPT(printFPS, boolean);
    SET_OPT(fullscreen, boolean);
    SET_OPT(fixedAspectRatio, boolean);
    SET_OPT(smoothScaling, boolean);
    SET_OPT(winResizable, boolean);
    SET_OPT(vsync, boolean);
    SET_STRINGOPT(windowTitle, windowTitle);
    SET_OPT(fixedFramerate, integer);
    SET_OPT(frameSkip, boolean);
    SET_OPT(syncToRefreshrate, boolean);
    SET_OPT(solidFonts, boolean);
#ifdef __APPLE__
    SET_OPT(preferMetalRenderer, boolean);
#endif
    SET_OPT(subImageFix, boolean);
    SET_OPT(enableBlitting, boolean);
    SET_OPT_CUSTOMKEY(integerScaling.active, integerScalingActive, boolean);
    SET_OPT_CUSTOMKEY(integerScaling.lastMileScaling, integerScalingLastMile, boolean);
    SET_OPT(maxTextureSize, integer);
    SET_OPT(anyAltToggleFS, boolean);
    SET_OPT(enableReset, boolean);
    SET_OPT(enableSettings, boolean);
    SET_STRINGOPT(midi.soundFont, midiSoundFont);
    SET_OPT_CUSTOMKEY(midi.chorus, midiChorus, boolean);
    SET_OPT_CUSTOMKEY(midi.reverb, midiReverb, boolean);
    SET_OPT_CUSTOMKEY(SE.sourceCount, SESourceCount, integer);
    SET_STRINGOPT(customScript, customScript);
    SET_OPT(useScriptNames, boolean);
    
    fillStringVec(opts["preloadScript"], preloadScripts);
    fillStringVec(opts["RTP"], rtps);
    fillStringVec(opts["fontSub"], fontSubs);
    fillStringVec(opts["rubyLoadpath"], rubyLoadpaths);
    
    auto &bnames = opts["bindingNames"].as_object();
    
#define BINDING_NAME(btn) kbActionNames.btn = bnames[#btn].as_string()
    BINDING_NAME(a);
    BINDING_NAME(b);
    BINDING_NAME(c);
    BINDING_NAME(x);
    BINDING_NAME(y);
    BINDING_NAME(z);
    BINDING_NAME(l);
    BINDING_NAME(r);
    
    rgssVersion = clamp(rgssVersion, 0, 3);
    SE.sourceCount = clamp(SE.sourceCount, 1, 64);
    
    // Determine whether to open a console window on... Windows
    winConsole = getEnvironmentBool("MKXPZ_WINDOWS_CONSOLE", editor.debug);
    
#ifdef __APPLE__
    // Determine whether to use the Metal renderer on macOS
    // Environment variable takes priority over the json setting
    preferMetalRenderer = isMetalSupported() && getEnvironmentBool("MKXPZ_MACOS_METAL", preferMetalRenderer);
#endif
    
    // Determine whether to allow manual selection of a game folder on startup
    // Only works on macOS atm, mainly used to test games located outside of the bundle.
    // The config is re-read after the window is already created, so some entries
    // may not take effect
    manualFolderSelect = getEnvironmentBool("MKXPZ_FOLDER_SELECT", false);
    
    raw = optsJ;
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
    SDLRWStream iniFile(iniFileName.c_str(), "r");
    
    bool convSuccess = false;
    if (iniFile)
    {
        INIConfiguration ic;
        if (ic.load(iniFile.stream()))
        {
            GUARD(game.title = ic.getStringProperty("Game", "Title"););
            GUARD(game.scripts = ic.getStringProperty("Game", "Scripts"););
            
            strReplace(game.scripts, '\\', '/');
            
            if (game.title.empty()) {
                Debug() << iniFileName + ": Could not find Game.Title";
            }
            
            if (game.scripts.empty())
                Debug() << iniFileName + ": Could not find Game.Scripts";
        }
    }
    else
        Debug() << "Could not read" << iniFileName;
    
    try {
        game.title = Encoding::convertString(game.title);
        convSuccess = true;
    }
    catch (const Exception &e) {
        Debug() << iniFileName + ": Could not determine encoding of Game.Title";
    }
    
    if (game.title.empty() || !convSuccess)
        game.title = "mkxp-z";
    
    if (dataPathOrg.empty())
        dataPathOrg = ".";
    
    if (dataPathApp.empty())
        dataPathApp = game.title;
    
    customDataPath = prefPath(dataPathOrg.c_str(), dataPathApp.c_str());
    
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
