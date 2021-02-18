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


@interface SettingsMenu : NSViewController <NSTableViewDelegate, NSTableViewDataSource>

+(SettingsMenu*)openWindow;

-(SettingsMenu*)setWindow:(NSWindow*)window;
-(SettingsMenu*)raise;
@end

#endif /* SettingsMenuController_h */
