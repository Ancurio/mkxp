/*
** binding-mri.cpp
**
** This file is part of mkxp.
**
** Copyright (C) 2013 Jonas Kulla <Nyocurio@gmail.com>
**
** mkxp is free software: you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation, either version 2 of the License, or
** (at your option) any later version.
**
** mkxp is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with mkxp.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "audio.h"
#include "binding-util.h"
#include "binding.h"
#include "debugwriter.h"
#include "eventthread.h"
#include "filesystem.h"
#include "graphics.h"
#include "lang-fun.h"
#include "sdl-util.h"
#include "sharedstate.h"
#include "src/config.h"
#include "src/util.h"

#include "boost-hash.h"
#include <vector>

extern "C" {
#include <ruby.h>

#if RAPI_FULL >= 190
#include <ruby/encoding.h>
#endif
}

#include <assert.h>
#include <string>
#include <zlib.h>

#include <SDL_cpuinfo.h>
#include <SDL_filesystem.h>
#include <SDL_power.h>

#ifdef __WIN32__
#define NULL_IO "NUL"
#else
#define NULL_IO "/dev/null"
#endif

extern const char module_rpg1[];
extern const char module_rpg2[];
extern const char module_rpg3[];

#include "EssentialsTilemapHack.rb.xxd"

static void mriBindingExecute();
static void mriBindingTerminate();
static void mriBindingReset();

ScriptBinding scriptBindingImpl = {mriBindingExecute, mriBindingTerminate,
                                   mriBindingReset};

ScriptBinding *scriptBinding = &scriptBindingImpl;

void tableBindingInit();
void etcBindingInit();
void fontBindingInit();
void bitmapBindingInit();
void spriteBindingInit();
void viewportBindingInit();
void planeBindingInit();
void windowBindingInit();
void tilemapBindingInit();
void windowVXBindingInit();
void tilemapVXBindingInit();

void inputBindingInit();
void audioBindingInit();
void graphicsBindingInit();

void fileIntBindingInit();

#ifdef USE_MINIFFI
void MiniFFIBindingInit();
#endif

#ifdef HAVE_STEAMSHIM
void CUSLBindingInit();
#endif

RB_METHOD(mriPrint);
RB_METHOD(mriP);
RB_METHOD(mkxpDataDirectory);
RB_METHOD(mkxpSetTitle);
RB_METHOD(mkxpPuts);
RB_METHOD(mkxpRawKeyStates);
RB_METHOD(mkxpMouseInWindow);
RB_METHOD(mkxpPlatform);
RB_METHOD(mkxpUserLanguage);
RB_METHOD(mkxpGameTitle);
RB_METHOD(mkxpPowerState);
RB_METHOD(mkxpSettingsMenu);
RB_METHOD(mkxpCpuCount);
RB_METHOD(mkxpSystemMemory);

RB_METHOD(mriRgssMain);
RB_METHOD(mriRgssStop);
RB_METHOD(_kernelCaller);

static void mriBindingInit() {
  tableBindingInit();
  etcBindingInit();
  fontBindingInit();
  bitmapBindingInit();
  spriteBindingInit();
  viewportBindingInit();
  planeBindingInit();

  if (rgssVer == 1) {
    windowBindingInit();
    tilemapBindingInit();
  } else {
    windowVXBindingInit();
    tilemapVXBindingInit();
  }

  inputBindingInit();
  audioBindingInit();
  graphicsBindingInit();

  fileIntBindingInit();

#ifdef USE_MINIFFI
  MiniFFIBindingInit();
#endif

#ifdef HAVE_STEAMSHIM
  CUSLBindingInit();
#endif

  if (rgssVer >= 3) {
    _rb_define_module_function(rb_mKernel, "rgss_main", mriRgssMain);
    _rb_define_module_function(rb_mKernel, "rgss_stop", mriRgssStop);

    _rb_define_module_function(rb_mKernel, "msgbox", mriPrint);
    _rb_define_module_function(rb_mKernel, "msgbox_p", mriP);

    rb_define_global_const("RGSS_VERSION", rb_str_new_cstr("3.0.1"));
  } else {
    _rb_define_module_function(rb_mKernel, "print", mriPrint);
    _rb_define_module_function(rb_mKernel, "p", mriP);

    rb_define_alias(rb_singleton_class(rb_mKernel), "_mkxp_kernel_caller_alias",
                    "caller");
    _rb_define_module_function(rb_mKernel, "caller", _kernelCaller);
  }

  if (rgssVer == 1)
    rb_eval_string(module_rpg1);
  else if (rgssVer == 2)
    rb_eval_string(module_rpg2);
  else if (rgssVer == 3)
    rb_eval_string(module_rpg3);
  else
    assert(!"unreachable");

  VALUE mod = rb_define_module("System");
  _rb_define_module_function(mod, "data_directory", mkxpDataDirectory);
  _rb_define_module_function(mod, "set_window_title", mkxpSetTitle);
  _rb_define_module_function(mod, "show_settings", mkxpSettingsMenu);
  _rb_define_module_function(mod, "puts", mkxpPuts);
  _rb_define_module_function(mod, "raw_key_states", mkxpRawKeyStates);
  _rb_define_module_function(mod, "mouse_in_window", mkxpMouseInWindow);
  _rb_define_module_function(mod, "platform", mkxpPlatform);
  _rb_define_module_function(mod, "user_language", mkxpUserLanguage);
  _rb_define_module_function(mod, "game_title", mkxpGameTitle);
  _rb_define_module_function(mod, "power_state", mkxpPowerState);
  _rb_define_module_function(mod, "nproc", mkxpCpuCount);
  _rb_define_module_function(mod, "memory", mkxpSystemMemory);

  /* Load global constants */
  rb_gv_set("MKXP", Qtrue);

  VALUE debug = rb_bool_new(shState->config().editor.debug);
  if (rgssVer == 1)
    rb_gv_set("DEBUG", debug);
  else if (rgssVer >= 2)
    rb_gv_set("TEST", debug);

  rb_gv_set("BTEST", rb_bool_new(shState->config().editor.battleTest));
}

