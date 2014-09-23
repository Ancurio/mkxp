/*
** filesystem.cpp
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

#include "filesystem.h"

#include "rgssad.h"
#include "font.h"
#include "util.h"
#include "exception.h"
#include "sharedstate.h"
#include "boost-hash.h"
#include "debugwriter.h"

#include <physfs.h>

#include <SDL_sound.h>

#include <stdio.h>
#include <string.h>
#include <algorithm>
#include <vector>

#ifdef __APPLE__
#include <iconv.h>
#endif

static inline PHYSFS_File *sdlPHYS(SDL_RWops *ops)
{
	return static_cast<PHYSFS_File*>(ops->hidden.unknown.data1);
}

static Sint64 SDL_RWopsSize(SDL_RWops *ops)
{
	PHYSFS_File *f = sdlPHYS(ops);

	if (!f)
		return -1;

	return PHYSFS_fileLength(f);
}

static Sint64 SDL_RWopsSeek(SDL_RWops *ops, int64_t offset, int whence)
{
	PHYSFS_File *f = sdlPHYS(ops);

	if (!f)
		return -1;

	int64_t base;

	switch (whence)
	{
	default:
	case RW_SEEK_SET :
		base = 0;
		break;
	case RW_SEEK_CUR :
		base = PHYSFS_tell(f);
		break;
	case RW_SEEK_END :
		base = PHYSFS_fileLength(f);
		break;
	}

	int result = PHYSFS_seek(f, base + offset);

	return (result != 0) ? PHYSFS_tell(f) : -1;
}

static size_t SDL_RWopsRead(SDL_RWops *ops, void *buffer, size_t size, size_t maxnum)
{
	PHYSFS_File *f = sdlPHYS(ops);

	if (!f)
		return 0;

	PHYSFS_sint64 result = PHYSFS_readBytes(f, buffer, size*maxnum);

	return (result != -1) ? (result / size) : 0;
}

static size_t SDL_RWopsWrite(SDL_RWops *ops, const void *buffer, size_t size, size_t num)
{
	PHYSFS_File *f = sdlPHYS(ops);

	if (!f)
		return 0;

	PHYSFS_sint64 result = PHYSFS_writeBytes(f, buffer, size*num);

	return (result != -1) ? (result / size) : 0;
}

static int SDL_RWopsClose(SDL_RWops *ops)
{
	PHYSFS_File *f = sdlPHYS(ops);

	if (!f)
		return -1;

	int result = PHYSFS_close(f);
	ops->hidden.unknown.data1 = 0;

	return (result != 0) ? 0 : -1;
}

static int SDL_RWopsCloseFree(SDL_RWops *ops)
{
	int result = SDL_RWopsClose(ops);

	SDL_FreeRW(ops);

	return result;
}

const Uint32 SDL_RWOPS_PHYSFS = SDL_RWOPS_UNKNOWN+10;

struct FileSystemPrivate
{
	/* Maps: lower case filename, To: actual (mixed case) filename.
	 * This is for compatibility with games that take Windows'
	 * case insensitivity for granted */
	BoostHash<std::string, std::string> pathCache;
	bool havePathCache;

	std::vector<std::string> extensions[FileSystem::Undefined+1];

	/* Attempt to locate an extension string in a filename.
	 * Either a pointer into the input string pointing at the
	 * extension, or null is returned */
	const char *findExt(const char *filename)
	{
		size_t len;

		for (len = strlen(filename); len > 0; --len)
		{
			if (filename[len] == '/')
				return 0;

			if (filename[len] == '.')
				return &filename[len+1];
		}

		return 0;
	}

	/* Complete filename via regular physfs lookup */
	bool completeFilenameReg(const char *filename,
	                         FileSystem::FileType type,
	                         char *outBuffer,
	                         size_t outN,
	                         const char **foundExt)
	{
		/* Try supplementing extensions to find an existing path */
		const std::vector<std::string> &extList = extensions[type];

		for (size_t i = 0; i < extList.size(); ++i)
		{
			const char *ext = extList[i].c_str();

			snprintf(outBuffer, outN, "%s.%s", filename, ext);

			if (PHYSFS_exists(outBuffer))
			{
				if (foundExt)
					*foundExt = ext;

				return true;
			}
		}

		/* Doing the check without supplemented extension
		 * fits the usage pattern of RMXP games */
		if (PHYSFS_exists(filename))
		{
			strncpy(outBuffer, filename, outN);

			if (foundExt)
				*foundExt = findExt(filename);

			return true;
		}

		return false;
	}

	/* Complete filename via path cache */
	bool completeFilenamePC(const char *filename,
	                        FileSystem::FileType type,
	                        char *outBuffer,
	                        size_t outN,
	                        const char **foundExt)
	{
		size_t i;
		char lowCase[512];

		for (i = 0; i < sizeof(lowCase)-1 && filename[i]; ++i)
			lowCase[i] = tolower(filename[i]);

		lowCase[i] = '\0';

		std::string key;

		const std::vector<std::string> &extList = extensions[type];

		for (size_t i = 0; i < extList.size(); ++i)
		{
			const char *ext = extList[i].c_str();

			snprintf(outBuffer, outN, "%s.%s", lowCase, ext);
			key = outBuffer;

			if (pathCache.contains(key))
			{
				strncpy(outBuffer, pathCache[key].c_str(), outN);

				if (foundExt)
					*foundExt = ext;

				return true;
			}
		}

		key = lowCase;

		if (pathCache.contains(key))
		{
			strncpy(outBuffer, pathCache[key].c_str(), outN);

			if (foundExt)
				*foundExt = findExt(filename);

			return true;
		}

		return false;
	}

	/* Try to complete 'filename' with file extensions
	 * based on 'type'. If no combination could be found,
	 * returns false, and 'foundExt' is untouched */
	bool completeFileName(const char *filename,
	                      FileSystem::FileType type,
	                      char *outBuffer,
	                      size_t outN,
	                      const char **foundExt)
	{
		if (havePathCache)
			return completeFilenamePC(filename, type, outBuffer, outN, foundExt);
		else
			return completeFilenameReg(filename, type, outBuffer, outN, foundExt);
	}

	PHYSFS_File *openReadHandle(const char *filename,
	                            FileSystem::FileType type,
	                            const char **foundExt)
	{
		char found[512];

		if (!completeFileName(filename, type, found, sizeof(found), foundExt))
			throw Exception(Exception::NoFileError, "%s", filename);

		PHYSFS_File *handle = PHYSFS_openRead(found);

		if (!handle)
			throw Exception(Exception::PHYSFSError, "PhysFS: %s", PHYSFS_getLastError());

		return handle;
	}

	void initReadOps(PHYSFS_File *handle,
	                 SDL_RWops &ops,
	                 bool freeOnClose)
	{
		ops.size  = SDL_RWopsSize;
		ops.seek  = SDL_RWopsSeek;
		ops.read  = SDL_RWopsRead;
		ops.write = SDL_RWopsWrite;

		if (freeOnClose)
			ops.close = SDL_RWopsCloseFree;
		else
			ops.close = SDL_RWopsClose;

		ops.type = SDL_RWOPS_PHYSFS;
		ops.hidden.unknown.data1 = handle;
	}
};

