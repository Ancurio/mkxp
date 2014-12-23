/*
** debugwriter.h
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

#ifndef DEBUGWRITER_H
#define DEBUGWRITER_H

#include <iostream>
#include <sstream>
#include <vector>

#ifdef __ANDROID__
#include <android/log.h>
#endif


/* A cheap replacement for qDebug() */

class Debug
{
public:
	Debug()
	{
		buf << std::boolalpha;
	}

	template<typename T>
	Debug &operator<<(const T &t)
	{
		buf << t;
		buf << " ";

		return *this;
	}

	template<typename T>
	Debug &operator<<(const std::vector<T> &v)
	{
		for (size_t i = 0; i < v.size(); ++i)
			buf << v[i] << " ";

		return *this;
	}

	~Debug()
	{
#ifdef __ANDROID__
		__android_log_write(ANDROID_LOG_DEBUG, "mkxp", buf.str().c_str());
#else
		std::cerr << buf.str() << std::endl;
#endif
	}

private:
	std::stringstream buf;
};

#endif // DEBUGWRITER_H
