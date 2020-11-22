//
//  filesystemImpl.cpp
//  Player
//
//  Created by ゾロアーク on 11/21/20.
//

#include "filesystemImpl.h"
#include <filesystem>
#include <unistd.h>

// TODO
bool filesystemImpl::fileExists(const char *path) {
    assert(!"Not implemented");
    return false;
}


// TODO
std::string filesystemImpl::contentsOfFileAsString(const char *path) {
    assert(!"Not implemented");
    return std::string("");

}

// TODO
bool filesystemImpl::setCurrentDirectory(const char *path) {
    return chdir(path);
}

// TODO
std::string filesystemImpl::getCurrentDirectory() {
    return std::string(getcwd(0,0));
}

std::string filesystemImpl::normalizePath(const char *path, bool preferred, bool absolute) {
    assert(!"Not implemented");
    return std::string("");
}
