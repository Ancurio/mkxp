/*
** config.h
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

#ifndef CONFIG_H
#define CONFIG_H

#include <QByteArray>
#include <QVector>
#include <QHash>
#include <QVariant>

struct Config
{
	bool debugMode;

	bool winResizable;
	bool fullscreen;
	bool fixedAspectRatio;
	bool smoothScaling;
	bool vsync;

	int defScreenW;
	int defScreenH;

	int fixedFramerate;
	bool frameSkip;

	bool solidFonts;

	QByteArray gameFolder;
	bool allowSymlinks;

	QByteArray customScript;
	QVector<QByteArray> rtps;

	/* Any values in the [Binding]
	 * group are collected here */
	QHash<QByteArray, QVariant> bindingConf;

	/* Game INI contents */
	struct {
		QByteArray scripts;
		QByteArray title;
	} game;

	Config();

	void read();
	void readGameINI();
};

#endif // CONFIG_H
