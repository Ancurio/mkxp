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
		VALUE str = rb_funcall(argv[i], conv, 0);
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

static void runCustomScript(const char *filename)
{
	std::string scriptData("#encoding:utf-8\n");

	if (!readFile(filename, scriptData))
	{
		showMsg(std::string("Unable to open '") + filename + "'");
		return;
	}

	rb_eval_string_protect(scriptData.c_str(), 0);
}

VALUE kernelLoadDataInt(const char *filename);

struct Script
{
	std::string name;
	std::string encData;
	uint32_t unknown;

	std::string decData;
};

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

	size_t scriptCount = RARRAY_LEN(scriptArray);

	std::string decodeBuffer;
	decodeBuffer.resize(0x1000);

	std::vector<Script> encScripts(scriptCount);

	for (size_t i = 0; i < scriptCount; ++i)
	{
		VALUE script = rb_ary_entry(scriptArray, i);

		if (rb_type(script) != RUBY_T_ARRAY)
		{
			continue;
		}

		VALUE scriptUnknown = rb_ary_entry(script, 0);
		VALUE scriptName    = rb_ary_entry(script, 1);
		VALUE scriptString  = rb_ary_entry(script, 2);

		Script &sc = encScripts[i];
		sc.name = RSTRING_PTR(scriptName);
		sc.encData = std::string(RSTRING_PTR(scriptString), RSTRING_LEN(scriptString));
		sc.unknown = FIX2UINT(scriptUnknown);
	}

	for (size_t i = 0; i < scriptCount; ++i)
	{
		Script &sc = encScripts[i];

		int result = Z_OK;
		ulong bufferLen;

		while (true)
		{
			unsigned char *bufferPtr =
			        reinterpret_cast<unsigned char*>(const_cast<char*>(decodeBuffer.c_str()));
			const unsigned char *sourcePtr =
			        reinterpret_cast<const unsigned char*>(sc.encData.c_str());

			bufferLen = decodeBuffer.length();

			result = uncompress(bufferPtr, &bufferLen,
			                    sourcePtr, sc.encData.length());

			bufferPtr[bufferLen] = '\0';

			if (result != Z_BUF_ERROR)
				break;

			decodeBuffer.resize(decodeBuffer.size()*2);
		}

		if (result != Z_OK)
		{
			static char buffer[256];
			/* FIXME: '%zu' apparently gcc only? */
			snprintf(buffer, sizeof(buffer), "Error decoding script %zu: '%s'",
			         i, sc.name.c_str());

			showMsg(buffer);
			break;
		}

		/* Store encoding header + the decoded script
		 * in 'sc.decData' */
		sc.decData = "#encoding:utf-8\n";
		size_t hdSize = sc.decData.size();
		sc.decData.resize(hdSize + bufferLen);
		memcpy(&sc.decData[hdSize], decodeBuffer.c_str(), bufferLen);

		ruby_script(sc.name.c_str());

		rb_gc_start();

		/* Execute code */
		rb_eval_string_protect(sc.decData.c_str(), 0);

		VALUE exc = rb_gv_get("$!");
		if (rb_type(exc) != RUBY_T_NIL)
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
		runCustomScript(customScript.c_str());
	else
		runRMXPScripts();

	VALUE exc = rb_gv_get("$!");
	if (rb_type(exc) != RUBY_T_NIL && !rb_eql(rb_obj_class(exc), rb_eSystemExit))
	{
		Debug() << "Had exception:" << rb_class2name(rb_obj_class(exc));
		VALUE bt = rb_funcall(exc, rb_intern("backtrace"), 0);
		rb_p(bt);
		VALUE msg = rb_funcall(exc, rb_intern("message"), 0);
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
