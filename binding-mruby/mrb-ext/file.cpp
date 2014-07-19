/*
** file.cpp
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

#include "file.h"

#include "debugwriter.h"

#include "../binding-util.h"
#include <mruby/string.h>
#include <mruby/array.h>
#include <mruby/class.h>

#include <SDL_rwops.h>

#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <libgen.h>
#include <sys/stat.h>

#include <vector>

extern mrb_value timeFromSecondsInt(mrb_state *mrb, time_t seconds);

static void
checkValid(mrb_state *mrb, FileImpl *p, int rwMode = FileImpl::ReadWrite)
{
	MrbData *data = getMrbData(mrb);

	if (p->closed)
		mrb_raise(mrb, data->exc[IO], "closed stream");

	if (!(p->mode & rwMode))
	{
		const char *errorMsg = 0;

		switch (rwMode)
		{
		case FileImpl::Read :
			errorMsg = "not openend for reading"; break;
		case FileImpl::Write :
			errorMsg = "not openend for writing"; break;
		default: break;
		}

		mrb_raise(mrb, data->exc[IO], errorMsg);
	}
}

extern const mrb_data_type FileType =
{
    "File",
    freeInstance<FileImpl>
};

static int
getOpenMode(const char *mode)
{
#define STR_CASE(cs, ret) else if (!strcmp(mode, cs)) { return FileImpl:: ret; }

	if (!strcmp(mode, "r"))
		return FileImpl::Read;
	STR_CASE("rb", Read)
	STR_CASE("r+", ReadWrite)
	STR_CASE("w", Write)
	STR_CASE("wb", Write)
	STR_CASE("w+", ReadWrite)
	STR_CASE("rw", ReadWrite)
	STR_CASE("a", Write)
	STR_CASE("a+", ReadWrite)

	Debug() << "getOpenMode failed for:" << mode;
	return 0;
}

static void handleErrno(mrb_state *mrb)
{
	MrbException exc;
	const char *msg;
	mrb_value arg1;

#define ENO(_eno, _msg) \
	case _eno: { exc = Errno##_eno; msg = _msg; break; }

	switch (errno)
	{
	ENO(EACCES, "");
	ENO(EBADF, "");
	ENO(EEXIST, "");
	ENO(EINVAL, "");
	ENO(EMFILE, "");
	ENO(ENOMEM, "");
	ENO(ERANGE, "");
	default:
		exc = IO; msg = "Unknown Errno: %S";
		arg1 = mrb_any_to_s(mrb, mrb_fixnum_value(errno));
	}

#undef ENO

	mrb_raisef(mrb, getMrbData(mrb)->exc[exc], msg, arg1);
}

#define GUARD_ERRNO(stmnt) \
{ \
	errno = 0; \
	{ stmnt } \
	if (errno != 0) \
		handleErrno(mrb); \
}


static char *
findLastDot(char *str)
{
	char *find = 0;
	char *ptr = str;

	/* Dot in front is not considered */
	if (*ptr == '.')
		ptr++;

	for (; *ptr; ++ptr)
	{
		/* Dot in dirpath is not considered */
		if (*ptr == '/')
		{
			ptr++;
			if (*ptr == '.')
				ptr++;
		}

		if (*ptr == '.')
			find = ptr;
	}

	return find;
}

/* File class methods */
MRB_FUNCTION(fileBasename)
{
	mrb_value filename;
	const char *suffix = 0;

	mrb_get_args(mrb, "S|z", &filename, &suffix);

	char *_filename = RSTRING_PTR(filename);
	char *base = basename(_filename);

	char *ext = 0;

	if (suffix)
	{
		ext = findLastDot(base);
		if (ext && !strcmp(ext, suffix))
			*ext = '\0';
	}

	mrb_value str = mrb_str_new_cstr(mrb, base);

	/* Restore param string */
	if (ext)
		*ext = '.';

	return str;
}

MRB_FUNCTION(fileDelete)
{
	mrb_int argc;
	mrb_value *argv;

	mrb_get_args(mrb, "*", &argv, &argc);

	for (int i = 0; i < argc; ++i)
	{
		mrb_value &v = argv[i];

		if (!mrb_string_p(v))
			continue;

		const char *filename = RSTRING_PTR(v);
		GUARD_ERRNO( unlink(filename); )
	}

	return mrb_nil_value();
}

