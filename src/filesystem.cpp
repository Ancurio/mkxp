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

struct SDLRWIoContext
{
	SDL_RWops *ops;
	std::string filename;

	SDLRWIoContext(const char *filename)
	    : ops(SDL_RWFromFile(filename, "r")),
	      filename(filename)
	{
		if (!ops)
			throw Exception(Exception::SDLError,
			                "Failed to open file: %s", SDL_GetError());
	}

	~SDLRWIoContext()
	{
		SDL_RWclose(ops);
	}
};

static PHYSFS_Io *createSDLRWIo(const char *filename);

static SDL_RWops *getSDLRWops(PHYSFS_Io *io)
{
	return static_cast<SDLRWIoContext*>(io->opaque)->ops;
}

static PHYSFS_sint64 SDLRWIoRead(struct PHYSFS_Io *io, void *buf, PHYSFS_uint64 len)
{
	return SDL_RWread(getSDLRWops(io), buf, 1, len);
}

static int SDLRWIoSeek(struct PHYSFS_Io *io, PHYSFS_uint64 offset)
{
	return (SDL_RWseek(getSDLRWops(io), offset, RW_SEEK_SET) != -1);
}

static PHYSFS_sint64 SDLRWIoTell(struct PHYSFS_Io *io)
{
	return SDL_RWseek(getSDLRWops(io), 0, RW_SEEK_CUR);
}

static PHYSFS_sint64 SDLRWIoLength(struct PHYSFS_Io *io)
{
	return SDL_RWsize(getSDLRWops(io));
}

static struct PHYSFS_Io *SDLRWIoDuplicate(struct PHYSFS_Io *io)
{
	SDLRWIoContext *ctx = static_cast<SDLRWIoContext*>(io->opaque);
	int64_t offset = io->tell(io);
	PHYSFS_Io *dup = createSDLRWIo(ctx->filename.c_str());

	if (dup)
		SDLRWIoSeek(dup, offset);

	return dup;
}

static void SDLRWIoDestroy(struct PHYSFS_Io *io)
{
	delete static_cast<SDLRWIoContext*>(io->opaque);
	delete io;
}

static PHYSFS_Io SDLRWIoTemplate =
{
	0, 0, /* version, opaque */
	SDLRWIoRead,
	0, /* write */
	SDLRWIoSeek,
	SDLRWIoTell,
	SDLRWIoLength,
	SDLRWIoDuplicate,
	0, /* flush */
	SDLRWIoDestroy
};

static PHYSFS_Io *createSDLRWIo(const char *filename)
{
	SDLRWIoContext *ctx;

	try
	{
		ctx = new SDLRWIoContext(filename);
	}
	catch (const Exception &e)
	{
		Debug() << "Failed mounting" << filename;
		return 0;
	}

	PHYSFS_Io *io = new PHYSFS_Io;
	*io = SDLRWIoTemplate;
	io->opaque = ctx;

	return io;
}

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

/* Copies the first srcN characters from src into dst,
 * or the full string if srcN == -1. Never writes more
 * than dstMax, and guarantees dst to be null terminated.
 * Returns copied bytes (minus terminating null) */
static size_t
strcpySafe(char *dst, const char *src,
           size_t dstMax, int srcN)
{
	if (srcN < 0)
		srcN = strlen(src);

	size_t cpyMax = std::min<size_t>(dstMax-1, srcN);

	memcpy(dst, src, cpyMax);
	dst[cpyMax] = '\0';

	return cpyMax;
}

const Uint32 SDL_RWOPS_PHYSFS = SDL_RWOPS_UNKNOWN+10;

struct FileSystemPrivate
{
	/* Maps: lower case filepath without extension,
	 * To:   mixed case full filepath
	 * This is for compatibility with games that take Windows'
	 * case insensitivity for granted */
	BoostHash<std::string, std::string> pathCache;
	bool havePathCache;

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

	struct CompleteFilenameData
	{
		bool found;
		/* Contains the incomplete filename we're looking for;
		 * when found, we write the complete filename into this
		 * same buffer */
		char *outBuf;
		/* Length of incomplete file name */
		size_t filenameLen;
		/* Maximum we can write into outBuf */
		size_t outBufN;
	};

	static void completeFilenameRegCB(void *data, const char *,
	                                  const char *fname)
	{
		CompleteFilenameData &d = *static_cast<CompleteFilenameData*>(data);

		if (d.found)
			return;

		if (strncmp(d.outBuf, fname, d.filenameLen) != 0)
			return;

		/* If fname matches up to a following '.' (meaning the rest is part
		 * of the extension), or up to a following '\0' (full match), we've
		 * found our file */
		switch (fname[d.filenameLen])
		{
		case '.' :
			/* Overwrite the incomplete file name we looked for with
			 * the full version containing any extensions */
			strcpySafe(d.outBuf, fname, d.outBufN, -1);
		case '\0' :
			d.found = true;
		}
	}

