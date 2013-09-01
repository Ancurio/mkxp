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

#include "globalstate.h"
#include "exception.h"
#include "util.h"

#include <stdarg.h>

#include <QDebug>

void initType(rb_data_type_struct &type,
              const char *name,
              void (*freeInst)(void *))
{
	type.wrap_struct_name = name;
	type.function.dmark = 0;
	type.function.dsize = 0;
	type.function.dfree = freeInst;
	type.function.reserved[0] =
	type.function.reserved[1] = 0;
	type.parent = 0;
}

RbData *getRbData()
{
	return static_cast<RbData*>(gState->bindingData());
}

//enum RbException
//{
//	RGSS = 0,
//	PHYSFS,
//	SDL,

//	ErrnoENOENT,

//	IOError,

//	TypeError,
//	ArgumentError,

//	RbExceptionsMax
//};

struct
{
	RbException id;
	const char *name;
} static customExc[] =
{
	{ RGSS,   "RGSSError"   },
	{ PHYSFS, "PHYSFSError" },
	{ SDL,    "SDLError"    }
};

RbData::RbData()
{
	for (size_t i = 0; i < ARRAY_SIZE(customExc); ++i)
		exc[customExc[i].id] = rb_define_class(customExc[i].name, rb_eException);

	exc[ErrnoENOENT] = rb_const_get(rb_const_get(rb_cObject, rb_intern("Errno")), rb_intern("ENOENT"));
	exc[IOError] = rb_eIOError;
	exc[TypeError] = rb_eTypeError;
	exc[ArgumentError] = rb_eArgError;
}

RbData::~RbData()
{

}

/* Indexed with Exception::Type */
static const RbException excToRbExc[] =
{
    RGSS,        /* RGSSError   */
    ErrnoENOENT, /* NoFileError */
    IOError,

    TypeError,
    ArgumentError,

    PHYSFS,      /* PHYSFSError */
    SDL          /* SDLError    */
};

void raiseRbExc(const Exception &exc)
{
	RbData *data = getRbData();
	VALUE excClass = data->exc[excToRbExc[exc.type]];

	static char buffer[512];
	exc.snprintf(buffer, sizeof(buffer));
	rb_raise(excClass, buffer);
}

int
rb_get_args(int argc, VALUE *argv, const char *format, ...)
{
	char c;
	VALUE *arg = argv;
	va_list ap;
	bool opt = false;
	int argI = 0;

	va_start(ap, format);

	while ((c = *format++))
	{
		switch (c)
		{
	    case '|' :
			break;
	    default:
		// FIXME print num of needed args vs provided
			if (argc <= argI && !opt)
				rb_raise(rb_eArgError, "wrong number of arguments");
			break;
	    }

		if (argI >= argc)
			break;

		switch (c)
		{
		case 'o' :
		{
			if (argI >= argc)
				break;

			VALUE *obj = va_arg(ap, VALUE*);

			*obj = *arg++;
			++argI;
			break;
		}

		case 'S' :
		{
			if (argI >= argc)
				break;

			VALUE *str = va_arg(ap, VALUE*);
			VALUE tmp = *arg;

			if (!rb_type(tmp) == RUBY_T_STRING)
				rb_raise(rb_eTypeError, "Argument %d: Expected string", argI);

			*str = tmp;
			++argI;
			break;
		}

		case 's' :
		{
			if (argI >= argc)
				break;

			const char **s = va_arg(ap, const char**);
			int *len = va_arg(ap, int*);

			VALUE tmp = *arg;

			if (!rb_type(tmp) == RUBY_T_STRING)
				rb_raise(rb_eTypeError, "Argument %d: Expected string", argI);

			*s = RSTRING_PTR(tmp);
			*len = RSTRING_LEN(tmp);
			++argI;
			break;
		}

		case 'z' :
		{
			if (argI >= argc)
				break;

			const char **s = va_arg(ap, const char**);

			VALUE tmp = *arg++;

			if (!rb_type(tmp) == RUBY_T_STRING)
				rb_raise(rb_eTypeError, "Argument %d: Expected string", argI);

			*s = RSTRING_PTR(tmp);
			++argI;
			break;
		}

		case 'f' :
		{
			if (argI >= argc)
				break;

			double *f = va_arg(ap, double*);
			VALUE fVal = *arg++;

			switch (rb_type(fVal))
			{
			case RUBY_T_FLOAT :
				*f = rb_float_value(fVal);
				break;

			case RUBY_T_FIXNUM :
				*f = rb_fix2int(fVal);
				break;

			default:
				rb_raise(rb_eTypeError, "Argument %d: Expected float", argI);
			}

			++argI;
			break;
		}

		case 'i' :
		{
			if (argI >= argc)
				break;

			int *i = va_arg(ap, int*);
			VALUE iVal = *arg++;

			switch (rb_type(iVal))
			{
			case RUBY_T_FLOAT :
				// FIXME check int range?
				*i = rb_num2long(iVal);
				break;

			case RUBY_T_FIXNUM :
				*i = rb_fix2int(iVal);
				break;

			default:
				rb_raise(rb_eTypeError, "Argument %d: Expected fixnum", argI);
			}

			++argI;
			break;
		}

		case 'b' :
		{
			if (argI >= argc)
				break;

			bool *b = va_arg(ap, bool*);
			VALUE bVal = *arg++;

			switch (rb_type(bVal))
			{
			case RUBY_T_TRUE :
				*b = true;
				break;

			case RUBY_T_FALSE :
			case RUBY_T_NIL :
				*b = false;
				break;

			default:
				rb_raise(rb_eTypeError, "Argument %d: Expected bool", argI);
			}

			++argI;
			break;
		}

		case '|' :
			opt = true;
			break;

		default:
			rb_raise(rb_eFatal, "invalid argument specifier %c", c);
		}
	}

	// FIXME print num of needed args vs provided
	if (!c && argc > argI)
		rb_raise(rb_eArgError, "wrong number of arguments");

	va_end(ap);

	return argI;
}
