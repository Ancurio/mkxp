/*
** filesystem-binding.cpp
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

#include "binding-util.h"

#include "globalstate.h"
#include "filesystem.h"

#include <QDebug>

DEF_TYPE(FileInt);

static VALUE
fileIntForPath(const char *path)
{
	SDL_RWops *ops = SDL_AllocRW();
	gState->fileSystem().openRead(*ops, path);

	VALUE klass = rb_const_get(rb_cObject, rb_intern("FileInt"));

	VALUE obj = rb_obj_alloc(klass);

	setPrivateData(obj, ops, FileIntType);

	return obj;
}

RB_METHOD(fileIntRead)
{

	int length = -1;
	rb_get_args(argc, argv, "i", &length);

	SDL_RWops *ops = getPrivateData<SDL_RWops>(self);

	if (length == -1)
	{
		Sint64 cur = SDL_RWtell(ops);
		Sint64 end = SDL_RWseek(ops, 0, SEEK_END);
		length = end - cur;
		SDL_RWseek(ops, cur, SEEK_SET);
	}

	if (length == 0)
		return Qnil;

	VALUE data = rb_str_new(0, length);

	SDL_RWread(ops, RSTRING_PTR(data), 1, length);

	return data;
}

RB_METHOD(fileIntClose)
{
	RB_UNUSED_PARAM;

	SDL_RWops *ops = getPrivateData<SDL_RWops>(self);
	SDL_RWclose(ops);

	return Qnil;
}

RB_METHOD(fileIntGetByte)
{
	RB_UNUSED_PARAM;

	SDL_RWops *ops = getPrivateData<SDL_RWops>(self);

	unsigned char byte;
	size_t result = SDL_RWread(ops, &byte, 1, 1);

	return (result == 1) ? rb_fix_new(byte) : Qnil;
}

RB_METHOD(fileIntBinmode)
{
	RB_UNUSED_PARAM;

	return Qnil;
}

static void
fileIntFreeInstance(void *inst)
{
	SDL_RWops *ops = static_cast<SDL_RWops*>(inst);

	SDL_RWclose(ops);
	SDL_FreeRW(ops);
}

VALUE
kernelLoadDataInt(const char *filename)
{
	rb_gc_start();

	VALUE port = fileIntForPath(filename);

	VALUE marsh = rb_const_get(rb_cObject, rb_intern("Marshal"));

	VALUE result = rb_funcall(marsh, rb_intern("load"), 1, port);

	rb_funcall(port, rb_intern("close"), 0);

	return result;
}

RB_METHOD(kernelLoadData)
{
	RB_UNUSED_PARAM;

	const char *filename;
	rb_get_args(argc, argv, "z", &filename);

	return kernelLoadDataInt(filename);
}

RB_METHOD(kernelSaveData)
{
	RB_UNUSED_PARAM;

	VALUE obj;
	VALUE filename;

	rb_get_args(argc, argv, "oS", &obj, &filename);

	VALUE file = rb_funcall(rb_cFile, rb_intern("open"), 2, filename, rb_str_new_cstr("w"));

	VALUE marsh = rb_const_get(rb_cObject, rb_intern("Marshal"));

	rb_funcall(marsh, rb_intern("dump"), 2, obj, file);

	rb_funcall(file, rb_intern("close"), 0);

	return Qnil;
}

void
fileIntBindingInit()
{
	initType(FileIntType, "FileInt", fileIntFreeInstance);

	VALUE klass = rb_define_class("FileInt", rb_cIO);

	_rb_define_method(klass, "read", fileIntRead);
//	_rb_define_method(klass, "eof?", fileIntIsEof);
//	_rb_define_method(klass, "pos", fileIntGetPos);
//	_rb_define_method(klass, "pos=", fileIntSetPos);
	_rb_define_method(klass, "getbyte", fileIntGetByte);
	_rb_define_method(klass, "binmode", fileIntBinmode);
	_rb_define_method(klass, "close", fileIntClose);

	_rb_define_module_function(rb_mKernel, "load_data", kernelLoadData);
	_rb_define_module_function(rb_mKernel, "save_data", kernelSaveData);
}
