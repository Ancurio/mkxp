/*
** serial-util.h
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

#ifndef SERIALUTIL_H
#define SERIALUTIL_H

#include <stdint.h>
#include <string.h>

#include <SDL_endian.h>

typedef unsigned uint;

#if SDL_BYTEORDER != SDL_LIL_ENDIAN
#error "Non little endian systems not supported"
#endif

static inline int16_t
read_int16(const char *data, uint &i)
{
	int16_t result;

	memcpy(&result, &data[i], 2);
	i += 2;

	return result;
}

static inline int32_t
read_int32(const char *data, uint &i)
{
	int32_t result;

	memcpy(&result, &data[i], 4);
	i += 4;

	return result;
}

static inline double
read_double(const char *data, uint &i)
{
	double result;

	memcpy(&result, &data[i], 8);
	i += 8;

	return result;
}

static inline void
write_int16(char **data, int16_t value)
{
	memcpy(*data, &value, 2);

	data += 2;
}

static inline void
write_int32(char **data, int32_t value)
{
	memcpy(*data, &value, 4);

	data += 4;
}

static inline void
write_double(char **data, double value)
{
	memcpy(*data, &value, 8);

	data += 8;
}

#endif // SERIALUTIL_H
