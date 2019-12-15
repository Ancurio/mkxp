#if defined(__WIN32__)

#import <windows.h>
#import <stdlib.h>

#else
#import <locale>

#ifdef __APPLE__
#import <Foundation/Foundation.h>
#endif

#endif

#import <string>
#import <cstring>
#import "lang-fun.h"

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
NSString* get_mac_locale(void)
{
    NSMutableString* mac_locale = [NSMutableString string];
    
    if (mac_locale != nil)
    {
        NSLocale* locale = [NSLocale currentLocale];
        NSString* lang = [locale languageCode];
        NSString* country = [locale countryCode];
        NSString* locale_string;
        
        if (country != nil)
            locale_string = [NSString stringWithFormat:@"%@_%@", lang, country];
        else
            locale_string = [NSString stringWithString:lang];

        [mac_locale appendFormat:@"%@%@", locale_string, @".UTF-8"];
    }
    return mac_locale;
}
#endif

const char* getUserLanguage()
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
    if (!buf[0]) strncpy(buf, [get_mac_locale() UTF8String], sizeof(buf));
#endif
#endif

    for (int i = 0; (size_t)i < strlen(buf); i++)
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


