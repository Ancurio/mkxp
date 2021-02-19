//
//  filesystemImplApple.mm
//  Player
//
//  Created by ゾロアーク on 11/21/20.
//

#ifdef MKXPZ_DEBUG
#import <AppKit/AppKit.h>
#import <SDL_syswm.h>
#else
#import <Foundation/Foundation.h>
#endif

#import <SDL_filesystem.h>

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

std::string filesystemImpl::getDefaultGameRoot() {
    NSString *p = [NSString stringWithFormat: @"%@/%s", NSBundle.mainBundle.bundlePath, "Contents/Game"];
    return std::string(NSTOPATH(p));
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

#ifdef MKXPZ_DEBUG
std::string filesystemImpl::selectPath(SDL_Window *win) {
    NSOpenPanel *panel = [NSOpenPanel openPanel];
    panel.canChooseDirectories = true;
    panel.canChooseFiles = false;
    //panel.directoryURL = [NSURL fileURLWithPath:NSFileManager.defaultManager.currentDirectoryPath];
    
    SDL_SysWMinfo windowinfo{};
    SDL_GetWindowWMInfo(win, &windowinfo);
    
    [panel beginSheetModalForWindow:windowinfo.info.cocoa.window completionHandler:^(NSModalResponse res){
        [NSApp stopModalWithCode:res];
    }];
    
    [NSApp runModalForWindow:windowinfo.info.cocoa.window];
    if (panel.URLs.count > 0)
        return std::string(NSTOPATH(panel.URLs[0].path));
    
    return std::string();
}
#endif
