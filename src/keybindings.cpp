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

/* Common */
static const KbBindingData defaultKbBindings[] =
{
	{ SDL_SCANCODE_LEFT,   Input::Left  },
	{ SDL_SCANCODE_RIGHT,  Input::Right },
	{ SDL_SCANCODE_UP,     Input::Up    },
	{ SDL_SCANCODE_DOWN,   Input::Down  },
    
#ifndef MARIN
	{ SDL_SCANCODE_SPACE,  Input::ZL    },
	{ SDL_SCANCODE_RETURN, Input::ZL    },
	{ SDL_SCANCODE_ESCAPE, Input::B     },
	{ SDL_SCANCODE_KP_0,   Input::B     },
	{ SDL_SCANCODE_LSHIFT, Input::A     },
	{ SDL_SCANCODE_X,      Input::B     },
	{ SDL_SCANCODE_D,      Input::ZR    },
	{ SDL_SCANCODE_Q,      Input::L     },
	{ SDL_SCANCODE_W,      Input::R     },
	{ SDL_SCANCODE_A,      Input::X     },
	{ SDL_SCANCODE_S,      Input::Y     }
#else
    { SDL_SCANCODE_D,      Input::ZL    },
    { SDL_SCANCODE_F,      Input::ZR    },
    { SDL_SCANCODE_A,      Input::L     },
    { SDL_SCANCODE_S,      Input::R     },
    { SDL_SCANCODE_C,      Input::A     },
    { SDL_SCANCODE_X,      Input::B     },
    { SDL_SCANCODE_Z,      Input::X     },
    { SDL_SCANCODE_V,      Input::Y     }
#endif
};

#ifndef MARIN
/* RGSS1 */
static const KbBindingData defaultKbBindings1[] =
{
	{ SDL_SCANCODE_Z,      Input::A     },
	{ SDL_SCANCODE_C,      Input::ZL     },
};

/* RGSS2 and higher */
static const KbBindingData defaultKbBindings2[] =
{
	{ SDL_SCANCODE_Z,      Input::ZL     }
};
#endif

static elementsN(defaultKbBindings);

#ifndef MARIN
static elementsN(defaultKbBindings1);
static elementsN(defaultKbBindings2);
#endif

static const JsBindingData defaultJsBindings[] =
{
	{ 3, Input::A  },
	{ 0, Input::B  },
	{ 1, Input::ZL },
	{ 2, Input::X  },
	{ 4, Input::Y  },
	{ 5, Input::ZR },
	{ 9, Input::L  },
	{ 10, Input::R  }
};

static elementsN(defaultJsBindings);

static void addAxisBinding(BDescVec &d, uint8_t axis, AxisDir dir, Input::ButtonCode target)
{
	SourceDesc src;
	src.type = JAxis;
	src.d.ja.axis = axis;
	src.d.ja.dir = dir;

	BindingDesc desc;
	desc.src = src;
	desc.target = target;

	d.push_back(desc);
}

static void addHatBinding(BDescVec &d, uint8_t hat, uint8_t pos, Input::ButtonCode target)
{
	SourceDesc src;
	src.type = JHat;
	src.d.jh.hat = hat;
	src.d.jh.pos = pos;

	BindingDesc desc;
	desc.src = src;
	desc.target = target;

	d.push_back(desc);
}

BDescVec genDefaultBindings(const Config &conf)
{
	BDescVec d;

	for (size_t i = 0; i < defaultKbBindingsN; ++i)
		defaultKbBindings[i].add(d);
#ifndef MARIN
	if (conf.rgssVersion == 1)
		for (size_t i = 0; i < defaultKbBindings1N; ++i)
			defaultKbBindings1[i].add(d);
	else
		for (size_t i = 0; i < defaultKbBindings2N; ++i)
			defaultKbBindings2[i].add(d);
#endif
	for (size_t i = 0; i < defaultJsBindingsN; ++i)
		defaultJsBindings[i].add(d);

	addAxisBinding(d, 0, Negative, Input::Left );
	addAxisBinding(d, 0, Positive, Input::Right);
	addAxisBinding(d, 1, Negative, Input::Up   );
	addAxisBinding(d, 1, Positive, Input::Down );
	
	addHatBinding(d, 0, SDL_HAT_LEFT,  Input::Left );
	addHatBinding(d, 0, SDL_HAT_RIGHT, Input::Right);
	addHatBinding(d, 0, SDL_HAT_UP,    Input::Up   );
	addHatBinding(d, 0, SDL_HAT_DOWN,  Input::Down );

	return d;
}

#define FORMAT_VER 2

struct Header
{
	uint32_t formVer;
	uint32_t rgssVer;
	uint32_t count;
};

static void buildPath(const std::string &dir, uint32_t rgssVersion,
                      char *out, size_t outSize)
{
	snprintf(out, outSize, "%skeybindings.mkxp%u", dir.c_str(), rgssVersion);
}

static bool writeBindings(const BDescVec &d, const std::string &dir,
                          uint32_t rgssVersion)
{
	if (dir.empty())
		return false;

	char path[1024];
	buildPath(dir, rgssVersion, path, sizeof(path));

	FILE *f = fopen(path, "wb");

	if (!f)
		return false;

	Header hd;
	hd.formVer = FORMAT_VER;
	hd.rgssVer = rgssVersion;
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
	if (writeBindings(d, conf.customDataPath, conf.rgssVersion))
		return;

	writeBindings(d, conf.commonDataPath, conf.rgssVersion);
}

#define READ(ptr, size, n, f) if (fread(ptr, size, n, f) < n) return false

static bool verifyDesc(const BindingDesc &desc)
{
	const Input::ButtonCode codes[] =
	{
	    Input::None,
	    Input::Down, Input::Left, Input::Right, Input::Up,
	    Input::A, Input::B, Input::ZL,
	    Input::X, Input::Y, Input::ZR,
	    Input::L, Input::R,
	    Input::Shift, Input::Ctrl, Input::Alt,
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
	case JButton:
		return true;
	case JHat:
		/* Only accept single directional binds */
		return src.d.jh.pos == SDL_HAT_LEFT || src.d.jh.pos == SDL_HAT_RIGHT ||
		       src.d.jh.pos == SDL_HAT_UP   || src.d.jh.pos == SDL_HAT_DOWN;
	case JAxis:
		return src.d.ja.dir == Negative || src.d.ja.dir == Positive;
	default:
		return false;
	}
}

static bool readBindings(BDescVec &out, const std::string &dir,
                         uint32_t rgssVersion)
{
	if (dir.empty())
		return false;

	char path[1024];
	buildPath(dir, rgssVersion, path, sizeof(path));

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
	if (hd.rgssVer != rgssVersion)
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

	if (readBindings(d, conf.customDataPath, conf.rgssVersion))
		return d;

	if (readBindings(d, conf.commonDataPath, conf.rgssVersion))
		return d;

	return genDefaultBindings(conf);
}
