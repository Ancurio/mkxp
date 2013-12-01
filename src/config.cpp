/*
** config.cpp
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

#include "config.h"

#include <QSettings>
#include <QFileInfo>
#include <QString>
#include <QStringList>
#include <QFile>
#include <QRegExp>

Config::Config()
    : debugMode(false),
      winResizable(false),
      fullscreen(false),
      fixedAspectRatio(true),
      smoothScaling(false),
      vsync(false),
      defScreenW(640),
      defScreenH(480),
      fixedFramerate(0),
      solidFonts(false),
      fastSoundPitch(true),
      fastMusicPitch(false),
      gameFolder("."),
      allowSymlinks(false)
{}

void Config::read()
{
	QSettings confFile("mkxp.conf", QSettings::IniFormat);

#define READ_VAL(key, Type) key = confFile.value(#key, key).to##Type()

	READ_VAL(debugMode, Bool);
	READ_VAL(winResizable, Bool);
	READ_VAL(fullscreen, Bool);
	READ_VAL(fixedAspectRatio, Bool);
	READ_VAL(smoothScaling, Bool);
	READ_VAL(vsync, Bool);
	READ_VAL(defScreenW, Int);
	READ_VAL(defScreenH, Int);
	READ_VAL(fixedFramerate, Int);
	READ_VAL(solidFonts,  Bool);
	READ_VAL(fastSoundPitch, Bool);
	READ_VAL(fastMusicPitch, Bool);
	READ_VAL(gameFolder, ByteArray);
	READ_VAL(allowSymlinks, Bool);
	READ_VAL(customScript, ByteArray);

	QStringList _rtps = confFile.value("RTPs").toStringList();
	Q_FOREACH(const QString &s, _rtps)
		rtps << s.toUtf8();

	confFile.beginGroup("Binding");

	QStringList bindingKeys = confFile.childKeys();
	Q_FOREACH (const QString &key, bindingKeys)
	{
		QVariant value = confFile.value(key);

		/* Convert QString to QByteArray */
		if (value.type() == QVariant::String)
			value = value.toString().toUtf8();

		bindingConf.insert(key.toLatin1(), value);
	}

	confFile.endGroup();
}

void Config::readGameINI()
{
	if (!customScript.isEmpty())
	{
		game.title = basename(customScript.constData());
		return;
	}

	QSettings gameINI(gameFolder + "/Game.ini", QSettings::IniFormat);
	QFileInfo finfo(gameFolder.constData());
	game.title = gameINI.value("Game/Title", finfo.baseName()).toByteArray();

	/* Gotta read the "Scripts" entry manually because Qt can't handle '\' */
	QFile gameINIFile(gameFolder + "/Game.ini");
	if (gameINIFile.open(QFile::ReadOnly))
	{
		QString gameINIContents = gameINIFile.readAll();
		QRegExp scriptsRE(".*Scripts=(.*)\r\nTitle=.*");
		if (scriptsRE.exactMatch(gameINIContents))
		{
			game.scripts = scriptsRE.cap(1).toUtf8();
			game.scripts.replace('\\', '/');
		}
	}
}
