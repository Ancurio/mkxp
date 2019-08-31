#if defined(__WIN32__)

#include <windows.h>
#include <stdlib.h>

#else
#include <locale>

#ifdef __APPLE__
extern "C" {
#include <CoreFoundation/CFLocale.h>
#include <CoreFoundation/CFString.h>
}
#endif

#endif

#include <string>
#include "lang-fun.h"

// ======================================================================
// https://github.com/wine-mirror/wine/blob/master/dlls/kernel32/locale.c
// ======================================================================

#ifdef __APPLE__
/***********************************************************************
 *           get_mac_locale
 *
 * Return a locale identifier string reflecting the Mac locale, in a form
 * that parse_locale_name() will understand.  So, strip out unusual
 * things like script, variant, etc.  Or, rather, just construct it as
 * <lang>[_<country>].UTF-8.
 */
static const char* get_mac_locale(void)
{
    static char mac_locale[50];
    
    if (!mac_locale[0])
    {
        CFLocaleRef locale = CFLocaleCopyCurrent();
        CFStringRef lang = (CFStringRef)CFLocaleGetValue( locale, kCFLocaleLanguageCode );
        CFStringRef country = (CFStringRef)CFLocaleGetValue( locale, kCFLocaleCountryCode );
        CFStringRef locale_string;
        
        if (country)
            locale_string = CFStringCreateWithFormat(NULL, NULL, CFSTR("%@_%@"), lang, country);
        else
            locale_string = CFStringCreateCopy(NULL, lang);
        
        CFStringGetCString(locale_string, mac_locale, sizeof(mac_locale), kCFStringEncodingUTF8);
        strcat(mac_locale, ".UTF-8");
        
        CFRelease(locale);
        CFRelease(locale_string);
    }
    
    return mac_locale;
}
#endif

const char *getUserLanguage()
{
    static char buf[50] = {0};
#if defined(__WIN32__)
    wchar_t wbuf[50] = {0};
    LANGID lid = GetUserDefaultLangID();
    LCIDToLocaleName(lid, wbuf, sizeof(wbuf), 0);
    wcstombs(buf, wbuf, sizeof(buf));
#else
    strncpy(buf, std::locale("").name().c_str(), sizeof(buf));
#ifdef __APPLE__
    if (!buf[0]) strncpy(buf, get_mac_locale(), sizeof(buf));
#endif
#endif

    for (int i = 0; i < strlen(buf); i++)
    {
#ifdef __WIN32__
        if (buf[i] == '-')
        {
            buf[i] = '_';
#else
        if (buf[i] == '.')
        {
            buf[i] = 0;
#endif
            break;
        }
    }
    
    return buf;
}


