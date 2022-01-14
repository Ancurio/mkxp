//
//  TouchBar.h
//  mkxp-z
//
//  Created by ゾロア on 1/14/22.
//

#ifndef TouchBar_h
#define TouchBar_h

#include <stdio.h>

#include <SDL_events.h>
#include <SDL_video.h>

#include "config.h"

#ifdef __OBJC__
@interface MKXPZTouchBar : NSTouchBar <NSTouchBarDelegate>
+(MKXPZTouchBar*)sharedTouchBar;
@end
#endif

void initTouchBar(SDL_Window *win, Config &conf);
void updateTouchBarFPSDisplay(uint32_t value);

#endif /* TouchBar_h */
