#include "binding-util.h"
#include <steam/isteamapps.h>
#include <steam/isteamuserstats.h>

// This is a simple implementation of Steam's user stats.
// Basically, a C++ version of this:
// https://github.com/GMMan/RGSS_SteamUserStatsLite

// May or may not be expanded in the future.

RB_METHOD(CUSLSetStat) {
  RB_UNUSED_PARAM;

  VALUE name, stat;
  rb_scan_args(argc, argv, "2", &name, &stat);
  SafeStringValue(name);

  bool ret;
  if (RB_TYPE_P(stat, RUBY_T_FLOAT)) {
    ret =
        SteamUserStats()->SetStat(RSTRING_PTR(name), (float)RFLOAT_VALUE(stat));
  } else if (RB_TYPE_P(stat, RUBY_T_FIXNUM)) {
    ret = SteamUserStats()->SetStat(RSTRING_PTR(name), (int)NUM2INT(stat));
  } else {
    rb_raise(rb_eTypeError,
             "Statistic value must be either an integer or float.");
  }
  return rb_bool_new(ret);
}

RB_METHOD(CUSLGetStat) {
  RB_UNUSED_PARAM;

  VALUE name;
  rb_scan_args(argc, argv, "1", &name);
  SafeStringValue(name);

  int resi;
  float resf;

  if (SteamUserStats()->GetStat(RSTRING_PTR(name), &resi)) {
    return INT2NUM(resi);
  } else if (SteamUserStats()->GetStat(RSTRING_PTR(name), &resf)) {
    return rb_float_new(resf);
  } else {
    return Qnil;
  }
}

RB_METHOD(CUSLUpdateAvgRateStat) {
  RB_UNUSED_PARAM;

  VALUE name, sessioncount, sessionlength;
  rb_scan_args(argc, argv, "3", &name, &sessioncount, &sessionlength);
  SafeStringValue(name);
  Check_Type(sessioncount, RUBY_T_FLOAT);
  Check_Type(sessionlength, RUBY_T_FLOAT);

  bool ret = SteamUserStats()->UpdateAvgRateStat(
      RSTRING_PTR(name), RFLOAT_VALUE(sessioncount), NUM2DBL(sessionlength));

  return rb_bool_new(ret);
}

RB_METHOD(CUSLGetAchievement) {
  RB_UNUSED_PARAM;

  VALUE name;
  rb_scan_args(argc, argv, "1", &name);
  SafeStringValue(name);

  bool ret;
  bool valid = SteamUserStats()->GetAchievement(RSTRING_PTR(name), &ret);

  if (!valid)
    return Qnil;

  return rb_bool_new(ret);
}

RB_METHOD(CUSLSetAchievement) {
  RB_UNUSED_PARAM;

  VALUE name;
  rb_scan_args(argc, argv, "1", &name);
  SafeStringValue(name);

  bool ret = SteamUserStats()->SetAchievement(RSTRING_PTR(name));
  return rb_bool_new(ret);
}

RB_METHOD(CUSLClearAchievement) {
  RB_UNUSED_PARAM;

  VALUE name;

  rb_scan_args(argc, argv, "1", &name);
  SafeStringValue(name);

  bool ret = SteamUserStats()->ClearAchievement(RSTRING_PTR(name));
  return rb_bool_new(ret);
}

RB_METHOD(CUSLGetAchievementAndUnlockTime) {
  RB_UNUSED_PARAM;

  VALUE name;

  rb_scan_args(argc, argv, "1", &name);
  SafeStringValue(name);

  bool achieved;
  uint time;

  if (!SteamUserStats()->GetAchievementAndUnlockTime(RSTRING_PTR(name),
                                                     &achieved, &time))
    return Qnil;

  VALUE ret = rb_ary_new();
  rb_ary_push(ret, rb_bool_new(achieved));

  if (!time) {
    rb_ary_push(ret, Qnil);
    return ret;
  }

  VALUE t = rb_funcall(rb_cTime, rb_intern("at"), 1, UINT2NUM(time));
  rb_ary_push(ret, t);
  return ret;
}

