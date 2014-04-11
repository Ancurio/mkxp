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

#include "binding.h"
#include "binding-util.h"
#include "sharedstate.h"
#include "eventthread.h"
#include "filesystem.h"
#include "util.h"
#include "debugwriter.h"

#include <ruby.h>
#include <ruby/encoding.h>

#include <string>

#include <zlib.h>

#include <SDL_filesystem.h>

extern const char module_rpg[];

static void mriBindingExecute();
static void mriBindingTerminate();

ScriptBinding scriptBindingImpl =
{
	mriBindingExecute,
	mriBindingTerminate
};

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

void inputBindingInit();
void audioBindingInit();
void graphicsBindingInit();

void fileIntBindingInit();

RB_METHOD(mriPrint);
RB_METHOD(mriP);
RB_METHOD(mriDataDirectory);
RB_METHOD(mkxpPuts);

static void mriBindingInit()
{
	tableBindingInit();
	etcBindingInit();
	fontBindingInit();
	bitmapBindingInit();
	spriteBindingInit();
	viewportBindingInit();
	planeBindingInit();
	windowBindingInit();
	tilemapBindingInit();

	inputBindingInit();
	audioBindingInit();
	graphicsBindingInit();

	fileIntBindingInit();

	_rb_define_module_function(rb_mKernel, "print", mriPrint);
	_rb_define_module_function(rb_mKernel, "p",     mriP);

	rb_eval_string(module_rpg);

	VALUE mod = rb_define_module("System");
	_rb_define_module_function(mod, "data_directory", mriDataDirectory);
	_rb_define_module_function(mod, "puts", mkxpPuts);

	rb_define_global_const("MKXP", Qtrue);
}

static void
showMsg(const std::string &msg)
{
	shState->eThread().showMessageBox(msg.c_str());
}

RB_METHOD(mkxpPuts)
{
	RB_UNUSED_PARAM;

	const char *str;
	rb_get_args(argc, argv, "z", &str RB_ARG_END);

	Debug() << str;

	return Qnil;
}

static void printP(int argc, VALUE *argv,
                   const char *convMethod, const char *sep)
{
	VALUE dispString = rb_str_buf_new(128);
	ID conv = rb_intern(convMethod);

	for (int i = 0; i < argc; ++i)
	{
		VALUE str = rb_funcall2(argv[i], conv, 0, NULL);
		rb_str_buf_append(dispString, str);

		if (i < argc)
			rb_str_buf_cat2(dispString, sep);
	}

	showMsg(RSTRING_PTR(dispString));
}

RB_METHOD(mriPrint)
{
	RB_UNUSED_PARAM;

	printP(argc, argv, "to_s", "");

	return Qnil;
}

RB_METHOD(mriP)
{
	RB_UNUSED_PARAM;

	printP(argc, argv, "inspect", "\n");

	return Qnil;
}

RB_METHOD(mriDataDirectory)
{
	RB_UNUSED_PARAM;

	const char *org, *app;

	rb_get_args(argc, argv, "zz", &org, &app RB_ARG_END);

	char *path = SDL_GetPrefPath(org, app);

	VALUE pathStr = rb_str_new_cstr(path);

	SDL_free(path);

	return pathStr;
}

static VALUE newStringUTF8(const char *string, long length)
{
	return rb_enc_str_new(string, length, rb_utf8_encoding());
}

struct evalArg
{
	VALUE string;
	VALUE filename;
};

static VALUE evalHelper(evalArg *arg)
{
	VALUE argv[] = { arg->string, Qnil, arg->filename };
	return rb_funcall2(Qnil, rb_intern("eval"), ARRAY_SIZE(argv), argv);
}

static VALUE evalString(VALUE string, VALUE filename, int *state)
{
	evalArg arg = { string, filename };
	return rb_protect((VALUE (*)(VALUE))evalHelper, (VALUE)&arg, state);
}

static void runCustomScript(const std::string &filename)
{
	std::string scriptData;

	if (!readFile(filename.c_str(), scriptData))
	{
		showMsg(std::string("Unable to open '") + filename + "'");
		return;
	}

	evalString(newStringUTF8(scriptData.c_str(), scriptData.size()),
	           newStringUTF8(filename.c_str(), filename.size()), NULL);
}