	bool completeFilenameReg(const char *filepath,
	                         char *outBuffer,
	                         size_t outN)
	{
		strcpySafe(outBuffer, filepath, outN, -1);

		size_t len = strlen(outBuffer);
		char *delim;

		/* Find the deliminator separating directory and file name */
		for (delim = outBuffer + len; delim > outBuffer; --delim)
			if (*delim == '/')
				break;

		bool root = (delim == outBuffer);
		CompleteFilenameData d;

		if (!root)
		{
			/* If we have such a deliminator, we set it to '\0' so we
			 * can pass the first half to PhysFS as the directory name,
			 * and compare all filenames against the second half */
			d.outBuf = delim+1;
			d.filenameLen = len - (delim - outBuffer + 1);

			*delim = '\0';
		}
		else
		{
			/* Otherwise the file is in the root directory */
			d.outBuf = outBuffer;
			d.filenameLen = len - (delim - outBuffer);
		}

		d.found = false;
		d.outBufN = outN - (d.outBuf - outBuffer);

		PHYSFS_enumerateFilesCallback(root ? "" : outBuffer, completeFilenameRegCB, &d);

		if (!d.found)
			return false;

		/* Now we put the deliminator back in to form the completed
		 * file path (if required) */
		if (delim != outBuffer)
			*delim = '/';

		return true;
	}

	bool completeFilenamePC(const char *filepath,
	                        char *outBuffer,
	                        size_t outN)
	{
		std::string lowCase(filepath);

		for (size_t i = 0; i < lowCase.size(); ++i)
			lowCase[i] = tolower(lowCase[i]);

		if (!pathCache.contains(lowCase))
			return false;

		const std::string &fullPath = pathCache[lowCase];
		strcpySafe(outBuffer, fullPath.c_str(), outN, fullPath.size());

		return true;
	}

	bool completeFilename(const char *filepath,
	                      char *outBuffer,
	                      size_t outN)
	{
		if (havePathCache)
			return completeFilenamePC(filepath, outBuffer, outN);
		else
			return completeFilenameReg(filepath, outBuffer, outN);
	}

	PHYSFS_File *openReadHandle(const char *filename,
	                            char *extBuf,
	                            size_t extBufN)
	{
		char found[512];

		if (!completeFilename(filename, found, sizeof(found)))
			throw Exception(Exception::NoFileError, "%s", filename);

		PHYSFS_File *handle = PHYSFS_openRead(found);

		if (!handle)
			throw Exception(Exception::PHYSFSError, "PhysFS: %s", PHYSFS_getLastError());

		if (!extBuf)
			return handle;

		for (char *q = found+strlen(found); q > found; --q)
		{
			if (*q == '/')
				break;

			if (*q != '.')
				continue;

			strcpySafe(extBuf, q+1, extBufN, -1);
			break;
		}

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
	/* Try the normal mount first */
	if (!PHYSFS_mount(path, 0, 1))
	{
		/* If it didn't work, try mounting via a wrapped
		 * SDL_RWops */
		PHYSFS_Io *io = createSDLRWIo(path);

		if (io)
			PHYSFS_mountIo(io, path, 0, 1);
	}
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

	for (char *q = bufNfc; *q; ++q)
		*q = tolower(*q);

	p->pathCache.insert(std::string(ptr), mixedCase);

	for (char *q = ptr+strlen(ptr); q > ptr; --q)
	{
		if (*q == '/')
			break;

		if (*q != '.')
			continue;

		*q = '\0';
		p->pathCache.insert(std::string(ptr), mixedCase);
	}

	PHYSFS_enumerateFilesCallback(mixedCase.c_str(), cacheEnumCB, d);
}

void FileSystem::createPathCache()
{
#ifdef __APPLE__
	CacheEnumCBData data(p);
	PHYSFS_enumerateFilesCallback("", cacheEnumCB2, &data);
#else
	PHYSFS_enumerateFilesCallback("", cacheEnumCB, p);
#endif

	p->havePathCache = true;
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

	char lowExt[8];
	size_t i;

	for (i = 0; i < sizeof(lowExt)-1 && ext[i]; ++i)
		lowExt[i] = tolower(ext[i]);
	lowExt[i] = '\0';

	if (strcmp(lowExt, "ttf") && strcmp(lowExt, "otf"))
		return;

	char filename[512];
	snprintf(filename, sizeof(filename), "Fonts/%s", fname);
	filename[sizeof(filename)-1] = '\0';

	PHYSFS_File *handle = PHYSFS_openRead(filename);

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
                          bool freeOnClose,
                          char *extBuf,
                          size_t extBufN)
{
 	PHYSFS_File *handle = p->openReadHandle(filename, extBuf, extBufN);

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

bool FileSystem::exists(const char *filename)
{
	char found[512];

	return p->completeFilename(filename, found, sizeof(found));
}
