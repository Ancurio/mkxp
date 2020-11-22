//
//  filesystemImplApple.m
//  Player
//
//  Created by ゾロアーク on 11/21/20.
//

#import <Foundation/Foundation.h>
#import "filesystemImpl.h"
#import "exception.h"

#define PATHTONS(str) [NSFileManager.defaultManager stringWithFileSystemRepresentation:str length:strlen(str)]

#define NSTOPATH(str) [NSFileManager.defaultManager fileSystemRepresentationWithPath:str]

bool filesystemImpl::fileExists(const char *path) {
    NSString *nspath = PATHTONS(path);
    BOOL isDir;
    return  [NSFileManager.defaultManager fileExistsAtPath:PATHTONS(path) isDirectory: &isDir] && !isDir;
}



std::string filesystemImpl::contentsOfFileAsString(const char *path) {
    NSString *fileContents = [NSString stringWithContentsOfFile: PATHTONS(path)];
    if (fileContents == nil)
        throw new Exception(Exception::NoFileError, "Failed to locate file at %s", path);
    
    return std::string(fileContents.UTF8String);

}


bool filesystemImpl::setCurrentDirectory(const char *path) {
    return [NSFileManager.defaultManager changeCurrentDirectoryPath: PATHTONS(path)];
}

std::string filesystemImpl::getCurrentDirectory() {
    return std::string(NSTOPATH(NSFileManager.defaultManager.currentDirectoryPath));
}

std::string filesystemImpl::normalizePath(const char *path, bool preferred, bool absolute) {
    NSString *nspath = PATHTONS(path);
    if (!nspath.isAbsolutePath && absolute) {
        nspath = [NSURL fileURLWithPath: nspath].URLByStandardizingPath.path;
    }
    else {
        nspath = nspath.stringByStandardizingPath;
    }
    return std::string(NSTOPATH(nspath));
}
