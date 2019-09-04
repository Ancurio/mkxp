#include <discord_game_sdk.h>

#include "discordstate.h"
#include "sharedstate.h"
#include "binding-util.h"

//NYI
void discordResultCb(void *callback_data, enum EDiscordResult result)
{
    
}

#ifndef OLD_RUBY
DEF_TYPE_CUSTOMFREE(DCActivity, free);
#else
DEF_ALLOCFUNC_CUSTOMFREE(DCActivity, free);
#endif

RB_METHOD(DiscordConnected)
{
    RB_UNUSED_PARAM;
    
    return rb_bool_new(shState->discord().isConnected());
}

RB_METHOD(DiscordGetUsername)
{
    RB_UNUSED_PARAM;
    
    return rb_str_new_cstr(shState->discord().userName().c_str());
}

RB_METHOD(DiscordGetDiscriminator)
{
    RB_UNUSED_PARAM;
    
    return rb_str_new_cstr(shState->discord().userDiscrim().c_str());
}

RB_METHOD(DiscordGetUserId)
{
    RB_UNUSED_PARAM;
    
    return LL2NUM(shState->discord().userId());
}

RB_METHOD(DiscordActivitySend)
{
    RB_UNUSED_PARAM;
    IDiscordActivityManager *am = shState->discord().activityManager();
    
    DiscordActivity *activity = getPrivateData<DiscordActivity>(self);
    
    if (am) am->update_activity(am, activity, 0, discordResultCb);
    
    return Qnil;
}

RB_METHOD(DiscordActivityInitialize)
{
    RB_UNUSED_PARAM;
    
    DiscordActivity *activity = ALLOC(DiscordActivity);
    setPrivateData(self, activity);
    
    memset(activity, 0, sizeof(DiscordActivity));
    
    activity->type = DiscordActivityType_Playing;
    
    if (rb_block_given_p())
    {
        rb_yield(self);
        DiscordActivitySend(0, 0, self);
    };
    return self;
}

RB_METHOD(DiscordActivityClear)
{
    RB_UNUSED_PARAM;
    IDiscordActivityManager *am = shState->discord().activityManager();
    if (am) am->clear_activity(am, 0, discordResultCb);
    
    return Qnil;
}