static void showMsg(const std::string &msg) {
  shState->eThread().showMessageBox(msg.c_str());
}

static void printP(int argc, VALUE *argv, const char *convMethod,
                   const char *sep) {
  VALUE dispString = rb_str_buf_new(128);
  ID conv = rb_intern(convMethod);

  for (int i = 0; i < argc; ++i) {
    VALUE str = rb_funcall2(argv[i], conv, 0, NULL);
    rb_str_buf_append(dispString, str);

    if (i < argc)
      rb_str_buf_cat2(dispString, sep);
  }

  showMsg(RSTRING_PTR(dispString));
}

RB_METHOD(mriPrint) {
  RB_UNUSED_PARAM;

  printP(argc, argv, "to_s", "");

  return Qnil;
}

RB_METHOD(mriP) {
  RB_UNUSED_PARAM;

  printP(argc, argv, "inspect", "\n");

  return Qnil;
}

RB_METHOD(mkxpDataDirectory) {
  RB_UNUSED_PARAM;

  const std::string &path = shState->config().customDataPath;
  const char *s = path.empty() ? "." : path.c_str();

  char *s_nml = shState->fileSystem().normalize(s, 1, 1);
  VALUE ret = rb_str_new_cstr(s_nml);
  delete s_nml;

  return ret;
}

RB_METHOD(mkxpSetTitle) {
  RB_UNUSED_PARAM;

  VALUE s;
  rb_scan_args(argc, argv, "1", &s);
  SafeStringValue(s);

  shState->eThread().requestWindowRename(RSTRING_PTR(s));
  return s;
}

RB_METHOD(mkxpPuts) {
  RB_UNUSED_PARAM;

  const char *str;
  rb_get_args(argc, argv, "z", &str RB_ARG_END);

  Debug() << str;

  return Qnil;
}

RB_METHOD(mkxpRawKeyStates) {
  RB_UNUSED_PARAM;

  VALUE str = rb_str_new(0, sizeof(EventThread::keyStates));
  memcpy(RSTRING_PTR(str), EventThread::keyStates,
         sizeof(EventThread::keyStates));

  return str;
}