RB_METHOD(CUSLGetAchievementDisplayAttribute) {
  RB_UNUSED_PARAM;

  VALUE name, key;
  rb_scan_args(argc, argv, "2", &name, &key);
  SafeStringValue(name);
  SafeStringValue(key);

  const char *ret = SteamUserStats()->GetAchievementDisplayAttribute(
      RSTRING_PTR(name), RSTRING_PTR(key));

  return rb_str_new_cstr(ret);
}

RB_METHOD(CUSLIndicateAchievementProgress) {
  RB_UNUSED_PARAM;

  VALUE name, progress, max;

  rb_scan_args(argc, argv, "3", &name, &progress, &max);
  SafeStringValue(name);

  bool ret = SteamUserStats()->IndicateAchievementProgress(
      RSTRING_PTR(name), NUM2UINT(progress), NUM2UINT(max));

  if (ret)
    return rb_bool_new(SteamUserStats()->StoreStats());

  return rb_bool_new(ret);
}

RB_METHOD(CUSLGetNumAchievements) {
  RB_UNUSED_PARAM;

  rb_check_argc(argc, 0);

  return UINT2NUM(SteamUserStats()->GetNumAchievements());
}

RB_METHOD(CUSLGetAchievementName) {
  RB_UNUSED_PARAM;

  VALUE index;

  rb_scan_args(argc, argv, "1", &index);

  return rb_str_new_cstr(SteamUserStats()->GetAchievementName(NUM2UINT(index)));
}

RB_METHOD(CUSLStoreStats) {
  RB_UNUSED_PARAM;

  rb_check_argc(argc, 0);

  return rb_bool_new(SteamUserStats()->StoreStats());
}

RB_METHOD(CUSLResetAllStats) {
  RB_UNUSED_PARAM;

  bool achievementsToo;

  rb_get_args(argc, argv, "b", &achievementsToo);

  return rb_bool_new(SteamUserStats()->ResetAllStats(achievementsToo));
}

RB_METHOD(CUSLIsSubscribed) {
  RB_UNUSED_PARAM;

  rb_check_argc(argc, 0);

  return rb_bool_new(SteamApps()->BIsSubscribed());
}

RB_METHOD(CUSLIsDlcInstalled) {
  RB_UNUSED_PARAM;

  VALUE appid;
  rb_scan_args(argc, argv, "1", &appid);

  return rb_bool_new(SteamApps()->BIsDlcInstalled(NUM2UINT(appid)));
}

void CUSLBindingInit() {

  SteamUserStats()->RequestCurrentStats();

  VALUE mSteamLite = rb_define_module("SteamLite");

  _rb_define_module_function(mSteamLite, "subscribed?", CUSLIsSubscribed);
  _rb_define_module_function(mSteamLite, "dlc_installed?", CUSLIsDlcInstalled);

  _rb_define_module_function(mSteamLite, "get_stat", CUSLGetStat);
  _rb_define_module_function(mSteamLite, "set_stat", CUSLSetStat);

  _rb_define_module_function(mSteamLite, "update_avg_rate_stat",
                             CUSLUpdateAvgRateStat);
  _rb_define_module_function(mSteamLite, "get_achievement", CUSLGetAchievement);
  _rb_define_module_function(mSteamLite, "set_achievement", CUSLSetAchievement);
  _rb_define_module_function(mSteamLite, "clear_achievement",
                             CUSLClearAchievement);
  _rb_define_module_function(mSteamLite, "get_achievement_and_unlock_time",
                             CUSLGetAchievementAndUnlockTime);
  _rb_define_module_function(mSteamLite, "get_achievement_display_attribute",
                             CUSLGetAchievementDisplayAttribute);
  _rb_define_module_function(mSteamLite, "indicate_achievement_progress",
                             CUSLIndicateAchievementProgress);
  _rb_define_module_function(mSteamLite, "get_num_achievements",
                             CUSLGetNumAchievements);
  _rb_define_module_function(mSteamLite, "get_achievement_name",
                             CUSLGetAchievementName);
  _rb_define_module_function(mSteamLite, "reset_all_stats", CUSLResetAllStats);
}
