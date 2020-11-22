//
//  filesystemImplApple.m
//  Player
//
//  Created by ゾロアーク on 11/21/20.
//

#import <Foundation/Foundation.h>
#import "filesystemImpl.h"
#import "exception.h"

bool filesystemImpl::fileExists(const char *path) {
    return  [NSFileManager.defaultManager fileExistsAtPath:@(path) isDirectory: FALSE] == TRUE;
}



std::string filesystemImpl::contentsOfFileAsString(const char *path) {
    NSString *fileContents = [NSString stringWithContentsOfFile: @(path)];
    if (fileContents == nil)
        throw new Exception(Exception::MKXPError, "Failed to locate file at %s", path);
    
    return std::string(fileContents.UTF8String);

}