RB_METHOD(mkxpMouseInWindow) {
  RB_UNUSED_PARAM;

  return rb_bool_new(EventThread::mouseState.inWindow);
}

RB_METHOD(mkxpPlatform) {
  RB_UNUSED_PARAM;

  return rb_str_new_cstr(SDL_GetPlatform());
}

RB_METHOD(mkxpUserLanguage) {
  RB_UNUSED_PARAM;

  return rb_str_new_cstr(getUserLanguage());
}

RB_METHOD(mkxpGameTitle) {
  RB_UNUSED_PARAM;

  return rb_str_new_cstr(shState->config().game.title.c_str());
}

RB_METHOD(mkxpPowerState) {
  RB_UNUSED_PARAM;

  int secs, pct;
  SDL_PowerState ps = SDL_GetPowerInfo(&secs, &pct);

  VALUE hash = rb_hash_new();

  rb_hash_aset(hash, ID2SYM(rb_intern("seconds")),
               (secs > -1) ? INT2NUM(secs) : RUBY_Qnil);

  rb_hash_aset(hash, ID2SYM(rb_intern("percent")),
               (pct > -1) ? INT2NUM(pct) : RUBY_Qnil);

  rb_hash_aset(hash, ID2SYM(rb_intern("discharging")),
               rb_bool_new(ps == SDL_POWERSTATE_ON_BATTERY));

  return hash;
}

RB_METHOD(mkxpSettingsMenu) {
  RB_UNUSED_PARAM;

  shState->eThread().requestSettingsMenu();

  return Qnil;
}

RB_METHOD(mkxpCpuCount) {
  RB_UNUSED_PARAM;

  return INT2NUM(SDL_GetCPUCount());
}

RB_METHOD(mkxpSystemMemory) {
  RB_UNUSED_PARAM;

  return INT2NUM(SDL_GetSystemRAM());
}

static VALUE rgssMainCb(VALUE block) {
  rb_funcall2(block, rb_intern("call"), 0, 0);
  return Qnil;
}

static VALUE rgssMainRescue(VALUE arg, VALUE exc) {
  VALUE *excRet = (VALUE *)arg;

  *excRet = exc;

  return Qnil;
}

static void processReset() {
  shState->graphics().reset();
  shState->audio().reset();

  shState->rtData().rqReset.clear();
  shState->graphics().repaintWait(shState->rtData().rqResetFinish, false);
}

RB_METHOD(mriRgssMain) {
  RB_UNUSED_PARAM;

  while (true) {
    VALUE exc = Qnil;
#if RAPI_FULL < 270
    rb_rescue2((VALUE(*)(ANYARGS))rgssMainCb, rb_block_proc(),
               (VALUE(*)(ANYARGS))rgssMainRescue, (VALUE)&exc, rb_eException,
               (VALUE)0);
#else
    rb_rescue2(rgssMainCb, rb_block_proc(), rgssMainRescue, (VALUE)&exc,
               rb_eException, (VALUE)0);
#endif

    if (NIL_P(exc))
      break;

    if (rb_obj_class(exc) == getRbData()->exc[Reset])
      processReset();
    else
      rb_exc_raise(exc);
  }

  return Qnil;
}

RB_METHOD(mriRgssStop) {
  RB_UNUSED_PARAM;

  while (true)
    shState->graphics().update();

  return Qnil;
}

RB_METHOD(_kernelCaller) {
  RB_UNUSED_PARAM;

  VALUE trace =
      rb_funcall2(rb_mKernel, rb_intern("_mkxp_kernel_caller_alias"), 0, 0);

  if (!RB_TYPE_P(trace, RUBY_T_ARRAY))
    return trace;

  long len = RARRAY_LEN(trace);

  if (len < 2)
    return trace;

  /* Remove useless "ruby:1:in 'eval'" */
  rb_ary_pop(trace);

  /* Also remove trace of this helper function */
  rb_ary_shift(trace);

  len -= 2;

  if (len == 0)
    return trace;

  /* RMXP does this, not sure if specific or 1.8 related */
  VALUE args[] = {rb_str_new_cstr(":in `<main>'"), rb_str_new_cstr("")};
  rb_funcall2(rb_ary_entry(trace, len - 1), rb_intern("gsub!"), 2, args);

  return trace;
}