MRB_FUNCTION(fileDirname)
{
	mrb_value filename;
	mrb_get_args(mrb, "S", &filename);

	char *_filename = RSTRING_PTR(filename);
	char *dir = dirname(_filename);

	return mrb_str_new_cstr(mrb, dir);
}

MRB_FUNCTION(fileExpandPath)
{
	const char *path;
	const char *defDir = 0;

	mrb_get_args(mrb, "z|z", &path, &defDir);

	// FIXME No idea how to integrate 'default_dir' right now
	if (defDir)
		Debug() << "FIXME: File.expand_path: default_dir not implemented";

	char buffer[PATH_MAX];
	char *unused = realpath(path, buffer);
	(void) unused;

	return mrb_str_new_cstr(mrb, buffer);
}

MRB_FUNCTION(fileExtname)
{
	mrb_value filename;
	mrb_get_args(mrb, "S", &filename);

	char *ext = findLastDot(RSTRING_PTR(filename));

	return mrb_str_new_cstr(mrb, ext);
}

MRB_FUNCTION(fileOpen)
{
	mrb_value path;
	const char *mode = "r";
	mrb_value block = mrb_nil_value();

	mrb_get_args(mrb, "S|z&", &path, &mode, &block);

	/* We manually check errno because we're supplying the
	 * filename on ENOENT */
	errno = 0;
	FILE *f = fopen(RSTRING_PTR(path), mode);
	if (!f)
	{
		if (errno == ENOENT)
			mrb_raisef(mrb, getMrbData(mrb)->exc[ErrnoENOENT],
					   "No such file or directory - %S", path);
		else
			handleErrno(mrb);
	}

	SDL_RWops *ops = SDL_RWFromFP(f, SDL_TRUE);
	FileImpl *p = new FileImpl(ops, getOpenMode(mode));

	mrb_value obj = wrapObject(mrb, p, FileType);
	setProperty(mrb, obj, CSpath, path);

	if (mrb_type(block) == MRB_TT_PROC)
	{
		// FIXME inquire how GC behaviour works here for obj
		mrb_value ret = mrb_yield(mrb, block, obj);
		p->close();
		return ret;
	}

	return obj;
}

MRB_FUNCTION(fileRename)
{
	const char *from, *to;
	mrb_get_args(mrb, "zz", &from, &to);

	GUARD_ERRNO( rename(from, to); )

	return mrb_fixnum_value(0);
}


MRB_FUNCTION(mrbNoop)
{
	MRB_FUN_UNUSED_PARAM;

	return mrb_nil_value();
}

/* File instance methods */
MRB_METHOD(fileClose)
{
	FileImpl *p = getPrivateData<FileImpl>(mrb, self);
	checkValid(mrb, p);

	GUARD_ERRNO( p->close(); )

	return self;
}

static void
readLine(FILE *f, std::vector<char> &buffer)
{
	buffer.clear();

	while (true)
	{
		if (feof(f))
			break;

		int c = fgetc(f);

		if (c == '\n' || c == EOF)
			break;

		buffer.push_back(c);
	}
}

MRB_METHOD(fileEachLine)
{
	FileImpl *p = getPrivateData<FileImpl>(mrb, self);
	checkValid(mrb, p, FileImpl::Read);
	FILE *f = p->fp();

	mrb_value block;
	mrb_get_args(mrb, "&", &block);

	(void) f;
	std::vector<char> buffer;

	mrb_value str = mrb_str_buf_new(mrb, 0);

	while (feof(f) == 0)
	{
		GUARD_ERRNO( readLine(f, buffer); )

		if (buffer.empty() && feof(f) != 0)
			break;

		size_t lineLen = buffer.size();
		mrb_str_resize(mrb, str, lineLen);
		memcpy(RSTRING_PTR(str), &buffer[0], lineLen);

		mrb_yield(mrb, block, str);
	}

	return self;
}

MRB_METHOD(fileEachByte)
{
	FileImpl *p = getPrivateData<FileImpl>(mrb, self);
	checkValid(mrb, p, FileImpl::Read);
	FILE *f = p->fp();

	mrb_value block;
	mrb_get_args(mrb, "&", &block);

	GUARD_ERRNO(
	while (feof(f) == 0)
	{
		mrb_int byte = fgetc(f);

		if (byte == -1)
			break;

		mrb_yield(mrb, block, mrb_fixnum_value(byte));
	})

	return self;
}