FileSystem::FileSystem(const char *argv0,
                       bool allowSymlinks)
{
	p = new FileSystemPrivate;

	p->havePathCache = false;

	/* Image extensions */
	p->extensions[Image].push_back("jpg");
	p->extensions[Image].push_back("png");

	/* Audio extensions */
	const Sound_DecoderInfo **di;
	for (di = Sound_AvailableDecoders(); *di; ++di)
	{
		const char **ext;
		for (ext = (*di)->extensions; *ext; ++ext)
		{
			/* All reported extensions are uppercase,
			 * so we need to hammer them down first */
			char buf[16];
			for (size_t i = 0; i < sizeof(buf); ++i)
			{
				buf[i] = tolower((*ext)[i]);

				if (!buf[i])
					break;
			}

			p->extensions[Audio].push_back(buf);
		}
	}

	if (rgssVer >= 2 && !contains(p->extensions[Audio], std::string("ogg")))
		p->extensions[Audio].push_back("ogg");

	p->extensions[Audio].push_back("mid");
	p->extensions[Audio].push_back("midi");

	/* Font extensions */
	p->extensions[Font].push_back("ttf");
	p->extensions[Font].push_back("otf");

	PHYSFS_init(argv0);

	PHYSFS_registerArchiver(&RGSS1_Archiver);
	PHYSFS_registerArchiver(&RGSS2_Archiver);
	PHYSFS_registerArchiver(&RGSS3_Archiver);

	if (allowSymlinks)
		PHYSFS_permitSymbolicLinks(1);
}

FileSystem::~FileSystem()
{
	delete p;

	if (PHYSFS_deinit() == 0)
		Debug() << "PhyFS failed to deinit.";
}

