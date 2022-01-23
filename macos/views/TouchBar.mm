//
//  TouchBar.mm
//  mkxp-z
//
//  Created by ゾロア on 1/14/22.
//

#import <AppKit/AppKit.h>
#import <SDL_syswm.h>
#import <SDL_events.h>
#import <SDL_timer.h>
#import <SDL_scancode.h>

#import "TouchBar.h"
#import "sharedstate.h"
#import "eventthread.h"

#import "display/graphics.h"
#import "config.h"

MKXPZTouchBar *_sharedTouchBar;

@interface MKXPZTouchBar () {
    NSWindow *_parent;
    NSTextField *fpsLabel;
    NSSegmentedControl *functionKeys;
}

@property (retain,nonatomic) NSString *gameTitle;

-(void)updateFPSDisplay:(uint32_t)value;
@end

@implementation MKXPZTouchBar

@synthesize gameTitle;

+(MKXPZTouchBar*)sharedTouchBar {
    if (!_sharedTouchBar)
        _sharedTouchBar = [MKXPZTouchBar new];
    return _sharedTouchBar;
}

-(instancetype)init {
    self = [super init];
    self.delegate = self;
    self.defaultItemIdentifiers = @[@"function", NSTouchBarItemIdentifierFlexibleSpace, @"icon", @"fps", NSTouchBarItemIdentifierFlexibleSpace, @"rebind", @"reset"];
    
    fpsLabel = [NSTextField labelWithString:@"Loading..."];
    fpsLabel.alignment = NSTextAlignmentCenter;
    fpsLabel.font = [NSFont labelFontOfSize:NSFont.smallSystemFontSize];
    
    functionKeys = [NSSegmentedControl segmentedControlWithLabels:@[@"F5", @"F6", @"F7", @"F8", @"F9"] trackingMode:NSSegmentSwitchTrackingMomentary target:self action:@selector(simFunctionKey)];
    functionKeys.segmentStyle = NSSegmentStyleSeparated;
    
    return self;
}

-(NSWindow*)parent {
    return _parent;
}

-(NSWindow*)setParent:(NSWindow*)window {
    _parent = window;
    return _parent;
}

-(NSTouchBarItem *)touchBar:(NSTouchBar *)touchBar makeItemForIdentifier:(NSTouchBarItemIdentifier)identifier {
    NSCustomTouchBarItem *ret = [[NSCustomTouchBarItem alloc] initWithIdentifier:identifier];
    if ([identifier isEqualToString:@"reset"]) {
        ret.view = [NSButton buttonWithImage:[NSImage imageNamed:@"gobackward"] target:self action:@selector(simF12)];
        
        ((NSButton*)ret.view).bezelColor = [NSColor colorWithRed:0xac/255.0 green:0x14/255.0 blue:0x01/255.0 alpha:1.0];
    }
    else if ([identifier isEqualToString:@"rebind"]) {
        ret.view = [NSButton buttonWithImage:[NSImage imageNamed:@"gear"] target:self action:@selector(openSettingsMenu)];
    }
    else if ([identifier isEqualToString:@"icon"]) {
        NSImage *appIcon = [NSWorkspace.sharedWorkspace iconForFile:NSBundle.mainBundle.bundlePath];
        appIcon.size = CGSizeMake(30,30);
        ret.view = [NSImageView imageViewWithImage:appIcon];
        
    }
    else if ([identifier isEqualToString:@"fps"]) {
        ret.view = fpsLabel;
    }
    else if ([identifier isEqualToString:@"function"]) {
        ret.view = functionKeys;
    }
    else {
        // hopefully this should get the compiler to free the touchbaritem here
        ret = nil;
        return nil;
    }
    return ret;
}

-(void)updateFPSDisplay:(uint32_t)value {
    if (fpsLabel) {
        int targetFrameRate = shState->graphics().getFrameRate();
        dispatch_async(dispatch_get_main_queue(), ^{
            self->fpsLabel.stringValue = [NSString stringWithFormat:@"%@\n%i FPS (%i%%)", self.gameTitle, value, (int)((float)value / (float)targetFrameRate * 100)];
        });
    }
}

-(void)simF12 {
    [self simulateKeypress:SDL_SCANCODE_F12];
}

-(void)simFunctionKey {
    [self simulateKeypress:(SDL_Scancode)(SDL_SCANCODE_F5 + functionKeys.selectedSegment)];
}

-(void)simulateKeypress:(SDL_Scancode)scancode {
    [self simulateKeyDown:scancode];
    double afr = shState->graphics().averageFrameRate();
    afr = (afr >= 1) ? afr : 1;
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, 1000000 * afr), dispatch_get_main_queue(), ^{
        [self simulateKeyUp:scancode];
    });
}

-(void)simulateKeyDown:(SDL_Scancode)scancode {
    SDL_Event e;
    e.key.type = SDL_KEYDOWN;
    e.key.state = SDL_PRESSED;
    e.key.timestamp = SDL_GetTicks();
    e.key.keysym.scancode = scancode;
    e.key.keysym.sym = SDL_GetKeyFromScancode(scancode);
    e.key.windowID = 1;
    SDL_PushEvent(&e);
}

-(void)simulateKeyUp:(SDL_Scancode)scancode {
    SDL_Event e;
    e.key.type = SDL_KEYUP;
    e.key.state = SDL_RELEASED;
    e.key.timestamp = SDL_GetTicks();
    e.key.keysym.scancode = scancode;
    e.key.keysym.sym = SDL_GetKeyFromScancode(scancode);
    e.key.windowID = 1;
    SDL_PushEvent(&e);
}

-(void)openSettingsMenu {
    shState->eThread().requestSettingsMenu();
}

@end

void initTouchBar(SDL_Window *win, Config &conf) {
    SDL_SysWMinfo windowinfo{};
    SDL_GetWindowWMInfo(win, &windowinfo);
    
    windowinfo.info.cocoa.window.touchBar = MKXPZTouchBar.sharedTouchBar;
    MKXPZTouchBar.sharedTouchBar.parent = windowinfo.info.cocoa.window;
    MKXPZTouchBar.sharedTouchBar.gameTitle = @(conf.game.title.c_str());
}

void updateTouchBarFPSDisplay(uint32_t value) {
    [MKXPZTouchBar.sharedTouchBar updateFPSDisplay:value];
}
