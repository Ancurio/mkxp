//
//  filesystemImpl.h
//  Player
//
//  Created by ゾロアーク on 11/21/20.
//

#ifndef filesystemImpl_h
#define filesystemImpl_h

#include <string>

namespace filesystemImpl {
bool fileExists(const char *path);

std::string contentsOfFileAsString(const char *path);

bool setCurrentDirectory(const char *path);
    
std::string getCurrentDirectory();
    
std::string normalizePath(const char *path, bool preferred, bool absolute);

#ifdef MKXPZ_BUILD_XCODE
std::string getPathForAsset(const char *baseName, const char *ext);
std::string contentsOfAssetAsString(const char *baseName, const char *ext);
#endif

};
#endif /* filesystemImpl_h */