VALUE kernelLoadDataInt(const char *filename);

static void runRMXPScripts()
{
	const std::string &scriptPack = shState->rtData().config.game.scripts;

	if (scriptPack.empty())
	{
		showMsg("No game scripts specified (missing Game.ini?)");
		return;
	}

	if (!shState->fileSystem().exists(scriptPack.c_str()))
	{
		showMsg("Unable to open '" + scriptPack + "'");
		return;
	}

	VALUE scriptArray = kernelLoadDataInt(scriptPack.c_str());

	if (rb_type(scriptArray) != RUBY_T_ARRAY)
	{
		showMsg("Failed to read script data");
		return;
	}

	rb_gv_set("$RGSS_SCRIPTS", scriptArray);

	long scriptCount = RARRAY_LEN(scriptArray);

	std::string decodeBuffer;
	decodeBuffer.resize(0x1000);

	for (long i = 0; i < scriptCount; ++i)
	{
		VALUE script = rb_ary_entry(scriptArray, i);

		if (rb_type(script) != RUBY_T_ARRAY)
			continue;

		VALUE scriptName   = rb_ary_entry(script, 1);
		VALUE scriptString = rb_ary_entry(script, 2);

		int result = Z_OK;
		unsigned long bufferLen;

		while (true)
		{
			unsigned char *bufferPtr =
			        reinterpret_cast<unsigned char*>(const_cast<char*>(decodeBuffer.c_str()));
			const unsigned char *sourcePtr =
			        reinterpret_cast<const unsigned char*>(RSTRING_PTR(scriptString));

			bufferLen = decodeBuffer.length();

			result = uncompress(bufferPtr, &bufferLen,
			                    sourcePtr, RSTRING_LEN(scriptString));

			bufferPtr[bufferLen] = '\0';

			if (result != Z_BUF_ERROR)
				break;

			decodeBuffer.resize(decodeBuffer.size()*2);
		}

		if (result != Z_OK)
		{
			static char buffer[256];
			snprintf(buffer, sizeof(buffer), "Error decoding script %ld: '%s'",
			         i, RSTRING_PTR(scriptName));

			showMsg(buffer);

			break;
		}

		rb_ary_store(script, 3, rb_str_new_cstr(decodeBuffer.c_str()));
	}

	for (long i = 0; i < scriptCount; ++i)
	{
		VALUE script = rb_ary_entry(scriptArray, i);
		VALUE scriptDecoded = rb_ary_entry(script, 3);
		VALUE string = newStringUTF8(RSTRING_PTR(scriptDecoded),
		                             RSTRING_LEN(scriptDecoded));

		char fname[32];
		int len = snprintf(fname, sizeof(fname), "Section%03ld", i);

		int state;
		evalString(string, newStringUTF8(fname, len), &state);
		if (state)
			break;
	}
}

static void mriBindingExecute()
{
	ruby_setup();
	rb_enc_set_default_external(rb_enc_from_encoding(rb_utf8_encoding()));

	RbData rbData;
	shState->setBindingData(&rbData);

	mriBindingInit();

	std::string &customScript = shState->rtData().config.customScript;
	if (!customScript.empty())
		runCustomScript(customScript);
	else
		runRMXPScripts();

	VALUE exc = rb_errinfo();
	if (!NIL_P(exc) && !rb_obj_is_kind_of(exc, rb_eSystemExit))
	{
		Debug() << "Had exception:" << rb_class2name(rb_obj_class(exc));
		VALUE bt = rb_funcall2(exc, rb_intern("backtrace"), 0, NULL);
		rb_p(bt);
		VALUE msg = rb_funcall2(exc, rb_intern("message"), 0, NULL);
		if (RSTRING_LEN(msg) < 256)
			showMsg(RSTRING_PTR(msg));
		else
			Debug() << (RSTRING_PTR(msg));
	}

	ruby_cleanup(0);

	shState->rtData().rqTermAck = true;
}

static void mriBindingTerminate()
{
	rb_raise(rb_eSystemExit, " ");
}
