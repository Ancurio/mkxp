#include "config.h"

#include <stdlib.h>
#include <fstream>
#include <string>
#include <unistd.h>
#include <boost/algorithm/string.hpp>

inline void ensure_trailing_slash(std::string& path)
{
	if (path.size() && path[path.size()-1] != '/') {
		path += "/";
	}
}

static std::string find_xdg_config_dir()
{
	std::string path;
	const char* env = getenv("XDG_CONFIG_HOME");
	if (env) {
		path = env;
		ensure_trailing_slash(path);
	} else {
		env = getenv("HOME");
		if (!env) {
			return "";
		}
		path = env;
		ensure_trailing_slash(path);
		path += ".config/";
	}
	return path;
}

static std::string get_desktop_dir()
{
	std::string desktopPath;

	std::string cfg_dir = find_xdg_config_dir();
	if (!cfg_dir.empty()) {
		std::string cfg_file = cfg_dir + "user-dirs.dirs";
		if (access(cfg_file.c_str(), F_OK) == 0) {
			std::ifstream fin(cfg_file.c_str());
			std::string line;
			while (std::getline(fin, line)) {
				/* skip blank and comment lines */
				if (line.empty() || line[0] == '#') continue;

				std::vector<std::string> parts;
				boost::split(parts, line, boost::is_any_of("="));
				if (parts.size() == 2) {
					std::string key = boost::trim_copy(parts[0]);
					if (key == "XDG_DESKTOP_DIR") {
						std::string val = boost::trim_copy_if(parts[1], boost::is_any_of(" \n\r\t\""));
						desktopPath = val;
						break;
					}
				}
			}
		}
	}
	const char* home = getenv("HOME");
	if (home) {
		size_t pos = desktopPath.find("$HOME");
		if (pos != desktopPath.npos) {
			desktopPath.replace(pos, pos + 5, home);
		}
		pos = desktopPath.find("${HOME}");
		if (pos != desktopPath.npos) {
			desktopPath.replace(pos, pos + 7, home);
		}
	}
	if (desktopPath.empty() || access(desktopPath.c_str(), F_OK) != 0) {
		if (home) {
			desktopPath = home;
			ensure_trailing_slash(desktopPath);
			if (access((desktopPath + "Desktop").c_str(), F_OK) == 0) {
				desktopPath += "Desktop/";
			}
		} else {
			desktopPath = "";
		}
	}
	ensure_trailing_slash(desktopPath);
	return desktopPath;
}

void Config::setupPaths()
{
	desktopPath = get_desktop_dir();
}