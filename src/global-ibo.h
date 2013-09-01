/*
** global-ibo.h
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

#ifndef GLOBALIBO_H
#define GLOBALIBO_H

#include <gl-util.h>
#include <QVector>

struct GlobalIBO
{
	IBO::ID ibo;
	QVector<quint32> buffer;

	GlobalIBO()
	{
		ibo = IBO::gen();
	}

	~GlobalIBO()
	{
		IBO::del(ibo);
	}

	void ensureSize(int quadCount)
	{
		if (buffer.size() >= quadCount*6)
			return;

		int startInd = buffer.size() / 6;
		buffer.reserve(quadCount*6);

		for (int i = startInd; i < quadCount; ++i)
		{
			static const int indTemp[] = { 0, 1, 2, 2, 3, 0 };

			for (int j = 0; j < 6; ++j)
				buffer.append(i * 4 + indTemp[j]);
		}

		IBO::bind(ibo);
		IBO::uploadData(buffer.count() * sizeof(int),
		                buffer.constData());
		IBO::unbind();
	}
};

#endif // GLOBALIBO_H
