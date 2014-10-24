/*
** binding-util.cpp
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

#include "binding-util.h"
#include "util.h"
#include "exception.h"

#include <string.h>

#define SYMD(symbol) { CS##symbol, #symbol }

struct
{
	CommonSymbol ind;
	const char *str;
} static const symData[] =
{
	SYMD(font),
	SYMD(viewport),
	SYMD(bitmap),
	SYMD(color),
	SYMD(out_color),
	SYMD(tone),
	SYMD(rect),
	SYMD(src_rect),
	SYMD(tilemap),
	SYMD(tileset),
	SYMD(autotiles),
	SYMD(map_data),
	SYMD(flash_data),
	SYMD(priorities),
	SYMD(windowskin),
	SYMD(contents),
	SYMD(cursor_rect),
	SYMD(path),
	SYMD(array),
	SYMD(default_color),
	SYMD(default_out_color),
	SYMD(children),
	SYMD(_mkxp_dispose_alias)
};

static elementsN(symData);

struct MrbExcData
{
	MrbException ind;
	const char *str;
};

static const MrbExcData excData[] =
{
	{ Shutdown, "SystemExit"  },
	{ PHYSFS,   "PHYSFSError" },
	{ SDL,      "SDLError"    },
	{ MKXP,     "MKXPError"   },
	{ IO,       "IOError"     }
};

static elementsN(excData);

#define ENO(id) { Errno##id, #id }

static const MrbExcData enoExcData[] =
{
	ENO(E2BIG),
	ENO(EACCES),
	ENO(EAGAIN),
	ENO(EBADF),
	ENO(ECHILD),
	ENO(EDEADLOCK),
	ENO(EDOM),
	ENO(EEXIST),
	ENO(EINVAL),
	ENO(EMFILE),
	ENO(ENOENT),
	ENO(ENOEXEC),
	ENO(ENOMEM),
	ENO(ENOSPC),
	ENO(ERANGE),
	ENO(EXDEV)
};

static elementsN(enoExcData);

MrbData::MrbData(mrb_state *mrb)
{
	int arena = mrb_gc_arena_save(mrb);

	for (size_t i = 0; i < excDataN; ++i)
		exc[excData[i].ind] = mrb_define_class(mrb, excData[i].str, mrb->eException_class);

	exc[RGSS] = mrb_define_class(mrb, "RGSSError", mrb->eStandardError_class);

	RClass *errnoMod = mrb_define_module(mrb, "Errno");

	for (size_t i = 0; i < enoExcDataN; ++i)
		exc[enoExcData[i].ind] =
		        mrb_define_class_under(mrb, errnoMod, enoExcData[i].str, mrb->eStandardError_class);

	exc[TypeError] = mrb_class_get(mrb, "TypeError");
	exc[ArgumentError] = mrb_class_get(mrb, "ArgumentError");

	for (size_t i = 0; i < symDataN; ++i)
		symbols[symData[i].ind] = mrb_intern_cstr(mrb, symData[i].str);

	mrb_gc_arena_restore(mrb, arena);
}

/* Indexed with Exception::Type */
static const MrbException excToMrbExc[] =
{
	RGSS,        /* RGSSError   */
	ErrnoENOENT, /* NoFileError */
	IO,

	TypeError,
	ArgumentError,

	PHYSFS,      /* PHYSFSError */
	SDL,         /* SDLError    */
	MKXP         /* MKXPError   */
};

void raiseMrbExc(mrb_state *mrb, const Exception &exc)
{
	MrbData *data = getMrbData(mrb);
	RClass *excClass = data->exc[excToMrbExc[exc.type]];

	mrb_raise(mrb, excClass, exc.msg.c_str());
}

void
raiseDisposedAccess(mrb_state *mrb, mrb_value self)
{
	const char *klassName = DATA_TYPE(self)->struct_name;
	char buf[32];

	strncpy(buf, klassName, sizeof(buf));
	buf[0] = tolower(buf[0]);

	mrb_raisef(mrb, getMrbData(mrb)->exc[RGSS],
	           "disposed %S", mrb_str_new_cstr(mrb, buf));
}

MRB_METHOD_PUB(inspectObject)
{
	static char buffer[64];
	snprintf(buffer, sizeof(buffer), "#<%s:%p>", DATA_TYPE(self)->struct_name, DATA_PTR(self));

	return mrb_str_new_cstr(mrb, buffer);
}
