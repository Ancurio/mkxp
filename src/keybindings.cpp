/*
** keybindings.cpp
**
** This file is part of mkxp.
**
** Copyright (C) 2014 Jonas Kulla <Nyocurio@gmail.com>
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

#include "keybindings.h"

#include "config.h"
#include "util.h"

#include <stdio.h>

struct KbBindingData
{
	SDL_Scancode source;
	Input::ButtonCode target;

	void add(BDescVec &d) const
	{
		SourceDesc src;
		src.type = Key;
		src.d.scan = source;

		BindingDesc desc;
		desc.src = src;
		desc.target = target;

		d.push_back(desc);
	}
};

struct JsBindingData
{
	int source;
	Input::ButtonCode target;

	void add(BDescVec &d) const
	{
		SourceDesc src;
		src.type = JButton;
		src.d.jb = source;

		BindingDesc desc;
		desc.src = src;
		desc.target = target;

		d.push_back(desc);
	}
};

struct GcBindingData
{
	int source;
	Input::ButtonCode target;

	void add(BDescVec &d) const
	{
		SourceDesc src;
		src.type = CButton;
		src.d.jb = source;

		BindingDesc desc;
		desc.src = src;
		desc.target = target;

		d.push_back(desc);
	}
};

/* Common */
static const KbBindingData defaultKbBindings[] =
{
    { SDL_SCANCODE_LEFT,   Input::Left       },
	{ SDL_SCANCODE_RIGHT,  Input::Right      },
	{ SDL_SCANCODE_UP,     Input::Up         },
	{ SDL_SCANCODE_DOWN,   Input::Down       },
	{ SDL_SCANCODE_Z,      Input::Action     },
    { SDL_SCANCODE_SPACE,  Input::Action     },
	{ SDL_SCANCODE_X,      Input::Cancel     },
    { SDL_SCANCODE_ESCAPE, Input::Cancel     },
	{ SDL_SCANCODE_A,      Input::Menu       },
    { SDL_SCANCODE_RETURN, Input::Menu       },
	{ SDL_SCANCODE_S,      Input::Items      },
    { SDL_SCANCODE_LSHIFT, Input::Run        },
	{ SDL_SCANCODE_LCTRL,  Input::Deactivate },
    { SDL_SCANCODE_Q,      Input::L          },
	{ SDL_SCANCODE_W,      Input::R          },
};

static elementsN(defaultKbBindings);

static const GcBindingData defaultGcBindings[] =
{
    { SDL_CONTROLLER_BUTTON_DPAD_LEFT,     Input::Left       },
    { SDL_CONTROLLER_BUTTON_DPAD_RIGHT,    Input::Right      },
    { SDL_CONTROLLER_BUTTON_DPAD_UP,       Input::Up         },
    { SDL_CONTROLLER_BUTTON_DPAD_DOWN,     Input::Down       },
    { SDL_CONTROLLER_BUTTON_A,             Input::Action     },
    { SDL_CONTROLLER_BUTTON_B,             Input::Cancel     },
    { SDL_CONTROLLER_BUTTON_X,             Input::Run        },
    { SDL_CONTROLLER_BUTTON_Y,             Input::Items      },
    { SDL_CONTROLLER_BUTTON_START,         Input::Menu       },
    { SDL_CONTROLLER_BUTTON_BACK,          Input::Deactivate },
    { SDL_CONTROLLER_BUTTON_LEFTSHOULDER,  Input::L          },
    { SDL_CONTROLLER_BUTTON_RIGHTSHOULDER, Input::R          },
};

static elementsN(defaultGcBindings);

static void addGcAxisBinding(BDescVec &d, uint8_t axis, AxisDir dir, Input::ButtonCode target)
{
	SourceDesc src;
	src.type = CAxis;
	src.d.ja.axis = axis;
	src.d.ja.dir = dir;

	BindingDesc desc;
	desc.src = src;
	desc.target = target;

	d.push_back(desc);
}