MRB_METHOD(fileIsEof)
{
	FileImpl *p = getPrivateData<FileImpl>(mrb, self);
	checkValid(mrb, p);
	FILE *f = p->fp();

	bool isEof;
	GUARD_ERRNO( isEof = feof(f) != 0; )

	return mrb_bool_value(isEof);
}

MRB_METHOD(fileSetPos)
{
	FileImpl *p = getPrivateData<FileImpl>(mrb, self);
	checkValid(mrb, p);
	FILE *f = p->fp();

	mrb_int pos;
	mrb_get_args(mrb, "i", &pos);

	GUARD_ERRNO( fseek(f, pos, SEEK_SET); )

	return self;
}

MRB_METHOD(fileGetPos)
{
	FileImpl *p = getPrivateData<FileImpl>(mrb, self);
	checkValid(mrb, p);
	FILE *f = p->fp();

	long pos;
	GUARD_ERRNO( pos = ftell(f); )

	return mrb_fixnum_value(pos);
}

MRB_METHOD(fileRead)
{
	FileImpl *p = getPrivateData<FileImpl>(mrb, self);
	checkValid(mrb, p, FileImpl::Read);
	FILE *f = p->fp();

	long cur, size;
	GUARD_ERRNO( cur = ftell(f); )
	GUARD_ERRNO( fseek(f, 0, SEEK_END); )
	GUARD_ERRNO( size = ftell(f); )
	GUARD_ERRNO( fseek(f, cur, SEEK_SET); )

	mrb_int length = size - cur;
	mrb_get_args(mrb, "|i", &length);

	mrb_value str = mrb_str_new(mrb, 0, 0);
	mrb_str_resize(mrb, str, length);

	GUARD_ERRNO( int unused = fread(RSTRING_PTR(str), 1, length, f); (void) unused; )

	return str;
}

// FIXME this function always splits on newline right now,
// to make rs fully work, I'll have to use some strrstr magic I guess
MRB_METHOD(fileReadLines)
{
	FileImpl *p = getPrivateData<FileImpl>(mrb, self);
	checkValid(mrb, p, FileImpl::Read);
	FILE *f = p->fp();

	mrb_value arg;
	mrb_get_args(mrb, "|o", &arg);

	const char *rs = "\n"; (void) rs;
	if (mrb->c->ci->argc > 0)
	{
		Debug() << "FIXME: File.readlines: rs not implemented";

		if (mrb_string_p(arg))
			rs = RSTRING_PTR(arg);
		else if (mrb_nil_p(arg))
			rs = "\n";
		else
			mrb_raise(mrb, getMrbData(mrb)->exc[TypeError], "Expected string or nil"); // FIXME add "got <> instead" remark
	}

	mrb_value ary = mrb_ary_new(mrb);
	std::vector<char> buffer;

	while (feof(f) == 0)
	{
		GUARD_ERRNO( readLine(f, buffer); )

		if (buffer.empty() && feof(f) != 0)
			break;

		mrb_value str = mrb_str_new(mrb, &buffer[0], buffer.size());
		mrb_ary_push(mrb, ary, str);
	}

	return ary;
}

MRB_METHOD(fileWrite)
{
	FileImpl *p = getPrivateData<FileImpl>(mrb, self);
	checkValid(mrb, p, FileImpl::Write);
	FILE *f = p->fp();

	mrb_value str;
	mrb_get_args(mrb, "S", &str);

	size_t count;
	GUARD_ERRNO( count = fwrite(RSTRING_PTR(str), 1, RSTRING_LEN(str), f); )

	return mrb_fixnum_value(count);
}

MRB_METHOD(fileGetPath)
{
	FileImpl *p = getPrivateData<FileImpl>(mrb, self);
	checkValid(mrb, p);

	return getProperty(mrb, self, CSpath);
}

static void
getFileStat(mrb_state *mrb, struct stat &fileStat)
{
	const char *filename;
	mrb_get_args(mrb, "z", &filename);

	stat(filename, &fileStat);
}

