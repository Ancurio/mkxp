//
//  systemImplApple.m
//  Player
//
//  Created by ゾロアーク on 11/22/20.
//

#import <Foundation/Foundation.h>
#import "system.h"

std::string systemImpl::getSystemLanguage() {
    NSString *languageCode = NSLocale.currentLocale.languageCode;
    NSString *countryCode = NSLocale.currentLocale.countryCode;
    return std::string([NSString stringWithFormat:@"%@_%@", languageCode, countryCode].UTF8String);
}

std::string systemImpl::getUserName() {
    return std::string(NSUserName().UTF8String);
}
