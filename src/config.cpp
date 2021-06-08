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

#if defined(MKXPZ_BUILD_XCODE) || defined(MKXPZ_INI_ENCODING)
#include <iconv.h>
#include <uchardet.h>
#include <errno.h>
#endif

namespace json = json5pp;

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
            (it.second.is_boolean() && destVec[it.first].is_boolean()) )
        {
            destVec[it.first] = it.second;
        }
        else {
            Debug() << "Invalid or unrecognized variable in configuration:" << objectName << it.first;
        }
    }
    return true;
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
        {"encryptedGraphics", true},
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
        
        copyObject(optsJ, confData);
        copyObject(opts["bindingNames"], confData.as_object()["bindingNames"], "bindingNames .");
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
    SET_OPT(useScriptNames, boolean);
    SET_OPT_CUSTOMKEY(jit.enabled, JITEnable, boolean);
    SET_OPT_CUSTOMKEY(jit.verboseLevel, JITVerboseLevel, integer);
    SET_OPT_CUSTOMKEY(jit.maxCache, JITMaxCache, integer);
    SET_OPT_CUSTOMKEY(jit.minCalls, JITMinCalls, integer);
    
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
    
    bool convSuccess = false;

    // Attempt to convert from other encodings to UTF-8
    if (!validUtf8(game.title.c_str()))
    {
#if defined(MKXPZ_BUILD_XCODE) || defined(MKXPZ_INI_ENCODING)
        uchardet_t ud = uchardet_new();
        uchardet_handle_data(ud, game.title.c_str(), game.title.length());
        uchardet_data_end(ud);
        const char *charset = uchardet_get_charset(ud);
        
        Debug() << iniFileName << ": Assuming encoding is" << charset << "...";
        iconv_t cd = iconv_open("UTF-8", charset);
        
        uchardet_delete(ud);
        
        size_t inLen = game.title.size();
        size_t outLen = inLen * 4;
        std::string buf(outLen, '\0');
        char *inPtr = const_cast<char*>(game.title.c_str());
        char *outPtr = const_cast<char*>(buf.c_str());
        
        errno = 0;
        size_t result = iconv(cd, &inPtr, &inLen, &outPtr, &outLen);
        
        iconv_close(cd);
        
        if (result != (size_t)-1 && errno == 0)
        {
            buf.resize(buf.size()-outLen);
            game.title = buf;
            convSuccess = true;
        }
        else {
            Debug() << iniFileName << ": failed to convert game title to UTF-8";
        }
#else
        Debug() << iniFileName << ": Game title isn't valid UTF-8";
#endif
    }
    else
        convSuccess = true;
    
    if (game.title.empty() || !convSuccess)
        game.title = "mkxp-z";
    
    if (dataPathOrg.empty())
        dataPathOrg = ".";
    
    if (dataPathApp.empty())
        dataPathApp = game.title;
    
    customDataPath = prefPath(dataPathOrg.c_str(), dataPathApp.c_str());
    
    commonDataPath = prefPath(".", "mkxp-z");
    
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
