/*
** exception.h
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

#ifndef EXCEPTION_H
#define EXCEPTION_H

#include <string>
#include <stdio.h>
#include <stdarg.h>

struct Exception
{
	enum Type
	{
		RGSSError,
		NoFileError,
		IOError,

		/* Already defined by ruby */
		TypeError,
		ArgumentError,

		/* New types introduced in mkxp */
		PHYSFSError,
		SDLError,
		MKXPError
	};

	Type type;
	std::string msg;

	Exception(Type type, const char *format, ...)
	    : type(type)
	{
		va_list ap;
		va_start(ap, format);

		msg.resize(512);
		vsnprintf(&msg[0], msg.size(), format, ap);

		va_end(ap);
	}
};

#endif // EXCEPTION_H
