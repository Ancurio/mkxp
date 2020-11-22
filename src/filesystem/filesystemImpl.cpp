//
//  filesystemImpl.cpp
//  Player
//
//  Created by ゾロアーク on 11/21/20.
//

#include "filesystemImpl.h"
#include "util/exception.h"

#include <sys/stat.h>
#include <unistd.h>
#include <fstream>

// https://stackoverflow.com/questions/12774207/fastest-way-to-check-if-a-file-exist-using-standard-c-c11-c
bool filesystemImpl::fileExists(const char *path) {
    struct stat buffer;   
    return (stat (path, &buffer) == 0); 
}


// https://stackoverflow.com/questions/2912520/read-file-contents-into-a-string-in-c
std::string filesystemImpl::contentsOfFileAsString(const char *path) {
    std::string ret;
    try {
        std::ifstream ifs(path);
        ret = std::string ( (std::istreambuf_iterator<char>(ifs) ),
                       (std::istreambuf_iterator<char>()    ) );
    } catch (...) {
        throw new Exception(Exception::NoFileError, "Failed to read file at %s", path);
    }

    return ret;
}

// chdir and getcwd do not support unicode on Windows
bool filesystemImpl::setCurrentDirectory(const char *path) {
    return chdir(path);
}

std::string filesystemImpl::getCurrentDirectory() {
    return std::string(getcwd(0,0));
}


std::string filesystemImpl::normalizePath(const char *path, bool preferred, bool absolute) {
    return std::string(path);
}
