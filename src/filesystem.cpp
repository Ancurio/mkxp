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

#include "font.h"
#include "util.h"
#include "defines.hpp"
#include "exception.h"
#include "boost-hash.h"
#include "debugwriter.h"

#include <physfs.h>

#include <SDL_sound.h>

#include <stdio.h>
#include <string.h>
#include <algorithm>
#include <vector>

struct RGSS_entryData
{
	int64_t offset;
	uint64_t size;
	uint32_t startMagic;
};

struct RGSS_entryHandle
{
	RGSS_entryData data;
	uint32_t currentMagic;
	uint64_t currentOffset;
	PHYSFS_Io *io;

	RGSS_entryHandle(const RGSS_entryData &data, PHYSFS_Io *archIo)
	    : data(data),
	      currentMagic(data.startMagic),
	      currentOffset(0)
	{
		io = archIo->duplicate(archIo);
	}

	~RGSS_entryHandle()
	{
		io->destroy(io);
	}
};

struct RGSS_archiveData
{
	PHYSFS_Io *archiveIo;

	/* Maps: file path
	 * to:   entry data */
	BoostHash<std::string, RGSS_entryData> entryHash;

	/* Maps: directory path,
	 * to:   list of contained entries */
	BoostHash<std::string, BoostSet<std::string> > dirHash;
};

static bool
readUint32(PHYSFS_Io *io, uint32_t &result)
{
	char buff[4];
	PHYSFS_sint64 count = io->read(io, buff, 4);

	result = ((buff[0] << 0x00) & 0x000000FF) |
	         ((buff[1] << 0x08) & 0x0000FF00) |
	         ((buff[2] << 0x10) & 0x00FF0000) |
	         ((buff[3] << 0x18) & 0xFF000000) ;

	return (count == 4);
}

#define RGSS_HEADER_1 0x53534752
#define RGSS_HEADER_2 0x01004441

#define RGSS_MAGIC 0xDEADCAFE

#define PHYSFS_ALLOC(type) \
	static_cast<type*>(PHYSFS_getAllocator()->Malloc(sizeof(type)))

static inline uint32_t
advanceMagic(uint32_t &magic)
{
	uint32_t old = magic;

	magic = magic * 7 + 3;

	return old;
}

static PHYSFS_sint64
RGSS_ioRead(PHYSFS_Io *self, void *buffer, PHYSFS_uint64 len)
{
	RGSS_entryHandle *entry = static_cast<RGSS_entryHandle*>(self->opaque);

	PHYSFS_Io *io = entry->io;

	uint64_t toRead = std::min<uint64_t>(entry->data.size - entry->currentOffset, len);
	uint64_t offs = entry->currentOffset;

	io->seek(io, entry->data.offset + offs);

	/* We divide up the bytes to be read in 3 categories:
	 *
	 * preAlign: If the current read address is not dword
	 *   aligned, this is the number of bytes to read til
	 *   we reach alignment again (therefore can only be
	 *   3 or less).
	 *
	 * align: The number of aligned dwords we can read
	 *   times 4 (= number of bytes).
	 *
	 * postAlign: The number of bytes to read after the
	 *   last aligned dword. Always 3 or less.
	 *
	 * Treating the pre- and post aligned reads specially,
	 * we can read all aligned dwords in one syscall directly
	 * into the write buffer and then run the xor chain on
	 * it afterwards. */

	uint8_t preAlign = 4 - (offs % 4);

	if (preAlign == 4)
		preAlign = 0;
	else
		preAlign = std::min<uint64_t>(preAlign, len);

	uint8_t postAlign = (len > preAlign) ? (offs + len) % 4 : 0;

	uint64_t align = len - (preAlign + postAlign);

	/* Byte buffer pointer */
	uint8_t *bBufferP = static_cast<uint8_t*>(buffer);

	if (preAlign > 0)
	{
		uint32_t dword;
		io->read(io, &dword, preAlign);

		/* Need to align the bytes with the
		 * magic before xoring */
		dword <<= 8 * (offs % 4);
		dword ^= entry->currentMagic;

		/* Shift them back to normal */
		dword >>= 8 * (offs % 4);
		memcpy(bBufferP, &dword, preAlign);

		bBufferP += preAlign;

		/* Only advance the magic if we actually
		 * reached the next alignment */
		if ((offs+preAlign) % 4 == 0)
			advanceMagic(entry->currentMagic);
	}

	if (align > 0)
	{
		/* Double word buffer pointer */
		uint32_t *dwBufferP = reinterpret_cast<uint32_t*>(bBufferP);

		/* Read aligned dwords in one go */
		io->read(io, bBufferP, align);

		/* Then xor them */
		for (uint64_t i = 0; i < (align / 4); ++i)
			dwBufferP[i] ^= advanceMagic(entry->currentMagic);

		bBufferP += align;
	}

	if (postAlign > 0)
	{
		uint32_t dword;
		io->read(io, &dword, postAlign);

		/* Bytes are already aligned with magic */
		dword ^= entry->currentMagic;
		memcpy(bBufferP, &dword, postAlign);
	}

	entry->currentOffset += toRead;

	return toRead;
}