MRB_METHOD(fileGetMtime)
{
	mrb_value path = getProperty(mrb, self, CSpath);

	struct stat fileStat;
	stat(RSTRING_PTR(path), &fileStat);

	return timeFromSecondsInt(mrb, fileStat.st_mtim.tv_sec);
}

/* FileTest module functions */
MRB_FUNCTION(fileTestDoesExist)
{
	const char *filename;
	mrb_get_args(mrb, "z", &filename);

	int result = access(filename, F_OK);

	return mrb_bool_value(result == 0);
}

MRB_FUNCTION(fileTestIsFile)
{
	struct stat fileStat;
	getFileStat(mrb, fileStat);

	return mrb_bool_value(S_ISREG(fileStat.st_mode));
}

MRB_FUNCTION(fileTestIsDirectory)
{
	struct stat fileStat;
	getFileStat(mrb, fileStat);

	return mrb_bool_value(S_ISDIR(fileStat.st_mode));
}

MRB_FUNCTION(fileTestSize)
{
	struct stat fileStat;
	getFileStat(mrb, fileStat);

	return mrb_fixnum_value(fileStat.st_size);
}

void
fileBindingInit(mrb_state *mrb)
{
	mrb_define_method(mrb, mrb->kernel_module, "open", fileOpen, MRB_ARGS_REQ(1) | MRB_ARGS_OPT(1) | MRB_ARGS_BLOCK());

	RClass *klass = defineClass(mrb, "IO");
	klass = mrb_define_class(mrb, "File", klass);

	mrb_define_class_method(mrb, klass, "basename", fileBasename, MRB_ARGS_REQ(1) | MRB_ARGS_OPT(1));
	mrb_define_class_method(mrb, klass, "delete", fileDelete, MRB_ARGS_REQ(1) | MRB_ARGS_ANY());
	mrb_define_class_method(mrb, klass, "dirname", fileDirname, MRB_ARGS_REQ(1));
	mrb_define_class_method(mrb, klass, "expand_path", fileExpandPath, MRB_ARGS_REQ(1) | MRB_ARGS_OPT(1));
	mrb_define_class_method(mrb, klass, "extname", fileExtname, MRB_ARGS_REQ(1));
	mrb_define_class_method(mrb, klass, "open", fileOpen, MRB_ARGS_REQ(1) | MRB_ARGS_OPT(1) | MRB_ARGS_BLOCK());
	mrb_define_class_method(mrb, klass, "rename", fileRename, MRB_ARGS_REQ(2));

	/* IO */
	mrb_define_method(mrb, klass, "binmode", mrbNoop, MRB_ARGS_NONE());
	mrb_define_method(mrb, klass, "close", fileClose, MRB_ARGS_NONE());
	mrb_define_method(mrb, klass, "each_line", fileEachLine, MRB_ARGS_BLOCK());
	mrb_define_method(mrb, klass, "each_byte", fileEachByte, MRB_ARGS_BLOCK());
	mrb_define_method(mrb, klass, "eof?", fileIsEof, MRB_ARGS_NONE());
	mrb_define_method(mrb, klass, "pos", fileGetPos, MRB_ARGS_NONE());
	mrb_define_method(mrb, klass, "pos=", fileSetPos, MRB_ARGS_REQ(1));
	mrb_define_method(mrb, klass, "read", fileRead, MRB_ARGS_OPT(1));
	mrb_define_method(mrb, klass, "readlines", fileReadLines, MRB_ARGS_OPT(1));
	mrb_define_method(mrb, klass, "write", fileWrite, MRB_ARGS_REQ(1));

	/* File */
	mrb_define_method(mrb, klass, "mtime", fileGetMtime, MRB_ARGS_NONE());
	mrb_define_method(mrb, klass, "path", fileGetPath, MRB_ARGS_NONE());

	/* FileTest */
	RClass *module = mrb_define_module(mrb, "FileTest");
	mrb_define_module_function(mrb, module, "exist?", fileTestDoesExist, MRB_ARGS_REQ(1));
	mrb_define_module_function(mrb, module, "directory?", fileTestIsDirectory, MRB_ARGS_REQ(1));
	mrb_define_module_function(mrb, module, "file?", fileTestIsFile, MRB_ARGS_REQ(1));
	mrb_define_module_function(mrb, module, "size", fileTestSize, MRB_ARGS_REQ(1));
}