#define DEF_DCPROP_ACTPARTYSZ(n) \
RB_METHOD(DiscordActivityGetParty##n) \
{ \
RB_UNUSED_PARAM; \
DiscordActivity *p = getPrivateData<DiscordActivity>(self); \
return INT2NUM(p->party.size.n); \
} \
RB_METHOD(DiscordActivitySetParty##n) \
{ \
RB_UNUSED_PARAM; \
int num; \
rb_get_args(argc, argv, "i", &num); \
DiscordActivity *p = getPrivateData<DiscordActivity>(self); \
p->party.size.n = num; \
return INT2NUM(num); \
}

DEF_DCPROP_ACTPARTYSZ(current_size);
DEF_DCPROP_ACTPARTYSZ(max_size);

#define DEF_DCPROP_S(basename, propname, maxsz) \
RB_METHOD(Discord##basename##Get##propname) \
{ \
RB_UNUSED_PARAM; \
Discord##basename *p = getPrivateData<Discord##basename>(self); \
return rb_str_new_cstr(p->propname); \
} \
RB_METHOD(Discord##basename##Set##propname) \
{ \
RB_UNUSED_PARAM; \
VALUE str; \
rb_scan_args(argc, argv, "1", &str); \
SafeStringValue(str); \
Discord##basename *p = getPrivateData<Discord##basename>(self); \
strncpy(p->propname, RSTRING_PTR(str), maxsz); \
return str; \
}

#define DEF_DCPROP_S_SUB(basename, subname, propname, maxsz) \
RB_METHOD(Discord##basename##Get##subname##_##propname) \
{ \
RB_UNUSED_PARAM; \
Discord##basename *p = getPrivateData<Discord##basename>(self); \
return rb_str_new_cstr(p->subname.propname); \
} \
RB_METHOD(Discord##basename##Set##subname##_##propname) \
{ \
RB_UNUSED_PARAM; \
VALUE str; \
rb_scan_args(argc, argv, "1", &str); \
SafeStringValue(str); \
Discord##basename *p = getPrivateData<Discord##basename>(self); \
strncpy(p->subname.propname, RSTRING_PTR(str), maxsz); \
return str; \
}

#define DEF_DCPROP_I(basename, propname, convtype, cast) \
RB_METHOD(Discord##basename##Get##propname) \
{ \
RB_UNUSED_PARAM; \
Discord##basename *p = getPrivateData<Discord##basename>(self); \
return convtype##2NUM(p->propname); \
} \
RB_METHOD(Discord##basename##Set##propname) \
{ \
RB_UNUSED_PARAM; \
VALUE num; \
rb_scan_args(argc, argv, "1", &num); \
Discord##basename *p = getPrivateData<Discord##basename>(self); \
p->propname = (cast)NUM2##convtype(num); \
return num; \
}

#define DEF_DCPROP_I_SUB(basename, subname, propname, convtype, cast) \
RB_METHOD(Discord##basename##Get##subname##_##propname) \
{ \
RB_UNUSED_PARAM; \
Discord##basename *p = getPrivateData<Discord##basename>(self); \
return convtype##2NUM(p->subname.propname); \
} \
RB_METHOD(Discord##basename##Set##subname##_##propname) \
{ \
RB_UNUSED_PARAM; \
VALUE num; \
rb_scan_args(argc, argv, "1", &num); \
Discord##basename *p = getPrivateData<Discord##basename>(self); \
p->subname.propname = (cast)NUM2##convtype(num); \
return num; \
}

#define DEF_DCPROP_B(basename, propname) \
RB_METHOD(Discord##basename##Get##propname) \
{ \
RB_UNUSED_PARAM; \
Discord##basename *p = getPrivateData<Discord##basename>(self); \
return rb_bool_new(p->propname); \
} \
RB_METHOD(Discord##basename##Set##propname) \
{ \
RB_UNUSED_PARAM; \
bool b; \
rb_get_args(argc, argv, "b", &b RB_ARG_END); \
Discord##basename *p = getPrivateData<Discord##basename>(self); \
p->propname = b; \
return rb_bool_new(b); \
}

#define DEF_DCPROP_B_SUB(basename, propname, subname) \
RB_METHOD(Discord##basename##Get##propname) \
{ \
RB_UNUSED_PARAM; \
Discord##basename *p = getPrivateData<Discord##basename>(self); \
return rb_bool_new(p->subname.propname); \
} \
RB_METHOD(Discord##basename##Set##propname) \
{ \
RB_UNUSED_PARAM; \
bool b; \
rb_get_args(argc, argv, "b", &b RB_ARG_END); \
Discord##basename *p = getPrivateData<Discord##basename>(self); \
p->subname.propname = b; \
return rb_bool_new(b); \
}

DEF_DCPROP_S(Activity, state, 128);
DEF_DCPROP_S(Activity, details, 128);
DEF_DCPROP_S_SUB(Activity, assets, large_image, 128);
DEF_DCPROP_S_SUB(Activity, assets, large_text, 128);
DEF_DCPROP_S_SUB(Activity, assets, small_image, 128);
DEF_DCPROP_S_SUB(Activity, assets, small_text, 128);
DEF_DCPROP_S_SUB(Activity, secrets, match, 128);
DEF_DCPROP_S_SUB(Activity, secrets, join, 128);
DEF_DCPROP_S_SUB(Activity, secrets, spectate, 128);
DEF_DCPROP_S_SUB(Activity, party, id, 128);

DEF_DCPROP_I(Activity, type, INT, EDiscordActivityType);
DEF_DCPROP_I_SUB(Activity, timestamps, start, LL, DiscordTimestamp);
DEF_DCPROP_I_SUB(Activity, timestamps, end, LL, DiscordTimestamp);
DEF_DCPROP_B(Activity, instance);

#define BIND_DCPROP(basename, b, f) \
_rb_define_method(activityClass, b, Discord##basename##Get##f); \
_rb_define_method(activityClass, b "=", Discord##basename##Set##f);


void DiscordBindingInit()
{
    VALUE mod = rb_define_module("Discord");
    _rb_define_module_function(mod, "connected?", DiscordConnected);
    _rb_define_module_function(mod, "user_name", DiscordGetUsername);
    _rb_define_module_function(mod, "user_discriminator", DiscordGetDiscriminator);
    _rb_define_module_function(mod, "user_id", DiscordGetUserId);
    
    VALUE activityClass = rb_define_class_under(mod, "Activity", rb_cObject);
#ifndef OLD_RUBY
    rb_define_alloc_func(activityClass, classAllocate<&DCActivityType>);
#else
    rb_define_alloc_func(activityClass, DCActivityAllocate);
#endif
    
    _rb_define_method(activityClass, "initialize", DiscordActivityInitialize);
    
    rb_define_class_method(activityClass, "clear", DiscordActivityClear);
    
    _rb_define_method(activityClass, "send", DiscordActivitySend);
    
    BIND_DCPROP(Activity, "state", state);
    BIND_DCPROP(Activity, "details", details);
    BIND_DCPROP(Activity, "large_image", assets_large_image);
    BIND_DCPROP(Activity, "large_text", assets_large_text);
    BIND_DCPROP(Activity, "small_image", assets_small_image);
    BIND_DCPROP(Activity, "small_text", assets_small_text);
    BIND_DCPROP(Activity, "match_secret", secrets_match);
    BIND_DCPROP(Activity, "join_secret", secrets_join);
    BIND_DCPROP(Activity, "spectate_secret", secrets_spectate);
    BIND_DCPROP(Activity, "party_id", party_id);
    BIND_DCPROP(Activity, "party_currentsize", Partycurrent_size);
    BIND_DCPROP(Activity, "party_maxsize", Partymax_size);
    
    BIND_DCPROP(Activity, "type", type);
    BIND_DCPROP(Activity, "start_time", timestamps_start);
    BIND_DCPROP(Activity, "end_time", timestamps_end);
    BIND_DCPROP(Activity, "instance", instance);
    
}
