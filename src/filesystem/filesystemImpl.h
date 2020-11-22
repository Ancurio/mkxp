//
//  filesystemImpl.h
//  Player
//
//  Created by ゾロアーク on 11/21/20.
//

#ifndef filesystemImpl_h
#define filesystemImpl_h

#include <string>

class filesystemImpl {
public:
static bool fileExists(const char *path);

static std::string contentsOfFileAsString(const char *path);

};
#endif /* filesystemImpl_h */