static int
RGSS_ioSeek(PHYSFS_Io *self, PHYSFS_uint64 offset)
{
	RGSS_entryHandle *entry = static_cast<RGSS_entryHandle*>(self->opaque);

	if (offset == entry->currentOffset)
		return 1;

	if (offset > entry->data.size-1)
		return 0;

	/* If rewinding, we need to rewind to begining */
	if (offset < entry->currentOffset)
	{
		entry->currentOffset = 0;
		entry->currentMagic = entry->data.startMagic;
	}

	/* For each overstepped alignment, advance magic */
	uint64_t currentDword = entry->currentOffset / 4;
	uint64_t targetDword  = offset / 4;
	uint64_t dwordsSought = targetDword - currentDword;

	for (uint64_t i = 0; i < dwordsSought; ++i)
		advanceMagic(entry->currentMagic);

	entry->currentOffset = offset;
	entry->io->seek(entry->io, entry->data.offset + entry->currentOffset);

	return 1;
}

static PHYSFS_sint64
RGSS_ioTell(PHYSFS_Io *self)
{
	RGSS_entryHandle *entry = static_cast<RGSS_entryHandle*>(self->opaque);

	return entry->currentOffset;
}

static PHYSFS_sint64
RGSS_ioLength(PHYSFS_Io *self)
{
	RGSS_entryHandle *entry = static_cast<RGSS_entryHandle*>(self->opaque);

	return entry->data.size;
}

static PHYSFS_Io*
RGSS_ioDuplicate(PHYSFS_Io *self)
{
	RGSS_entryHandle *entry = static_cast<RGSS_entryHandle*>(self->opaque);
	RGSS_entryHandle *entryDup = new RGSS_entryHandle(*entry);

	PHYSFS_Io *dup = PHYSFS_ALLOC(PHYSFS_Io);
	*dup = *self;
	dup->opaque = entryDup;

	return dup;
}

static void
RGSS_ioDestroy(PHYSFS_Io *self)
{
	RGSS_entryHandle *entry = static_cast<RGSS_entryHandle*>(self->opaque);

	delete entry;

	PHYSFS_getAllocator()->Free(self);
}

static const PHYSFS_Io RGSS_IoTemplate =
{
    0, /* version */
    0, /* opaque */
    RGSS_ioRead,
    0, /* write */
    RGSS_ioSeek,
    RGSS_ioTell,
    RGSS_ioLength,
    RGSS_ioDuplicate,
    0, /* flush */
    RGSS_ioDestroy
};

static void*
RGSS_openArchive(PHYSFS_Io *io, const char *, int forWrite)
{
	if (forWrite)
		return 0;

	/* Check header */
	uint32_t header1, header2;

	if (!readUint32(io, header1))
		return 0;

	if (!readUint32(io, header2))
		return 0;

	if (header1 != RGSS_HEADER_1 || header2 != RGSS_HEADER_2)
		return 0;

	RGSS_archiveData *data = new RGSS_archiveData;
	data->archiveIo = io;

	uint32_t magic = RGSS_MAGIC;

	/* Top level entry list */
	BoostSet<std::string> &topLevel = data->dirHash[""];

	while (true)
	{
		/* Read filename length,
         * if nothing was read, no files remain */
		uint32_t nameLen;

		if (!readUint32(io, nameLen))
			break;

		nameLen ^= advanceMagic(magic);

		static char nameBuf[512];
		uint i;
		for (i = 0; i < nameLen; ++i)
		{
			char c;
			io->read(io, &c, 1);
			nameBuf[i] = c ^ (advanceMagic(magic) & 0xFF);
			if (nameBuf[i] == '\\')
				nameBuf[i] = '/';
		}
		nameBuf[i] = 0;

		uint32_t entrySize;
		readUint32(io, entrySize);
		entrySize ^= advanceMagic(magic);

		RGSS_entryData entry;
		entry.offset = io->tell(io);
		entry.size = entrySize;
		entry.startMagic = magic;

		data->entryHash.insert(nameBuf, entry);

		/* Check for top level entries */
		for (i = 0; i < nameLen; ++i)
		{
			bool slash = nameBuf[i] == '/';
			if (!slash && i+1 < nameLen)
				continue;

			if (slash)
				nameBuf[i] = '\0';

			topLevel.insert(nameBuf);

			if (slash)
				nameBuf[i] = '/';
		}

		/* Check for more entries */
		for (i = nameLen; i > 0; i--)
			if (nameBuf[i] == '/')
			{
				nameBuf[i] = '\0';

				const char *dir = nameBuf;
				const char *entry = &nameBuf[i+1];

				BoostSet<std::string> &entryList = data->dirHash[dir];
				entryList.insert(entry);
			}

		io->seek(io, entry.offset + entry.size);
	}

	return data;
}

