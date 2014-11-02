/*
** table.cpp
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

#include "table.h"

#include <string.h>
#include <algorithm>

#include "serial-util.h"
#include "exception.h"
#include "util.h"

/* Init normally */
Table::Table(int x, int y /*= 1*/, int z /*= 1*/)
	: m_x(x), m_y(y), m_z(z),
      data(x*y*z)
{}

Table::Table(const Table &other)
    : m_x(other.m_x), m_y(other.m_y), m_z(other.m_z),
      data(other.data)
{}

int16_t Table::get(int x, int y, int z) const
{
	return data[m_x*m_y*z + m_x*y + x];
}

void Table::set(int16_t value, int x, int y, int z)
{
	if (x < 0 || x >= m_x
	||  y < 0 || y >= m_y
	||  z < 0 || z >= m_z)
	{
		return;
	}

	data[m_x*m_y*z + m_x*y + x] = value;

	modified();
}

void Table::resize(int x, int y, int z)
{
	if (x == m_x && y == m_y && z == m_z)
		return;

	std::vector<int16_t> newData(x*y*z);

	for (int k = 0; k < std::min(z, m_z); ++k)
		for (int j = 0; j < std::min(y, m_y); ++j)
			for (int i = 0; i < std::min(x, m_x); ++i)
				newData[x*y*k + x*j + i] = at(i, j, k);

	data.swap(newData);

	m_x = x;
	m_y = y;
	m_z = z;

	return;
}

void Table::resize(int x, int y)
{
	resize(x, y, m_z);
}

void Table::resize(int x)
{
	resize(x, m_y, m_z);
}

/* Serializable */
int Table::serialSize() const
{
	/* header + data */
	return 20 + (m_x * m_y * m_z) * 2;
}

void Table::serialize(char *buffer) const
{
	char *buff_p = buffer;

	/* Table dimensions: we don't care
	 * about them but RMXP needs them */
	int dim = 1;
	int size = m_x * m_y * m_z;

	if (m_y > 1)
		dim = 2;

	if (m_z > 1)
		dim = 3;

	write_int32(&buff_p, dim);
	write_int32(&buff_p, m_x);
	write_int32(&buff_p, m_y);
	write_int32(&buff_p, m_z);
	write_int32(&buff_p, size);

	memcpy(buff_p, dataPtr(data), sizeof(int16_t)*size);
}


Table *Table::deserialize(const char *data, int len)
{
	if (len < 20)
		throw Exception(Exception::RGSSError, "Marshal: Table: bad file format");

	uint idx = 0;

	read_int32(data, idx);
	int x = read_int32(data, idx);
	int y = read_int32(data, idx);
	int z = read_int32(data, idx);
	int size = read_int32(data, idx);

	if (size != x*y*z)
		throw Exception(Exception::RGSSError, "Marshal: Table: bad file format");

	if (len != 20 + x*y*z*2)
		throw Exception(Exception::RGSSError, "Marshal: Table: bad file format");

	Table *t = new Table(x, y, z);
	memcpy(dataPtr(t->data), &data[idx], sizeof(int16_t)*size);

	return t;
}