#if RAPI_FULL > 187
static VALUE newStringUTF8(const char *string, long length) {
  return rb_enc_str_new(string, length, rb_utf8_encoding());
}
#else
#define newStringUTF8 rb_str_new
#endif

struct evalArg {
  VALUE string;
  VALUE filename;
};

static VALUE evalHelper(evalArg *arg) {
  VALUE argv[] = {arg->string, Qnil, arg->filename};
  return rb_funcall2(Qnil, rb_intern("eval"), ARRAY_SIZE(argv), argv);
}

static VALUE evalString(VALUE string, VALUE filename, int *state) {
  evalArg arg = {string, filename};
  return rb_protect((VALUE(*)(VALUE))evalHelper, (VALUE)&arg, state);
}

static void runCustomScript(const std::string &filename) {
  std::string scriptData;

  if (!readFileSDL(filename.c_str(), scriptData)) {
    showMsg(std::string("Unable to open '") + filename + "'");
    return;
  }

  evalString(newStringUTF8(scriptData.c_str(), scriptData.size()),
             newStringUTF8(filename.c_str(), filename.size()), NULL);
}

VALUE kernelLoadDataInt(const char *filename, bool rubyExc, bool raw);

struct BacktraceData {
  /* Maps: Ruby visible filename, To: Actual script name */
  BoostHash<std::string, std::string> scriptNames;
};

bool evalScript(VALUE string, const char *filename)
{
  int state;
  evalString(string, rb_str_new_cstr(filename), &state);
  if (state) return false;
  return true;
}

#ifndef MARIN
#define SCRIPT_SECTION_FMT (rgssVer >= 3 ? "{%04ld}" : "Section%03ld")