static void
RGSS_enumerateFiles(void *opaque, const char *dirname,
                    PHYSFS_EnumFilesCallback cb,
                    const char *origdir, void *callbackdata)
{
	RGSS_archiveData *data = static_cast<RGSS_archiveData*>(opaque);

	std::string _dirname(dirname);

	if (!data->dirHash.contains(_dirname))
		return;

	const BoostSet<std::string> &entries = data->dirHash[_dirname];

	BoostSet<std::string>::const_iterator iter;
	for (iter = entries.cbegin(); iter != entries.cend(); ++iter)
		cb(callbackdata, origdir, iter->c_str());
}

static PHYSFS_Io*
RGSS_openRead(void *opaque, const char *filename)
{
	RGSS_archiveData *data = static_cast<RGSS_archiveData*>(opaque);

	if (!data->entryHash.contains(filename))
		return 0;

	RGSS_entryHandle *entry =
	        new RGSS_entryHandle(data->entryHash[filename], data->archiveIo);

	PHYSFS_Io *io = PHYSFS_ALLOC(PHYSFS_Io);

	*io = RGSS_IoTemplate;
	io->opaque = entry;

	return io;
}

static int
RGSS_stat(void *opaque, const char *filename, PHYSFS_Stat *stat)
{
	RGSS_archiveData *data = static_cast<RGSS_archiveData*>(opaque);

	bool hasFile = data->entryHash.contains(filename);
	bool hasDir  = data->dirHash.contains(filename);

	if (!hasFile && !hasDir)
	{
		PHYSFS_setErrorCode(PHYSFS_ERR_NOT_FOUND);
		return 0;
	}

	stat->modtime    =
	stat->createtime =
	stat->accesstime = 0;
	stat->readonly   = 1;

	if (hasFile)
	{
		RGSS_entryData &entry = data->entryHash[filename];

		stat->filesize = entry.size;
		stat->filetype = PHYSFS_FILETYPE_REGULAR;
	}
	else
	{
		stat->filesize = 0;
		stat->filetype = PHYSFS_FILETYPE_DIRECTORY;
	}

	return 1;
}

static void
RGSS_closeArchive(void *opaque)
{
	RGSS_archiveData *data = static_cast<RGSS_archiveData*>(opaque);

	delete data;
}

static PHYSFS_Io*
RGSS_noop1(void*, const char*)
{
	return 0;
}

static int
RGSS_noop2(void*, const char*)
{
	return 0;
}

static const PHYSFS_Archiver RGSS_Archiver =
{
	0,
	{
		"RGSSAD",
		"RGSS encrypted archive format",
		"Jonas Kulla <Nyocurio@gmail.com>",
		"http://k-du.de/rgss/rgss.html",
		0 /* symlinks not supported */
	},
	RGSS_openArchive,
	RGSS_enumerateFiles,
	RGSS_openRead,
	RGSS_noop1, /* openWrite */
	RGSS_noop1, /* openAppend */
	RGSS_noop2, /* remove */
	RGSS_noop2, /* mkdir */
	RGSS_stat,
	RGSS_closeArchive
};


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

	/* Font extensions */
	p->extensions[Font].push_back("ttf");
	p->extensions[Font].push_back("otf");

	PHYSFS_init(argv0);
	PHYSFS_registerArchiver(&RGSS_Archiver);

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

static void cacheEnumCB(void *d, const char *origdir,
                        const char *fname)
{
	FileSystemPrivate *p = static_cast<FileSystemPrivate*>(d);

	char buf[512];

	if (*origdir == '\0')
		strncpy(buf, fname, sizeof(buf));
	else
		snprintf(buf, sizeof(buf), "%s/%s", origdir, fname);

	char *ptr = buf;

	/* Trim leading slash */
	if (*ptr == '/')
		++ptr;

	std::string mixedCase(ptr);

	for (char *p = buf; *p; ++p)
		*p = tolower(*p);

	std::string lowerCase(ptr);

	p->pathCache.insert(lowerCase, mixedCase);

	PHYSFS_enumerateFilesCallback(mixedCase.c_str(), cacheEnumCB, p);
}

void FileSystem::createPathCache()
{
	PHYSFS_enumerateFilesCallback("", cacheEnumCB, p);

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