BDescVec genDefaultBindings()
{
	BDescVec d;

	for (size_t i = 0; i < defaultKbBindingsN; ++i)
		defaultKbBindings[i].add(d);

	for (size_t i = 0; i < defaultGcBindingsN; ++i)
		defaultGcBindings[i].add(d);

	addGcAxisBinding(d, SDL_CONTROLLER_AXIS_LEFTX,        Negative, Input::Left      );
	addGcAxisBinding(d, SDL_CONTROLLER_AXIS_LEFTX,        Positive, Input::Right     );
	addGcAxisBinding(d, SDL_CONTROLLER_AXIS_LEFTY,        Negative, Input::Up        );
	addGcAxisBinding(d, SDL_CONTROLLER_AXIS_LEFTY,        Positive, Input::Down      );
	addGcAxisBinding(d, SDL_CONTROLLER_AXIS_TRIGGERLEFT,  Positive, Input::Deactivate);
	addGcAxisBinding(d, SDL_CONTROLLER_AXIS_TRIGGERRIGHT, Positive, Input::Run       );

	return d;
}

#define FORMAT_VER 0

struct Header
{
	uint32_t formVer;
	uint32_t count;
};

static void buildPath(const std::string &dir, char *out, size_t outSize)
{
	snprintf(out, outSize, "%skeybindings", dir.c_str());
}

static bool writeBindings(const BDescVec &d, const std::string &dir)
{
	if (dir.empty())
		return false;

	char path[1024];
	buildPath(dir, path, sizeof(path));

	FILE *f = fopen(path, "wb");

	if (!f)
		return false;

	Header hd;
	hd.formVer = FORMAT_VER;
	hd.count = d.size();

	if (fwrite(&hd, sizeof(hd), 1, f) < 1)
	{
		fclose(f);
		return false;
	}

	if (fwrite(&d[0], sizeof(d[0]), hd.count, f) < hd.count)
	{
		fclose(f);
		return false;
	}

	fclose(f);
	return true;
}

void storeBindings(const BDescVec &d, const Config &conf)
{
	if (writeBindings(d, conf.customDataPath))
		return;

	writeBindings(d, conf.commonDataPath);
}

#define READ(ptr, size, n, f) if (fread(ptr, size, n, f) < n) return false

static bool verifyDesc(const BindingDesc &desc)
{
	const Input::ButtonCode codes[] =
	{
	    Input::None,
	    Input::Down, Input::Left, Input::Right, Input::Up,
	    Input::Action,
	    Input::Cancel,
	    Input::Menu,
	    Input::Items,
	    Input::Run,
	    Input::Deactivate,
	    Input::L, Input::R,
	    Input::F5, Input::F6, Input::F7, Input::F8, Input::F9
	};

	elementsN(codes);
	size_t i;

	for (i = 0; i < codesN; ++i)
		if (desc.target == codes[i])
			break;

	if (i == codesN)
		return false;

	const SourceDesc &src = desc.src;

	switch (src.type)
	{
	case Invalid:
		return true;
	case Key:
		return src.d.scan < SDL_NUM_SCANCODES;
	case CButton:
	case JButton:
		return true;
	case JHat:
		/* Only accept single directional binds */
		return src.d.jh.pos == SDL_HAT_LEFT || src.d.jh.pos == SDL_HAT_RIGHT ||
		       src.d.jh.pos == SDL_HAT_UP   || src.d.jh.pos == SDL_HAT_DOWN;
	case CAxis:
	case JAxis:
		return src.d.ja.dir == Negative || src.d.ja.dir == Positive;
	default:
		return false;
	}
}

static bool readBindings(BDescVec &out, const std::string &dir)
{
	if (dir.empty())
		return false;

	char path[1024];
	buildPath(dir, path, sizeof(path));

	FILE *f = fopen(path, "rb");

	if (!f)
		return false;

	Header hd;
	if (fread(&hd, sizeof(hd), 1, f) < 1)
	{
		fclose(f);
		return false;
	}

	if (hd.formVer != FORMAT_VER)
		return false;
	/* Arbitrary max value */
	if (hd.count > 1024)
		return false;

	out.resize(hd.count);
	if (fread(&out[0], sizeof(out[0]), hd.count, f) < hd.count)
	{
		fclose(f);
		return false;
	}

	for (size_t i = 0; i < hd.count; ++i)
		if (!verifyDesc(out[i]))
			return false;

	return true;
}

BDescVec loadBindings(const Config &conf)
{
	BDescVec d;

	if (readBindings(d, conf.customDataPath))
		return d;

	if (readBindings(d, conf.commonDataPath))
		return d;

	return genDefaultBindings();
}
