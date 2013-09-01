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

typedef unsigned uint;

static inline int16_t
read_int16(const char *data, uint &i)
{
	int16_t result = (data[i] & 0x000000FF) | ((data[i+1] << 8) & 0x0000FF00);
	i += 2;

	return result;
}

static inline int32_t
read_int32(const char *data, uint &i)
{
	int32_t result = ((data[i+0] << 0x00) & 0x000000FF)
	               | ((data[i+1] << 0x08) & 0x0000FF00)
	               | ((data[i+2] << 0x10) & 0x00FF0000)
	               | ((data[i+3] << 0x18) & 0xFF000000);
	i += 4;

	return result;
}

static inline void
write_int16(char **data, int16_t value)
{
	*(*data)++ = (value >> 0) & 0xFF;
	*(*data)++ = (value >> 8) & 0xFF;
}

static inline void
write_int32(char **data, int32_t value)
{
	*(*data)++ = (value >> 0x00) & 0xFF;
	*(*data)++ = (value >> 0x08) & 0xFF;
	*(*data)++ = (value >> 0x10) & 0xFF;
	*(*data)++ = (value >> 0x18) & 0xFF;
}

union doubleInt
{
	double d;
	int64_t i;
};

static inline void
write_double(char **data, double value)
{
	doubleInt di;
	di.d = value;

	*(*data)++ = (di.i >> 0x00) & 0xFF;
	*(*data)++ = (di.i >> 0x08) & 0xFF;
	*(*data)++ = (di.i >> 0x10) & 0xFF;
	*(*data)++ = (di.i >> 0x18) & 0xFF;
	*(*data)++ = (di.i >> 0x20) & 0xFF;
	*(*data)++ = (di.i >> 0x28) & 0xFF;
	*(*data)++ = (di.i >> 0x30) & 0xFF;
	*(*data)++ = (di.i >> 0x38) & 0xFF;
}

static inline double
read_double(const char *data, uint &i)
{
	doubleInt di;
	di.i = (((int64_t)data[i+0] << 0x00) & 0x00000000000000FF)
	     | (((int64_t)data[i+1] << 0x08) & 0x000000000000FF00)
	     | (((int64_t)data[i+2] << 0x10) & 0x0000000000FF0000)
	     | (((int64_t)data[i+3] << 0x18) & 0x00000000FF000000)
	     | (((int64_t)data[i+4] << 0x20) & 0x000000FF00000000)
	     | (((int64_t)data[i+5] << 0x28) & 0x0000FF0000000000)
	     | (((int64_t)data[i+6] << 0x30) & 0x00FF000000000000)
	     | (((int64_t)data[i+7] << 0x38) & 0xFF00000000000000);
	i += 8;

	return di.d;
}

#endif // SERIALUTIL_H
