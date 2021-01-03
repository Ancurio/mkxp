//
//  steamshim_mac_helpers.mm
//  shim
//
//  Created by ゾロアーク on 1/3/21.
//

#import <Foundation/Foundation.h>
#import "steamshim_mac_helpers.h"

std::string execPath() {
    std::string ret([NSString pathWithComponents:@[
        NSBundle.mainBundle.executablePath,
        @(GAME_LAUNCH_NAME)
    ]].UTF8String);
    
}
