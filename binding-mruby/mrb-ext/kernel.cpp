/*
** kernel.cpp
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

#include <mruby.h>
#include <mruby/string.h>
#include <mruby/compile.h>

#include <stdlib.h>
#include <sys/time.h>

#include <SDL_messagebox.h>

#include "../binding-util.h"
#include "marshal.h"
#include "sharedstate.h"
#include "eventthread.h"
#include "exception.h"
#include "filesystem.h"
#include "binding.h"

void mrbBindingTerminate();

MRB_FUNCTION(kernelEval)
{
	const char *exp;
	mrb_int expLen;
	mrb_get_args(mrb, "s", &exp, &expLen);

	return mrb_load_nstring(mrb, exp, expLen);
}

static void printP(mrb_state *mrb,
                   const char *conv, const char *sep)
{
	mrb_int argc;
	mrb_value *argv;
	mrb_get_args(mrb, "*", &argv, &argc);

	mrb_value buffer = mrb_str_buf_new(mrb, 128);
	mrb_value arg;

	for (int i = 0; i < argc; ++i)
	{
		arg = argv[i];
		arg = mrb_funcall(mrb, arg, conv, 0);

		mrb_str_buf_append(mrb, buffer, arg);

		if (i < argc)
			mrb_str_buf_cat(mrb, buffer, sep, strlen(sep));
	}

	shState->eThread().showMessageBox(RSTRING_PTR(buffer), SDL_MESSAGEBOX_INFORMATION);
}

MRB_FUNCTION(kernelP)
{
	printP(mrb, "inspect", "\n");

	return mrb_nil_value();
}

MRB_FUNCTION(kernelPrint)
{
	printP(mrb, "to_s", "");

	return mrb_nil_value();
}

// FIXME store this in kernel module
static int currentSeed = 1;
bool srandCalled = false;

static void
srandCurrentTime(int *currentSeed)
{
	timeval tv;
	gettimeofday(&tv, 0);

	// FIXME could check how MRI does this
	*currentSeed = tv.tv_sec * tv.tv_usec;

	srand(*currentSeed);
}

MRB_FUNCTION(kernelRand)
{
	if (!srandCalled)
	{
		srandCurrentTime(&currentSeed);
		srandCalled = true;
	}

	mrb_value arg;

	mrb_get_args(mrb, "o", &arg);

	if (mrb_fixnum_p(arg) && mrb_fixnum(arg) != 0)
	{
		return mrb_fixnum_value(rand() % mrb_fixnum(arg));
	}
	else if ((mrb_fixnum_p(arg) && mrb_fixnum(arg) == 0) || mrb_nil_p(arg))
	{
		return mrb_float_value(mrb, (float) rand() / RAND_MAX);
	}
	else if (mrb_float_p(arg))
	{
		return mrb_float_value(mrb, (float) rand() / RAND_MAX);
	}
	else
	{
		mrb_raisef(mrb, E_TYPE_ERROR, "Expected Fixnum or Float, got %S",
		           mrb_str_new_cstr(mrb, mrb_class_name(mrb, mrb_class(mrb, arg))));
		return mrb_nil_value();
	}
}

MRB_FUNCTION(kernelSrand)
{
	srandCalled = true;

	if (mrb->c->ci->argc == 1)
	{
		mrb_int seed;
		mrb_get_args(mrb, "i", &seed);
		srand(seed);

		int oldSeed = currentSeed;
		currentSeed = seed;

		return mrb_fixnum_value(oldSeed);
	}
	else
	{
		int oldSeed = currentSeed;
		srandCurrentTime(&currentSeed);

		return mrb_fixnum_value(oldSeed);
	}
}

MRB_FUNCTION(kernelExit)
{
	MRB_FUN_UNUSED_PARAM;

	scriptBinding->terminate();

	return mrb_nil_value();
}

MRB_FUNCTION(kernelLoadData)
{
	const char *filename;
	mrb_get_args(mrb, "z", &filename);

	SDL_RWops ops;
	GUARD_EXC( shState->fileSystem().openRead(ops, filename); )

	mrb_value obj;
	try { obj = marshalLoadInt(mrb, &ops); }
	catch (const Exception &e)
	{
		SDL_RWclose(&ops);
		raiseMrbExc(mrb, e);
	}

	SDL_RWclose(&ops);

	return obj;
}

MRB_FUNCTION(kernelSaveData)
{
	mrb_value obj;
	const char *filename;

	mrb_get_args(mrb, "oz", &obj, &filename);

	SDL_RWops *ops = SDL_RWFromFile(filename, "w");
	if (!ops)
		mrb_raise(mrb, getMrbData(mrb)->exc[SDL], SDL_GetError());

	try { marshalDumpInt(mrb, ops, obj); }
	catch (const Exception &e)
	{
		SDL_RWclose(ops);
		raiseMrbExc(mrb, e);
	}

	SDL_RWclose(ops);

	return mrb_nil_value();
}

MRB_FUNCTION(kernelInteger)
{
	mrb_value obj;
	mrb_get_args(mrb, "o", &obj);

	return mrb_to_int(mrb, obj);
}

void kernelBindingInit(mrb_state *mrb)
{
	RClass *module = mrb->kernel_module;

	mrb_define_module_function(mrb, module, "eval", kernelEval, MRB_ARGS_REQ(1));
	mrb_define_module_function(mrb, module, "p", kernelP, MRB_ARGS_REQ(1) | MRB_ARGS_REST());
	mrb_define_module_function(mrb, module, "print", kernelPrint, MRB_ARGS_REQ(1) | MRB_ARGS_REST());
	mrb_define_module_function(mrb, module, "rand", kernelRand, MRB_ARGS_REQ(1));
	mrb_define_module_function(mrb, module, "srand", kernelSrand, MRB_ARGS_OPT(1));
	mrb_define_module_function(mrb, module, "exit", kernelExit, MRB_ARGS_NONE());
	mrb_define_module_function(mrb, module, "load_data", kernelLoadData, MRB_ARGS_REQ(1));
	mrb_define_module_function(mrb, module, "save_data", kernelSaveData, MRB_ARGS_REQ(2));
	mrb_define_module_function(mrb, module, "exit", kernelExit, MRB_ARGS_NONE());
	mrb_define_module_function(mrb, module, "Integer", kernelInteger, MRB_ARGS_REQ(1));
}
