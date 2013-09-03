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
#include "globalstate.h"
#include "eventthread.h"
#include "filesystem.h"

#include "./ruby/include/ruby.h"

#include "zlib.h"

#include <QFile>
#include <QByteArray>

#include <QDebug>

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

static VALUE mriPrint(int, VALUE*, VALUE);
static VALUE mriP(int, VALUE*, VALUE);

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

	rb_define_global_const("MKXP", Qtrue);
}

static void
showMsg(const QByteArray &msg)
{
	gState->eThread().showMessageBox(msg.constData());
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

	rb_str_free(dispString);
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

static void runCustomScript(const char *filename)
{
	QFile scriptFile(filename);
	if (!scriptFile.open(QFile::ReadOnly))
	{
		showMsg(QByteArray("Unable to open '") + filename + "'");
		return;
	}

	QByteArray scriptData = scriptFile.readAll();
	scriptFile.close();

	rb_eval_string_protect(scriptData.constData(), 0);
}

VALUE kernelLoadDataInt(const char *filename);

struct Script
{
	QByteArray name;
	QByteArray encData;
	uint32_t unknown;

	QByteArray decData;
};

static void runRMXPScripts()
{
	const QByteArray &scriptPack = gState->rtData().config.game.scripts;

	if (scriptPack.isEmpty())
	{
		showMsg("No game scripts specified (missing Game.ini?)");
		return;
	}

	if (!gState->fileSystem().exists(scriptPack.constData()))
	{
		showMsg("Unable to open '" + scriptPack + "'");
		return;
	}

	VALUE scriptArray = kernelLoadDataInt(scriptPack.constData());

	if (rb_type(scriptArray) != RUBY_T_ARRAY)
	{
		showMsg("Failed to read script data");
		return;
	}

	int scriptCount = RARRAY_LEN(scriptArray);

	QByteArray decodeBuffer;
	decodeBuffer.resize(0x1000);

	QVector<Script> encScripts(scriptCount);

	for (int i = 0; i < scriptCount; ++i)
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
		sc.encData = QByteArray(RSTRING_PTR(scriptString), RSTRING_LEN(scriptString));
		sc.unknown = FIX2UINT(scriptUnknown);
	}

	for (int i = 0; i < scriptCount; ++i)
	{
		Script &sc = encScripts[i];

		int result = Z_OK;
		ulong bufferLen;

		while (true)
		{
			unsigned char *bufferPtr =
			        reinterpret_cast<unsigned char*>(const_cast<char*>(decodeBuffer.constData()));
			const unsigned char *sourcePtr =
			        reinterpret_cast<const unsigned char*>(sc.encData.constData());

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
			snprintf(buffer, sizeof(buffer), "Error decoding script %d: '%s'",
			         i, sc.name.constData());

			showMsg(buffer);
			break;
		}

		sc.decData = QByteArray(decodeBuffer.constData(), bufferLen);

		ruby_script(sc.name.constData());

		rb_gc_start();

		/* Execute code */
		rb_eval_string_protect(decodeBuffer.constData(), 0);

		VALUE exc = rb_gv_get("$!");
		if (rb_type(exc) != RUBY_T_NIL)
			break;
	}
}

static void mriBindingExecute()
{
	ruby_setup();

	RbData rbData;
	gState->setBindingData(&rbData);

	mriBindingInit();

	QByteArray &customScript = gState->rtData().config.customScript;
	if (!customScript.isEmpty())
		runCustomScript(customScript.constData());
	else
		runRMXPScripts();

	VALUE exc = rb_gv_get("$!");
	if (rb_type(exc) != RUBY_T_NIL && !rb_eql(rb_obj_class(exc), rb_eSystemExit))
	{
		qDebug() << "Had exception:" << rb_class2name(rb_obj_class(exc));
		VALUE bt = rb_funcall(exc, rb_intern("backtrace"), 0);
		rb_p(bt);
		VALUE msg = rb_funcall(exc, rb_intern("message"), 0);
		if (RSTRING_LEN(msg) < 256)
			showMsg(RSTRING_PTR(msg));
		else
			qDebug() << (RSTRING_PTR(msg));
	}

	ruby_cleanup(0);

	gState->rtData().rqTermAck = true;
}

static void mriBindingTerminate()
{
	rb_raise(rb_eSystemExit, " ");
}
