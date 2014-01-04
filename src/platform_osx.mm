#include "config.h"

#include <string>

#import <Foundation/Foundation.h>

static void ensure_trailing_slash(std::string& path)
{
	if (path.size() && path[path.size()-1] != '/') {
		path += "/";
	}
}

static std::string get_desktop_dir()
{
	std::string ret;
	NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];

	NSArray *paths = NSSearchPathForDirectoriesInDomains(NSDesktopDirectory, NSUserDomainMask, YES);
	NSString *path = [paths objectAtIndex: 0];

	ret = [path fileSystemRepresentation];

	[pool drain];

	ensure_trailing_slash(ret);
	return ret;
}


void Config::setupPaths()
{
	desktopPath = get_desktop_dir();
}

