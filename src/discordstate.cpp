#include <discord_game_sdk.h>

#include "sharedstate.h"
#include "eventthread.h"
#include "discordstate.h"
#include "exception.h"
#include "debugwriter.h"
#include "bitmap.h"

struct Application {
    struct IDiscordCore* core;
    struct IDiscordUserManager* users;
    struct IDiscordAchievementManager* achievements;
    struct IDiscordActivityManager* activities;
    struct IDiscordRelationshipManager* relationships;
    struct IDiscordApplicationManager* application;
    struct IDiscordLobbyManager* lobbies;
    DiscordUserId user_id;
};

struct DiscordStatePrivate
{
    DiscordClientId clientId;

    IDiscordCore *core;
    
    Application app;
    IDiscordActivityEvents activityEvents;
    IDiscordUserEvents userEvents;
    
    DiscordUser currentUser;
    
    bool discordInstalled;
    bool connected;
    bool userPresent;
    
    DiscordStatePrivate()
        : discordInstalled(false),
          connected(false),
          userPresent(false)
    {};
    
    ~DiscordStatePrivate()
    {
        if (core) core->destroy(core);
    }
};

void onCurrentUserUpdateCb(void *event_data)
{
    DiscordStatePrivate *p = (DiscordStatePrivate*)event_data;
    p->app.users->get_current_user(p->app.users, &p->currentUser);
    p->userPresent = true;
}

void onActivityJoinCb(void *event_data, const char *secret)
{
    
}

void onActivitySpectateCb(void *event_data, const char *secret)
{
    
}

void onActivityJoinRequestCb(void *event_data, struct DiscordUser *user)
{
    
}

void onActivityInviteRequestCb(void *event_data, enum EDiscordActivityActionType type, struct DiscordUser *user, struct DiscordActivity *activity)
{
    
}

void discordLogHook(void *hook_data, enum EDiscordLogLevel level, const char *message)
{
    Debug() << "DISCORD:" << message;
}

int discordTryConnect(DiscordStatePrivate *p)
{
    DiscordCreateParams params{};
    DiscordCreateParamsSetDefault(&params);
    
    params.client_id = p->clientId;
    params.flags = DiscordCreateFlags_NoRequireDiscord;
    params.event_data = (void*)p;
    
    p->activityEvents.on_activity_join = onActivityJoinCb;
    p->activityEvents.on_activity_spectate = onActivitySpectateCb;
    p->activityEvents.on_activity_join_request = onActivityJoinRequestCb;
    p->activityEvents.on_activity_invite = onActivityInviteRequestCb;
    params.activity_events = &p->activityEvents;
    
    
    p->userEvents.on_current_user_update = onCurrentUserUpdateCb;
    params.user_events = &p->userEvents;
    
    int rc = DiscordCreate(DISCORD_VERSION, &params, &p->core);
    
    if (rc != DiscordResult_NotInstalled)
        p->discordInstalled = true;

    if (rc != DiscordResult_Ok)
        return rc;
    
    p->core->set_log_hook(p->core, DiscordLogLevel_Debug, (void*)p, discordLogHook);
    
    p->app.activities = p->core->get_activity_manager(p->core);
    p->app.users = p->core->get_user_manager(p->core);
    
    p->connected = true;
    
    return rc;
}

DiscordState::DiscordState(DiscordClientId clientId, int *result)
{
    p = new DiscordStatePrivate();
    memset(&p->app, 0, sizeof(Application));
    p->clientId = clientId;
    int rc = discordTryConnect(p);
    if (result) *result = rc;
}

DiscordState::~DiscordState()
{
    delete p;
}

IDiscordActivityManager *DiscordState::activityManager()
{
    return p->app.activities;
}

IDiscordUserManager *DiscordState::userManager()
{
    return p->app.users;
}

int DiscordState::update()
{
    if (!p->discordInstalled) return DiscordResult_NotInstalled;
    
    if (p->connected)
    {
        int rc = p->core->run_callbacks(p->core);
        if (rc == DiscordResult_NotRunning)
        {
            p->connected = false;
            memset(&p->currentUser, 0, sizeof(DiscordUser));
            memset(&p->app, 0, sizeof(Application));
            p->userPresent = false;
            return rc;
        }
        return rc;
    }
    
    return discordTryConnect(p);
}

bool DiscordState::isConnected()
{
    return p->connected;
}

std::string DiscordState::userName()
{
    std::string ret; ret.clear();
    if (p->userPresent) ret = p->currentUser.username;
    return ret;
}

std::string DiscordState::userDiscrim()
{
    std::string ret; ret.clear();
    if (p->userPresent) ret = p->currentUser.discriminator;
    return ret;
}

DiscordUserId DiscordState::userId()
{
    return (p->userPresent) ? p->currentUser.id : 0;
}

// NYI
Bitmap *userAvatar()
{
    Bitmap *ret = new Bitmap(256, 256);
    
    return ret;
}