static void runRMXPScripts(BacktraceData &btData) {
  const Config &conf = shState->rtData().config;
  const std::string &scriptPack = conf.game.scripts;
  const char *platform = SDL_GetPlatform();

  if (scriptPack.empty()) {
    showMsg("No game scripts specified (missing Game.ini?)");
    return;
  }

  if (!shState->fileSystem().exists(scriptPack.c_str())) {
    showMsg("Unable to open '" + scriptPack + "'");
    return;
  }

  VALUE scriptArray;

  /* We checked if Scripts.rxdata exists, but something might
   * still go wrong */
  try {
    scriptArray = kernelLoadDataInt(scriptPack.c_str(), false, false);
  } catch (const Exception &e) {
    showMsg(std::string("Failed to read script data: ") + e.msg);
    return;
  }

  if (!RB_TYPE_P(scriptArray, RUBY_T_ARRAY)) {
    showMsg("Failed to read script data");
    return;
  }

  rb_gv_set("$RGSS_SCRIPTS", scriptArray);

  long scriptCount = RARRAY_LEN(scriptArray);

  std::string decodeBuffer;
  decodeBuffer.resize(0x1000);

  for (long i = 0; i < scriptCount; ++i) {
    VALUE script = rb_ary_entry(scriptArray, i);

    if (!RB_TYPE_P(script, RUBY_T_ARRAY))
      continue;

    VALUE scriptName = rb_ary_entry(script, 1);
    VALUE scriptString = rb_ary_entry(script, 2);

    int result = Z_OK;
    unsigned long bufferLen;

    while (true) {
      unsigned char *bufferPtr = reinterpret_cast<unsigned char *>(
          const_cast<char *>(decodeBuffer.c_str()));
      const unsigned char *sourcePtr =
          reinterpret_cast<const unsigned char *>(RSTRING_PTR(scriptString));

      bufferLen = decodeBuffer.length();

      result = uncompress(bufferPtr, &bufferLen, sourcePtr,
                          RSTRING_LEN(scriptString));

      bufferPtr[bufferLen] = '\0';

      if (result != Z_BUF_ERROR)
        break;

      decodeBuffer.resize(decodeBuffer.size() * 2);
    }

    if (result != Z_OK) {
      static char buffer[256];
      snprintf(buffer, sizeof(buffer), "Error decoding script %ld: '%s'", i,
               RSTRING_PTR(scriptName));

      showMsg(buffer);

      break;
    }

    rb_ary_store(script, 3, rb_str_new_cstr(decodeBuffer.c_str()));
  }

  // Can be force-disabled similarly to framerate options
#ifndef NO_PRELOAD_SCRIPTS
  /* Execute preloaded scripts */
  for (std::vector<std::string>::const_iterator i = conf.preloadScripts.begin();
       i != conf.preloadScripts.end(); ++i)
    runCustomScript(*i);

  VALUE exc = rb_gv_get("$!");
  if (exc != Qnil)
    return;
#endif

  // Used to try and fix Essentials garbage later if it's detected
  int minimonsters = 0;

  while (true) {
#if RAPI_FULL < 200 && defined(NO_CONSOLE)
    VALUE iostr = rb_str_new2(NULL_IO);
    // Sysinit isn't a thing yet, so send io to /dev/null instead
    rb_funcall(rb_gv_get("$stderr"), rb_intern("reopen"), 1, iostr);
    rb_funcall(rb_gv_get("$stdout"), rb_intern("reopen"), 1, iostr);
#endif
    for (long i = 0; i < scriptCount; ++i) {
      VALUE script = rb_ary_entry(scriptArray, i);
      VALUE scriptDecoded = rb_ary_entry(script, 3);
      VALUE string =
          newStringUTF8(RSTRING_PTR(scriptDecoded), RSTRING_LEN(scriptDecoded));

      VALUE fname;
      const char *scriptName = RSTRING_PTR(rb_ary_entry(script, 1));
      char buf[512];
      int len;

      if (conf.useScriptNames)
        len = snprintf(buf, sizeof(buf), "%03ld:%s", i, scriptName);
      else
        len = snprintf(buf, sizeof(buf), SCRIPT_SECTION_FMT, i);

      fname = newStringUTF8(buf, len);
      btData.scriptNames.insert(buf, scriptName);

      // There is 0 reason for anything other than Essentials to have this class
      if (rb_const_defined(rb_cObject, rb_intern("PokemonMapMetadata")) && minimonsters == 0)
        minimonsters = 1;

      // Before checking to see if the next script should be skipped,
      // make sure to check whether it's the last one or not and run
      // any extra stuff before the end (primarily stupid Essentials stuff)
      // Will be placed within a build option later
      #define SCRIPT(name) rb_str_new((const char*)&___scripts_##name##_rb, ___scripts_##name##_rb_len), #name " (Internal)"
      if (minimonsters > 0 && i + 2 == scriptCount && !RTEST(rb_gv_get("Z_NOPOKEFIX"))){
        evalScript(SCRIPT(EssentialsTilemapHack));
        minimonsters = -1;
      }

      // if the script name starts with |s|, only execute
      // it if "s" is the same first letter as the platform
      // we're running on

      // |W| - Windows, |M| - Mac OS X, |L| - Linux

      // Adding a 'not' symbol means it WON'T run on that
      // platform (i.e. |!W| won't run on Windows)

      if (scriptName[0] == '|') {
        int len = strlen(scriptName);
        if (len > 2) {
          if (scriptName[1] == '!' && len > 3 &&
              scriptName[3] == scriptName[0]) {
            if (toupper(scriptName[2]) == platform[0])
              continue;
          }
          if (scriptName[2] == scriptName[0] &&
              toupper(scriptName[1]) != platform[0])
            continue;
        }
      }

      int state;

      evalString(string, fname, &state);
      if (state)
        break;
    }

    VALUE exc = rb_gv_get("$!");
    if (rb_obj_class(exc) != getRbData()->exc[Reset])
      break;

    processReset();
  }
}
#endif