void FileSystem::addPath(const char *path)
{
	PHYSFS_mount(path, 0, 1);
}

#ifdef __APPLE__
struct CacheEnumCBData
{
	FileSystemPrivate *p;
	iconv_t nfd2nfc;

	CacheEnumCBData(FileSystemPrivate *fsp)
	{
		p = fsp;
		nfd2nfc = iconv_open("utf-8", "utf-8-mac");
	}

	~CacheEnumCBData()
	{
		iconv_close(nfd2nfc);
	}

	void nfcFromNfd(char *dst, const char *src, size_t dstSize)
	{
		size_t srcSize = strlen(src);
		/* Reserve room for null terminator */
		--dstSize;
		/* iconv takes a char** instead of a const char**, even though
		 * the string data isn't written to. */
		iconv(nfd2nfc,
			  const_cast<char**>(&src), &srcSize,
			  &dst, &dstSize);
		/* Null-terminate */
		*dst = 0;
	}
};
#endif

static void cacheEnumCB(void *d, const char *origdir,
                        const char *fname)
{
#ifdef __APPLE__
	CacheEnumCBData *data = static_cast<CacheEnumCBData*>(d);
	FileSystemPrivate *p = data->p;
#else
	FileSystemPrivate *p = static_cast<FileSystemPrivate*>(d);
#endif

	char buf[512];

	if (*origdir == '\0')
		strncpy(buf, fname, sizeof(buf));
	else
		snprintf(buf, sizeof(buf), "%s/%s", origdir, fname);

#ifdef __APPLE__
	char bufNfc[sizeof(buf)];
	data->nfcFromNfd(bufNfc, buf, sizeof(bufNfc));
#else
	char *const bufNfc = buf;
#endif

	char *ptr = bufNfc;

	/* Trim leading slash */
	if (*ptr == '/')
		++ptr;

	std::string mixedCase(ptr);

	for (char *p = bufNfc; *p; ++p)
		*p = tolower(*p);

	std::string lowerCase(ptr);

	p->pathCache.insert(lowerCase, mixedCase);

	PHYSFS_enumerateFilesCallback(mixedCase.c_str(), cacheEnumCB, d);
}

void FileSystem::createPathCache()
{
#ifdef __APPLE__
	CacheEnumCBData data(p);
	PHYSFS_enumerateFilesCallback("", cacheEnumCB, &data);
#else
	PHYSFS_enumerateFilesCallback("", cacheEnumCB, p);
#endif

	p->havePathCache = true;
}

static void strToLower(std::string &str)
{
	for (size_t i = 0; i < str.size(); ++i)
		str[i] = tolower(str[i]);
}

struct FontSetsCBData
{
	FileSystemPrivate *p;
	SharedFontState *sfs;
};

static void fontSetEnumCB(void *data, const char *,
                          const char *fname)
{
	FontSetsCBData *d = static_cast<FontSetsCBData*>(data);
	FileSystemPrivate *p = d->p;

	/* Only consider filenames with font extensions */
	const char *ext = p->findExt(fname);

	if (!ext)
		return;

	std::string lower(ext);
	strToLower(lower);

	if (!contains(p->extensions[FileSystem::Font], lower))
		return;

	std::string filename("Fonts/");
	filename += fname;

	PHYSFS_File *handle = PHYSFS_openRead(filename.c_str());

	if (!handle)
		return;

	SDL_RWops ops;
	p->initReadOps(handle, ops, false);

	d->sfs->initFontSetCB(ops, filename);

	SDL_RWclose(&ops);
}

void FileSystem::initFontSets(SharedFontState &sfs)
{
	FontSetsCBData d = { p, &sfs };

	PHYSFS_enumerateFilesCallback("Fonts", fontSetEnumCB, &d);
}

void FileSystem::openRead(SDL_RWops &ops,
                          const char *filename,
                          FileType type,
                          bool freeOnClose,
                          const char **foundExt)
{
	PHYSFS_File *handle = p->openReadHandle(filename, type, foundExt);

	p->initReadOps(handle, ops, freeOnClose);
}

void FileSystem::openReadRaw(SDL_RWops &ops,
                             const char *filename,
                             bool freeOnClose)
{
	PHYSFS_File *handle = PHYSFS_openRead(filename);
	assert(handle);

	p->initReadOps(handle, ops, freeOnClose);
}

bool FileSystem::exists(const char *filename, FileType type)
{
	char found[512];

	return p->completeFileName(filename, type, found, sizeof(found), 0);
}
