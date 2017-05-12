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

#include "sharedstate.h"
#include "exception.h"
#include "util.h"

#include <stdarg.h>
#include <string.h>
#include <assert.h>

RbData *getRbData()
{
	return static_cast<RbData*>(shState->bindingData());
}

struct
{
	RbException id;
	const char *name;
} static customExc[] =
{
	{ MKXP,   "MKXPError"   },
	{ PHYSFS, "PHYSFSError" },
	{ SDL,    "SDLError"    }
};

RbData::RbData()
{
	for (size_t i = 0; i < ARRAY_SIZE(customExc); ++i)
		exc[customExc[i].id] = rb_define_class(customExc[i].name, rb_eException);

	exc[RGSS]  = rb_define_class("RGSSError", rb_eStandardError);
	exc[Reset] = rb_define_class(rgssVer >= 3 ? "RGSSReset" : "Reset", rb_eException);

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
    SDL,         /* SDLError    */
    MKXP         /* MKXPError   */
};

void raiseRbExc(const Exception &exc)
{
	RbData *data = getRbData();
	VALUE excClass = data->exc[excToRbExc[exc.type]];

	rb_raise(excClass, "%s", exc.msg.c_str());
}

void
raiseDisposedAccess(VALUE self)
{
#ifdef RUBY_LEGACY_VERSION
// FIXME: raiseDisposedAccess not implemented
#else
	const char *klassName = RTYPEDDATA_TYPE(self)->wrap_struct_name;
	char buf[32];

	strncpy(buf, klassName, sizeof(buf));
	buf[0] = tolower(buf[0]);

	rb_raise(getRbData()->exc[RGSS], "disposed %s", buf);
#endif
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

			if (!RB_TYPE_P(tmp, RUBY_T_STRING))
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

			if (!RB_TYPE_P(tmp, RUBY_T_STRING))
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

			if (!RB_TYPE_P(tmp, RUBY_T_STRING))
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

			rb_float_arg(fVal, f, argI);

			++argI;
			break;
		}

		case 'i' :
		{
			if (argI >= argc)
				break;

			int *i = va_arg(ap, int*);
			VALUE iVal = *arg++;

			rb_int_arg(iVal, i, argI);

			++argI;
			break;
		}

		case 'b' :
		{
			if (argI >= argc)
				break;

			bool *b = va_arg(ap, bool*);
			VALUE bVal = *arg++;

			rb_bool_arg(bVal, b, argI);

			++argI;
			break;
		}

		case 'n' :
		{
			if (argI >= argc)
				break;

			ID *sym = va_arg(ap, ID*);

			VALUE symVal = *arg++;

			if (!SYMBOL_P(symVal))
				rb_raise(rb_eTypeError, "Argument %d: Expected symbol", argI);

			*sym = SYM2ID(symVal);
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

#ifndef NDEBUG

	/* Pop remaining arg pointers off
	 * the stack to check for RB_ARG_END */
	format--;

	while ((c = *format++))
	{
		switch (c)
		{
		case 'o' :
		case 'S' :
			va_arg(ap, VALUE*);
			break;

		case 's' :
			va_arg(ap, const char**);
			va_arg(ap, int*);
			break;

		case 'z' :
			va_arg(ap, const char**);
			break;

		case 'f' :
			va_arg(ap, double*);
			break;

		case 'i' :
			va_arg(ap, int*);
			break;

		case 'b' :
			va_arg(ap, bool*);
			break;
		}
	}

	// FIXME print num of needed args vs provided
	if (!c && argc > argI)
		rb_raise(rb_eArgError, "wrong number of arguments");

	/* Verify correct termination */
	void *argEnd = va_arg(ap, void*);
	(void) argEnd;
	assert(argEnd == RB_ARG_END_VAL);

#endif

	va_end(ap);

	return argI;
}

#if RUBY_API_VERSION_MAJOR == 1
NORETURN(void rb_error_arity(int, int, int)) {
	assert(false);
}
#if RUBY_API_VERSION_MINOR == 8
// Functions providing Ruby 1.8 compatibililty
VALUE rb_sprintf(const char* fmt, ...) {
	va_list args;
	va_start(args, fmt);
	char buf[4096];
	int const result = vsnprintf(buf, sizeof(buf), fmt, args);
	va_end(args);

	return rb_str_new2(buf);
}

VALUE rb_data_typed_object_alloc(VALUE klass, void *datap, const rb_data_type_t * rbType) {
	return  rb_data_object_alloc(klass, 0, rbType->function.dmark, rbType->function.dfree);
}

void *rb_check_typeddata(VALUE v, const rb_data_type_t *) {
// FIXME: rb_check_typeddata not implemented
	return RTYPEDDATA_DATA(v);
}

VALUE rb_enc_str_new(const char* ch, long len, rb_encoding*) {
	return rb_str_new(ch, len);
}

rb_encoding *rb_utf8_encoding(void) {
	return NULL;
}

void rb_enc_set_default_external(VALUE encoding) {
	// no-op, assuming UTF-8
}

VALUE rb_enc_from_encoding(rb_encoding *enc) {
	return Qnil;
}

VALUE rb_str_catf(VALUE, const char*, ...) {
	return Qnil;
}

VALUE rb_errinfo(void) {
	return ruby_errinfo;
}

int rb_typeddata_is_kind_of(VALUE, const rb_data_type_t *) {
	return 1;
}

VALUE rb_file_open_str(VALUE filename, const char* mode) {
	return rb_file_open(RB_OBJ_STRING(filename), mode);
}

VALUE rb_enc_associate_index(VALUE, int) {
	return Qnil;
}

int rb_utf8_encindex(void) {
	return 0;
}

VALUE rb_hash_lookup2(VALUE, VALUE, VALUE) {
	return Qnil;
}
#endif // RUBY_API_VERSION_MINOR == 8
#endif // RUBY_API_VERSION_MAJOR == 1