//
//  filesystemImplApple.mm
//  Player
//
//  Created by ゾロアーク on 11/21/20.
//

#import <Foundation/Foundation.h>
#import "filesystemImpl.h"
#import "util/exception.h"

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
        throw Exception(Exception::NoFileError, "Failed to read file at %s", path);
    
    return std::string(fileContents.UTF8String);

}


bool filesystemImpl::setCurrentDirectory(const char *path) {
    return [NSFileManager.defaultManager changeCurrentDirectoryPath: PATHTONS(path)];
}

std::string filesystemImpl::getCurrentDirectory() {
    return std::string(NSTOPATH(NSFileManager.defaultManager.currentDirectoryPath));
}

std::string filesystemImpl::normalizePath(const char *path, bool preferred, bool absolute) {
    NSString *nspath = [NSURL fileURLWithPath: PATHTONS(path)].URLByStandardizingPath.path;
    NSString *pwd = [NSString stringWithFormat:@"%@/", NSFileManager.defaultManager.currentDirectoryPath];
    if (!absolute) {
        nspath = [nspath stringByReplacingOccurrencesOfString:pwd withString:@""];
    }
    nspath = [nspath stringByReplacingOccurrencesOfString:@"\\" withString:@"/"];
    return std::string(NSTOPATH(nspath));
}

NSString *getPathForAsset_internal(const char *baseName, const char *ext) {
    NSBundle *assetBundle = [NSBundle bundleWithPath:
                             [NSString stringWithFormat:
                              @"%@/%s",
                              NSBundle.mainBundle.resourcePath,
                              "Assets.bundle"
                             ]
                            ];
    
    if (assetBundle == nil)
        return nil;
    
    return [assetBundle pathForResource: @(baseName) ofType: @(ext)];
}

std::string filesystemImpl::getPathForAsset(const char *baseName, const char *ext) {
    NSString *assetPath = getPathForAsset_internal(baseName, ext);
    if (assetPath == nil)
        throw Exception(Exception::NoFileError, "Failed to find the asset named %s.%s", baseName, ext);
    
    return std::string(NSTOPATH(getPathForAsset_internal(baseName, ext)));
}

std::string filesystemImpl::contentsOfAssetAsString(const char *baseName, const char *ext) {
    NSString *path = getPathForAsset_internal(baseName, ext);
    NSString *fileContents = [NSString stringWithContentsOfFile: path];
    
    // This should never fail
    if (fileContents == nil)
        throw Exception(Exception::MKXPError, "Failed to read file at %s", path.UTF8String);
    
    return std::string(fileContents.UTF8String);

}
