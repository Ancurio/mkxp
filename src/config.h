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

#include <set>
#include <string>
#include <vector>

#ifdef HAVE_DISCORDSDK
#include <discord_game_sdk.h>
#endif

struct Config {
  int rgssVersion;

  struct {
    char major;
    char minor;
  } glVersion;

  bool debugMode;
  bool printFPS;

  bool winResizable;
  bool fullscreen;
  bool fixedAspectRatio;
  bool smoothScaling;
  bool vsync;

  int defScreenW;
  int defScreenH;
  std::string windowTitle;

  int fixedFramerate;
  bool frameSkip;
  bool syncToRefreshrate;

  bool solidFonts;

  bool subImageFix;
  bool enableBlitting;
  int maxTextureSize;

  std::string gameFolder;
  bool anyAltToggleFS;
  bool enableReset;
  bool allowSymlinks;
  bool pathCache;
  bool encryptedGraphics;

  std::string dataPathOrg;
  std::string dataPathApp;

  std::string iconPath;
  std::string execName;
  std::string titleLanguage;

  struct {
    std::string soundFont;
    bool chorus;
    bool reverb;
  } midi;

  struct {
    int sourceCount;
  } SE;

  bool useScriptNames;

#ifdef HAVE_DISCORDSDK
  DiscordClientId discordClientId;
#endif

#ifdef HAVE_STEAMWORKS
  unsigned int steamAppId;
#endif

  std::string customScript;
  std::vector<std::string> preloadScripts;
  std::vector<std::string> rtps;

  std::vector<std::string> fontSubs;

  std::vector<std::string> rubyLoadpaths;

  /* Editor flags */
  struct {
    bool debug;
    bool battleTest;
  } editor;

  /* Game INI contents */
  struct {
    std::string scripts;
    std::string title;
  } game;

  /* Internal */
  std::string customDataPath;
  std::string commonDataPath;

  Config();

  void read(int argc, char *argv[]);
  void readGameINI();
};

#endif // CONFIG_H
