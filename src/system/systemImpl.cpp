//
//  systemImpl.cpp
//  Player
//
//  Created by ゾロアーク on 11/22/20.
//

#include "system.h"

#if defined(__WIN32__)
#include <stdlib.h>
#include <windows.h>
#else
#include <locale>
#endif

#include <cstring>
#include <string>

std::string systemImpl::getSystemLanguage() {
    static char buf[50] = {0};
#if defined(__WIN32__)
    wchar_t wbuf[50] = {0};
    LANGID lid = GetUserDefaultLangID();
    LCIDToLocaleName(lid, wbuf, sizeof(wbuf), 0);
    wcstombs(buf, wbuf, sizeof(buf));
#else
    strncpy(buf, std::locale("").name().c_str(), sizeof(buf));
#endif
    
    for (int i = 0; (size_t)i < strlen(buf); i++) {
#ifdef __WIN32__
        if (buf[i] == '-') {
            buf[i] = '_';
#else
        if (buf[i] == '.') {
            buf[i] = 0;
#endif
            break;
        }
    }
    return std::string(buf);
}

std::string systemImpl::getUserName() {
    char ret[30];
#ifdef __WINDOWS__
    GetUserName(ret, sizeof(ret));
#else
    char *username = getenv("USER");
    if (username)
        strncpy(ret, username, sizeof(ret));
#endif
    return std::string(ret);
    
}