static void showExc(VALUE exc, const BacktraceData &btData) {
  VALUE bt = rb_funcall2(exc, rb_intern("backtrace"), 0, NULL);
  VALUE msg = rb_funcall2(exc, rb_intern("message"), 0, NULL);
  VALUE bt0 = rb_ary_entry(bt, 0);
  VALUE name = rb_class_path(rb_obj_class(exc));

  VALUE ds = rb_sprintf("%" PRIsVALUE ": %" PRIsVALUE " (%" PRIsVALUE ")", bt0,
                        exc, name);
  /* omit "useless" last entry (from ruby:1:in `eval') */
  for (long i = 1, btlen = RARRAY_LEN(bt) - 1; i < btlen; ++i)
    rb_str_catf(ds, "\n\tfrom %" PRIsVALUE, rb_ary_entry(bt, i));
  Debug() << StringValueCStr(ds);

  char *s = RSTRING_PTR(bt0);

  char line[16];
  std::string file(512, '\0');

  char *p = s + strlen(s);
  char *e;

  while (p != s)
    if (*--p == ':')
      break;

  e = p;

  while (p != s)
    if (*--p == ':')
      break;

  /* s         p  e
   * SectionXXX:YY: in 'blabla' */

  *e = '\0';
  strncpy(line, *p ? p + 1 : p, sizeof(line));
  line[sizeof(line) - 1] = '\0';
  *e = ':';
  e = p;

  /* s         e
   * SectionXXX:YY: in 'blabla' */

  *e = '\0';
  strncpy(&file[0], s, file.size());
  *e = ':';

  /* Shrink to fit */
  file.resize(strlen(file.c_str()));
  file = btData.scriptNames.value(file, file);

  std::string ms(640, '\0');
  snprintf(&ms[0], ms.size(), "Script '%s' line %s: %s occured.\n\n%s",
           file.c_str(), line, RSTRING_PTR(name), RSTRING_PTR(msg));

  showMsg(ms);
}

static void mriBindingExecute() {
#if RAPI_FULL > 200
  /* Normally only a ruby executable would do a sysinit,
   * but not doing it will lead to crashes due to closed
   * stdio streams on some platforms (eg. Windows) */
  int argc = 0;
  char **argv = 0;
  ruby_sysinit(&argc, &argv);

  ruby_setup();
  rb_enc_set_default_external(rb_enc_from_encoding(rb_utf8_encoding()));
#else
  ruby_init();
  rb_eval_string("$KCODE='U'");
#endif

#if defined(USE_FAKEAPI) && !defined(__WIN32__)
  char *tmpdir = getenv("TMPDIR");
  if (tmpdir)
    setenv("TEMP", tmpdir, false);
#endif

  Config &conf = shState->rtData().config;

  VALUE lpaths = rb_gv_get(":");
  if (!conf.rubyLoadpaths.empty()) {
    /* Setup custom load paths */
    for (size_t i = 0; i < conf.rubyLoadpaths.size(); ++i) {
      std::string &path = conf.rubyLoadpaths[i];

      VALUE pathv = rb_str_new(path.c_str(), path.size());
      rb_ary_push(lpaths, pathv);
    }
  }
#ifdef MARIN
  else {
    rb_ary_push(lpaths, rb_str_new_cstr("ruby/extensions/2.5.0"));
    rb_ary_push(lpaths, rb_str_new_cstr("ruby/extensions/2.5.0/i386-mingw32"));
  }
#endif

  RbData rbData;
  shState->setBindingData(&rbData);
  BacktraceData btData;

  mriBindingInit();

  std::string &customScript = conf.customScript;
  if (!customScript.empty())
    runCustomScript(customScript);
  else
#ifdef MARIN
    runCustomScript("ruby/scripts/requires.rb");
#else
    runRMXPScripts(btData);
#endif

#if RAPI_FULL > 187
  VALUE exc = rb_errinfo();
#else
  VALUE exc = rb_gv_get("$!");
#endif
  if (!NIL_P(exc) && !rb_obj_is_kind_of(exc, rb_eSystemExit))
    showExc(exc, btData);

  ruby_cleanup(0);

  shState->rtData().rqTermAck.set();
}

static void mriBindingTerminate() { rb_raise(rb_eSystemExit, " "); }

static void mriBindingReset() { rb_raise(getRbData()->exc[Reset], " "); }
