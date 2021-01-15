//
//  SettingsMenuController.h
//  mkxp-z
//
//  Created by ゾロアーク on 1/15/21.
//

#ifndef SettingsMenuController_h
#define SettingsMenuController_h

#import <AppKit/AppKit.h>

#import <vector>

#import "eventthread.h"


@interface SettingsMenu : NSViewController

+(SettingsMenu*)openWindow;

@end

@interface SettingsMenuDelegate : NSObject <NSTableViewDelegate, NSTableViewDataSource>
-(id)initWithThreadData:(RGSSThreadData&)data;
@end

#endif /* SettingsMenuController_h */
