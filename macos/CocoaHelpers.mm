//
//  CocoaHelpers.cpp
//  PlayerLegacy
//
//  Created by ゾロアーク on 11/18/20.
//

#import "CocoaHelpers.hpp"
#import <Foundation/Foundation.h>
#import <ObjFW/ObjFW.h>

// ObjFW is a bit of a dick to strings when Cocoa gets involved
// Can't use literals when using both Apple and ObjFW headers, meh

// This is a pretty lazy header, but it'll do for the moment while
// I work on Apple stuff, lots to do
#define NSSTR(ptr) [NSString stringWithUTF8String: ptr]

std::string Cocoa::getFile(const char* baseName, const char* ext) {

    return std::string([[NSString stringWithContentsOfFile: NSSTR(getFilePath(baseName, ext).c_str())] UTF8String]);
}

std::string Cocoa::getFilePath(const char *baseName, const char *ext) {
    NSBundle *assetBundle = [NSBundle bundleWithPath:
                             [NSString stringWithFormat:
                              NSSTR("%@/%s"),
                              NSBundle.mainBundle.resourcePath,
                              "Assets.bundle"
                             ]
                            ];
    
    return std::string([assetBundle pathForResource: NSSTR(baseName) ofType: NSSTR(ext)].UTF8String);
}
