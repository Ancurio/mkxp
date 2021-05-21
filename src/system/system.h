//
//  system.h
//  Player
//
//  Created by ゾロアーク on 11/22/20.
//

#ifndef system_h
#define system_h

#include <string>

namespace systemImpl {
std::string getSystemLanguage();
std::string getUserName();
int getScalingFactor();
}

#ifdef MKXPZ_BUILD_XCODE
void openSettingsWindow();
#endif

namespace mkxp_sys = systemImpl;

#endif /* system_h */
