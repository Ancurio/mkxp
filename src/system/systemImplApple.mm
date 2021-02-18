//
//  systemImplApple.m
//  Player
//
//  Created by ゾロアーク on 11/22/20.
//

#import <AppKit/AppKit.h>
#import "system.h"
#import "SettingsMenuController.h"

std::string systemImpl::getSystemLanguage() {
    NSString *languageCode = NSLocale.currentLocale.languageCode;
    NSString *countryCode = NSLocale.currentLocale.countryCode;
    return std::string([NSString stringWithFormat:@"%@_%@", languageCode, countryCode].UTF8String);
}

std::string systemImpl::getUserName() {
    return std::string(NSUserName().UTF8String);
}


// constant, if it's not nil then just raise the menu instead
SettingsMenu *smenu = nil;
void openSettingsWindow() {
    if (smenu == nil) {
        smenu = [SettingsMenu openWindow];
        return;
    }
    [smenu raise];
}
