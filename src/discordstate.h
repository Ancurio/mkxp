#pragma once

#include <discord_game_sdk.h>
#include <string>

#include "bitmap.h"
#ifdef MARIN
#define DEFAULT_CLIENT_ID 624284820201013248
#else
#define DEFAULT_CLIENT_ID 618672572183347211
#endif

struct DiscordStatePrivate;

class DiscordState
{
public:
    DiscordState(RGSSThreadData *rtData);
    ~DiscordState();
    
    IDiscordActivityManager *activityManager();
    IDiscordUserManager *userManager();
    IDiscordImageManager *imageManager();
    
    int update();
    bool isConnected();

    std::string userName();
    std::string userDiscrim();
    DiscordUserId userId();
    
    Bitmap *getAvatar(DiscordUserId userId, int size);
    Bitmap *userAvatar(int size);
    
private:
    DiscordStatePrivate *p;
};
