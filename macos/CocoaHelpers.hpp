//
//  CocoaHelpers.hpp
//  PlayerLegacy
//
//  Created by ゾロアーク on 11/18/20.
//

#ifndef CocoaHelpers_hpp
#define CocoaHelpers_hpp

#include <string>
#include <vector>

class Cocoa {
public:
    static std::string getFile(const char *baseName, const char *ext);
    static std::string getFilePath(const char *baseName, const char *ext);
};

#endif /* CocoaHelpers_hpp */
