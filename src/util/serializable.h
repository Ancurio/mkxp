/*
** serializable.h
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

#ifndef SERIALIZABLE_H
#define SERIALIZABLE_H

struct Serializable
{
	virtual int serialSize() const = 0;
	virtual void serialize(char *buffer) const = 0;
};

template<class C>
C *deserialize(const char *data)
{
	return C::deserialize(data);
}

#endif // SERIALIZABLE_H
