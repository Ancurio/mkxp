#pragma once

#include <discord_game_sdk.h>
#include <string>

#include "bitmap.h"

#define DEFAULT_CLIENT_ID 618672572183347211

struct DiscordStatePrivate;

class DiscordState
{
public:
    DiscordState(RGSSThreadData *rtData);
    ~DiscordState();
    
    IDiscordActivityManager *activityManager();
    IDiscordUserManager *userManager();
    
    int update();
    bool isConnected();

    std::string userName();
    std::string userDiscrim();
    DiscordUserId userId();
    Bitmap *userAvatar();
    
private:
    DiscordStatePrivate *p;
};
