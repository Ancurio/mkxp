/*
** settingsmenu.cpp
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

#include "settingsmenu.h"

#include <SDL_video.h>
#include <SDL_ttf.h>
#include <SDL_surface.h>
#include <SDL_keyboard.h>
#include <SDL_rect.h>

#include "keybindings.h"
#include "eventthread.h"
#include "font.h"
#include "input.h"
#include "etc-internal.h"
#include "util.h"

#include <algorithm>
#include <assert.h>

const Vec2i winSize(640, 356);

const uint8_t cBgNorm = 50;
const uint8_t cBgDark = 20;
const uint8_t cLine = 0;
const uint8_t cText = 255;

const uint8_t frameWidth = 4;
const uint8_t fontSize = 15;

static bool pointInRect(const SDL_Rect &r, int x, int y)
{
	return (x >= r.x && x <= r.x+r.w && y >= r.y && y <= r.y+r.h);
}

typedef SettingsMenuPrivate SMP;

#define BTN_STRING_CUSTOM(btn, name) { Input:: btn, name }
#define BTN_STRING(btn) BTN_STRING_CUSTOM(btn, #btn)
struct VButton
{
	Input::ButtonCode code;
	const char *str;
} static const vButtons[] =
{
	BTN_STRING(Up),
	BTN_STRING(Down),
	BTN_STRING(L),
	BTN_STRING(Left),
	BTN_STRING(Right),
	BTN_STRING(R),
	BTN_STRING(A),
	BTN_STRING(B),
#ifdef MARIN
	BTN_STRING(ZL),
#else
    BTN_STRING_CUSTOM(ZL, "C"),
#endif
	BTN_STRING(X),
	BTN_STRING(Y),
#ifdef MARIN
	BTN_STRING(ZR),
#else
    BTN_STRING_CUSTOM(ZR, "Z"),
#endif
};

static elementsN(vButtons);

/* Human readable string representation */
std::string sourceDescString(const SourceDesc &src)
{
	char buf[128];
	char pos;

	switch (src.type)
	{
	case Invalid:
		return std::string();

	case Key:
	{
		if (src.d.scan == SDL_SCANCODE_LSHIFT)
			return "Shift";

		SDL_Keycode key = SDL_GetKeyFromScancode(src.d.scan);
		const char *str = SDL_GetKeyName(key);

		if (*str == '\0')
			return "Unknown key";
		else
			return str;
	}
	case JButton:
		snprintf(buf, sizeof(buf), "JS %d", src.d.jb);
		return buf;

	case JHat:
		switch(src.d.jh.pos)
		{
		case SDL_HAT_UP:
			pos = 'U';
			break;

		case SDL_HAT_DOWN:
			pos = 'D';
			break;

		case SDL_HAT_LEFT:
			pos = 'L';
			break;

		case SDL_HAT_RIGHT:
			pos = 'R';
			break;

		default:
			pos = '-';
		}
		snprintf(buf, sizeof(buf), "Hat %d:%c",
		         src.d.jh.hat, pos);
		return buf;

	case JAxis:
		snprintf(buf, sizeof(buf), "Axis %d%c",
		         src.d.ja.axis, src.d.ja.dir == Negative ? '-' : '+');
		return buf;
	}

	assert(!"unreachable");
	return "";
}

struct Widget
{
	/* Widgets have a static size and position,
	 * defined at creation */
	Widget(SMP *p, const IntRect &rect);

	/* Public methods take coordinates in global
	 * window coordinates */
	bool hit(int x, int y);
	void draw(SDL_Surface *surf);
	void motion(int x, int y);
	void leave();
	void click(int x, int y, uint8_t button);

protected:
	SMP *p;
	IntRect rect;

	/* Protected abstract methods are called with
	 * widget-local coordinates */
	virtual void drawHandler(SDL_Surface *surf) = 0;
	virtual void motionHandler(int x, int y) = 0;
	virtual void leaveHandler() = 0;
	virtual void clickHandler(int x, int y, uint8_t button) = 0;
};

struct BindingWidget : Widget
{
	VButton vb;
	/* Source slots */
	SourceDesc src[4];
	/* Flag indicating whether a slot source is used
	 * for multiple button targets (red indicator) */
	bool dupFlag[4];

	BindingWidget(int vbIndex, SMP *p, const IntRect &rect)
	    : Widget(p, rect),
	      vb(vButtons[vbIndex]),
	      hoveredCell(-1)
	{}

	void appendBindings(BDescVec &d) const;

protected:
	int hoveredCell;
	void setHoveredCell(int cell);
	/* Get the slot cell index that contains (x,y),
	 * or -1 if none */
	int cellIndex(int x, int y) const;

	void drawHandler(SDL_Surface *surf);
	void motionHandler(int x, int y);
	void leaveHandler();
	void clickHandler(int x, int y, uint8_t button);
};

struct Button : Widget
{
	typedef void (SMP::*Callback)();

	const char *str;
	Callback cb;

	Button(SMP *p, const IntRect &rect,
	       const char *str, Callback cb)
	    : Widget(p, rect),
	      str(str), cb(cb), hovered(false)
	{}

protected:
	bool hovered;

	void setHovered(bool val);

	void drawHandler(SDL_Surface *surf);
	void motionHandler(int, int);
	void leaveHandler();
	void clickHandler(int, int, uint8_t button);
};

struct Label : Widget
{
	const char *str;
	SDL_Color c;

	Label() : Widget(0, IntRect()) {}

	Label(SMP *p, const IntRect &rect,
	      const char *str, uint8_t r, uint8_t g, uint8_t b)
	    : Widget(p, rect),
	      str(str),
	      visible(true)
	{
		c.r = r;
		c.g = g;
		c.b = b;
		c.a = 255;
	}

	void setVisible(bool val);

protected:
	bool visible;

	void drawHandler(SDL_Surface *surf);
	void motionHandler(int, int) {}
	void leaveHandler() {}
	void clickHandler(int, int, uint8_t) {}
};

enum State
{
	Idle,
	AwaitingInput
};

enum Justification
{
	Left,
	Center
};

struct SettingsMenuPrivate
{
	State state;

	/* Necessary to decide which window gets to
	 * process joystick events */
	bool hasFocus;

	/* Tell the outer EventThread to destroy us */
	bool destroyReq;

	/* Offset added for all draw calls */
	Vec2i drawOff;

	SDL_Window *window;
	SDL_Surface *winSurf;
	uint32_t winID;

	TTF_Font *font;
	SDL_PixelFormat *rgb;

	RGSSThreadData &rtData;

	std::vector<BindingWidget> bWidgets;
	std::vector<Button> buttons;
	Label infoLabel;
	Label dupWarnLabel;
	std::vector<Widget*> widgets;
	Widget *hovered;

	SourceDesc *captureDesc;
	const char *captureName;

	SettingsMenuPrivate(RGSSThreadData &rtData)
	    : rtData(rtData)
	{}

	SDL_Surface *createSurface(int w, int h)
	{
		int bpp;
		Uint32 rMask, gMask, bMask, aMask;
		SDL_PixelFormatEnumToMasks(SDL_PIXELFORMAT_ABGR8888,
		                           &bpp, &rMask, &gMask, &bMask, &aMask);

		return SDL_CreateRGBSurface(0, w, h, bpp, rMask, gMask, bMask, 0);
	}

	void fillSurface(SDL_Surface *surf, uint8_t grey)
	{
		SDL_FillRect(surf, 0, SDL_MapRGBA(rgb, grey, grey, grey, 255));
	}

	void fillRect(SDL_Surface *surf,
	              int x, int y, int w, int h,
	              uint8_t r, uint8_t g, uint8_t b)
	{
		SDL_Rect rect = { drawOff.x+x, drawOff.y+y, w, h };
		SDL_FillRect(surf, &rect, SDL_MapRGB(rgb, r, g, b));
	}

	void fillRect(SDL_Surface *surf, uint8_t grey,
	              int x, int y, int w, int h)
	{
		fillRect(surf, x, y, w, h, grey, grey, grey);
	}

	void strokeLineH(SDL_Surface *surf, uint8_t grey,
	                 int x, int y, int length, int width)
	{
		fillRect(surf, grey, x, y-width/2, length, width);
	}

	void strokeLineV(SDL_Surface *surf, uint8_t grey,
	                 int x, int y, int length, int width)
	{
		fillRect(surf, grey, x-width/2, y, width, length);
	}

	void strokeLineH(SDL_Surface *surf, uint8_t r, uint8_t g, uint8_t b,
	                 int x, int y, int length, int width)
	{
		fillRect(surf, r, g, b, x, y-width/2, length, width);
	}

	void strokeLineV(SDL_Surface *surf, uint8_t r, uint8_t g, uint8_t b,
	                 int x, int y, int length, int width)
	{
		fillRect(surf, r, g, b, x-width/2, y, width, length);
	}

	void strokeRect(SDL_Surface *surf, uint8_t grey,
	                int x, int y, int w, int h,
	                int lineW)
	{
		strokeLineH(surf, grey, x, y, w, lineW);
		strokeLineH(surf, grey, x, y+h, w, lineW);

		strokeLineV(surf, grey, x, y, h, lineW);
		strokeLineV(surf, grey, x+w, y, h, lineW);
	}

	void strokeRectInner(SDL_Surface *surf,
	                     int x, int y, int w, int h, int lineW,
	                     uint8_t r, uint8_t g, uint8_t b)
	{
		fillRect(surf, x, y, w, lineW, r, g, b);
		fillRect(surf, x, y+h-lineW, w, lineW, r, g, b);
		fillRect(surf, x, y, lineW, h, r, g, b);
		fillRect(surf, x+w-lineW, y, lineW, h, r, g ,b);
	}

	void strokeRectInner(SDL_Surface *surf, uint8_t grey,
	                     int x, int y, int w, int h,
	                     int lineW)
	{
		strokeRectInner(surf, x, y, w, h, lineW, grey, grey, grey);
	}

	void applyFontStyle(bool bold)
	{
		if (bold)
			TTF_SetFontStyle(font, TTF_STYLE_BOLD);
		else
			TTF_SetFontStyle(font, TTF_STYLE_NORMAL);
	}

	SDL_Surface *createTextSurface(const char *str, bool bold)
	{
		SDL_Color c = { cText, cText, cText, 255 };
		applyFontStyle(bold);

		return TTF_RenderUTF8_Blended(font, str, c);
	}

	SDL_Surface *createTextSurface(const char *str, SDL_Color c,
	                               bool bold)
	{
		applyFontStyle(bold);
		return TTF_RenderUTF8_Blended(font, str, c);
	}

	/* Horizontally centered */
	void blitTextSurf(SDL_Surface *surf, int x, int y,
	                  int alignW, SDL_Surface *txtSurf,
	                  Justification just)
	{
		SDL_Rect dstRect;
		dstRect.x = drawOff.x;
		dstRect.y = drawOff.y + y - txtSurf->h / 2;
		dstRect.w = txtSurf->w;
		dstRect.h = txtSurf->h;

		switch (just)
		{
		case Left:
			dstRect.x += x;
			break;
		case Center:
			dstRect.x += x - (txtSurf->w - alignW) / 2;
			break;
		}

		if (txtSurf->w <= alignW)
		{
			SDL_BlitSurface(txtSurf, 0, surf, &dstRect);
		}
		else
		{
			dstRect.w = alignW;
			dstRect.x = x;
			SDL_BlitScaled(txtSurf, 0, surf, &dstRect);
		}
	}

	void drawText(SDL_Surface *surf, const char *str,
	              int x, int y, int alignW,
	              Justification just, SDL_Color c, bool bold = false)
	{
		SDL_Surface *txt = createTextSurface(str, c, bold);
		blitTextSurf(surf, x, y, alignW, txt, just);
		SDL_FreeSurface(txt);
	}

	void drawText(SDL_Surface *surf, const char *str,
	              int x, int y, int alignW,
	              Justification just, bool bold = false)
	{
		SDL_Color c = { cText, cText, cText, 255 };
		drawText(surf, str, x, y, alignW, just, c, bold);
	}

	void setupBindingData(const BDescVec &d)
	{
		size_t slotI[vButtonsN] = { 0 };

		for (size_t i = 0; i < bWidgets.size(); ++i)
			for (size_t j = 0; j < 4; ++j)
				bWidgets[i].src[j].type = Invalid;

		for (size_t i = 0; i < d.size(); ++i)
		{
			const BindingDesc &desc = d[i];
			const Input::ButtonCode trg = desc.target;

			size_t j;
			for (j = 0; j < vButtonsN; ++j)
				if (bWidgets[j].vb.code == trg)
					break;

			assert(j < vButtonsN);

			size_t &slot = slotI[j];
			BindingWidget &w = bWidgets[j];

			if (slot == 4)
				continue;

			w.src[slot++] = desc.src;
		}
	}

	void updateDuplicateStatus()
	{
		for (size_t i = 0; i < bWidgets.size(); ++i)
			for (size_t j = 0; j < 4; ++j)
				bWidgets[i].dupFlag[j] = false;

		bool haveDup = false;

		for (size_t i = 0; i < bWidgets.size(); ++i)
		{
			for (size_t j = 0; j < 4; ++j)
			{
				const SourceDesc &src = bWidgets[i].src[j];

				if (src.type == Invalid)
					continue;

				for (size_t k = 0; k < bWidgets.size(); ++k)
				{
					if (k == i)
						continue;

					for (size_t l = 0; l < 4; ++l)
					{
						if (bWidgets[k].src[l] != src)
							continue;

						bWidgets[i].dupFlag[j] = true;
						bWidgets[k].dupFlag[l] = true;
						haveDup = true;
					}
				}
			}
		}

		dupWarnLabel.setVisible(haveDup);
	}

	void redraw()
	{
		fillSurface(winSurf, cBgNorm);

		for (size_t i = 0; i < widgets.size(); ++i)
			widgets[i]->draw(winSurf);

		if (state == AwaitingInput)
		{
			char buf[64];
			snprintf(buf, sizeof(buf), "Press key or joystick button for \"%s\"", captureName);

			drawOff = Vec2i();

			SDL_Surface *dark = createSurface(winSize.x, winSize.y);
			fillSurface(dark, 0);
			SDL_SetSurfaceAlphaMod(dark, 128);
			SDL_SetSurfaceBlendMode(dark, SDL_BLENDMODE_BLEND);
			SDL_Surface *txt = createTextSurface(buf, false);

			SDL_BlitSurface(dark, 0, winSurf, 0);

			SDL_Rect fill;
			fill.x = (winSize.x - txt->w - 20) / 2;
			fill.y = (winSize.y - txt->h - 20) / 2;
			fill.w = txt->w + 20;
			fill.h = txt->h + 20;

			fillRect(winSurf, cBgNorm, fill.x, fill.y, fill.w, fill.h);
			strokeRectInner(winSurf, cLine, fill.x, fill.y, fill.w, fill.h, 2);

			fill.x += 10;
			fill.y += 10;
			fill.w = txt->w;
			fill.h = txt->h;

			SDL_BlitSurface(txt, 0, winSurf, &fill);

			SDL_FreeSurface(txt);
			SDL_FreeSurface(dark);
		}

		SDL_UpdateWindowSurface(window);
	}

	Widget *findWidget(int x, int y)
	{
		Widget *w = 0;

		for (size_t i = 0; i < widgets.size(); ++i)
			if (widgets[i]->hit(x, y))
			{
				w = widgets[i];
				break;
			}

		return w;
	}

	void onClick(const SDL_MouseButtonEvent &e)
	{
		if (e.button != SDL_BUTTON_LEFT && e.button != SDL_BUTTON_RIGHT)
			return;

		if (state == AwaitingInput)
		{
			state = Idle;
			redraw();
			return;
		}

		Widget *w = findWidget(e.x, e.y);

		if (w)
			w->click(e.x, e.y, e.button);
	}

	void onMotion(const SDL_MouseMotionEvent &e)
	{
		if (state == AwaitingInput)
			return;

		Widget *w = findWidget(e.x, e.y);

		if (w != hovered)
		{
			if (hovered)
				hovered->leave();
			hovered = w;
		}

		if (hovered)
			hovered->motion(e.x, e.y);
	}

	bool onCaptureInputEvent(const SDL_Event &event)
	{
		assert(captureDesc);
		SourceDesc &desc = *captureDesc;

		switch (event.type)
		{
		case SDL_KEYDOWN:
			desc.type = Key;
			desc.d.scan = event.key.keysym.scancode;

			/* Special case aliases */
			if (desc.d.scan == SDL_SCANCODE_RSHIFT)
				desc.d.scan = SDL_SCANCODE_LSHIFT;

			if (desc.d.scan == SDL_SCANCODE_KP_ENTER)
				desc.d.scan = SDL_SCANCODE_RETURN;

			break;

		case SDL_JOYBUTTONDOWN:
			desc.type = JButton;
			desc.d.jb = event.jbutton.button;
			break;

		case SDL_JOYHATMOTION:
		{
			int v = event.jhat.value;

			/* Only register if single directional input */
			if (v != SDL_HAT_LEFT && v != SDL_HAT_RIGHT &&
			    v != SDL_HAT_UP   && v != SDL_HAT_DOWN)
				return true;

			desc.type = JHat;
			desc.d.jh.hat = event.jhat.hat;
			desc.d.jh.pos = v;
			break;
		}

		case SDL_JOYAXISMOTION:
		{
			int v = event.jaxis.value;

			/* Only register if pushed halfway through */
			if (v > -JAXIS_THRESHOLD && v < JAXIS_THRESHOLD)
				return true;

			desc.type = JAxis;
			desc.d.ja.axis = event.jaxis.axis;
			desc.d.ja.dir = v < 0 ? Negative : Positive;
			break;
		}
		default:
			return false;
		}

		captureDesc = 0;
		state = Idle;
		updateDuplicateStatus();
		redraw();

		return true;
	}

	void onBWidgetCellClicked(SourceDesc &desc, const char *str, uint8_t button)
	{
		if (state != Idle)
			return;

		if (button == SDL_BUTTON_LEFT)
		{
			captureDesc = &desc;
			captureName = str;
			state = AwaitingInput;
		}
		else /* e.button == SDL_BUTTON_RIGHT */
		{
			/* Clear binding */
			desc.type = Invalid;
		}

		updateDuplicateStatus();
		redraw();
	}

	void onResetToDefault()
	{
		setupBindingData(genDefaultBindings(rtData.config));
		updateDuplicateStatus();
		redraw();
	}

	void onAccept()
	{
		BDescVec binds;

		for (size_t i = 0; i < bWidgets.size(); ++i)
			bWidgets[i].appendBindings(binds);

		rtData.bindingUpdateMsg.post(binds);

		/* Store the key bindings to disk as well to prevent config loss */
		storeBindings(binds, rtData.config);

		destroyReq = true;
	}

	void onCancel()
	{
		destroyReq = true;
	}
};

Widget::Widget(SMP *p, const IntRect &rect)
    : p(p), rect(rect)
{}

bool Widget::hit(int x, int y)
{
	return pointInRect(rect, x, y);
}

void Widget::draw(SDL_Surface *surf)
{
	Vec2i prev = p->drawOff;
	p->drawOff = rect.pos();
	drawHandler(surf);
	p->drawOff = prev;
}

void Widget::motion(int x, int y)
{
	motionHandler(x - rect.x, y - rect.y);
}

void Widget::leave()
{
	leaveHandler();
}

void Widget::click(int x, int y, uint8_t button)
{
	clickHandler(x - rect.x, y - rect.y, button);
}

/* Ratio of cell area to total widget width */
#define BW_CELL_R 0.75f

void BindingWidget::drawHandler(SDL_Surface *surf)
{
	const int cellW = (rect.w*BW_CELL_R) / 2;
	const int cellH = rect.h / 2;
	const int cellOffX = (1.0f-BW_CELL_R) * rect.w;

	const int cellOff[] =
	{
	    cellOffX, 1,
	    cellOffX+cellW, 1,
	    cellOffX, cellH,
	    cellOffX+cellW, cellH
	};

	const int lbOff[] =
	{
	    0, cellH / 2,
	    cellW, cellH / 2,
	    0, cellH + cellH / 2,
	    cellW, cellH + cellH / 2
	};

	/* Hovered cell background */
	if (hoveredCell != -1)
		p->fillRect(surf, cBgDark,
		            cellOff[hoveredCell*2], cellOff[hoveredCell*2+1],
		            cellW, cellH);

	/* Frame */
	p->strokeRectInner(surf, cLine, 0, 0, rect.w, rect.h, 2);

	/* Virtual button name */
	p->drawText(surf, vb.str, 1, rect.h/2, cellOffX, Center, true);

	/* Cell frames */
	p->strokeLineV(surf, cLine, cellOffX, 0, rect.h, 2);
	p->strokeLineV(surf, cLine, cellOffX+cellW, 0, rect.h, 2);
	p->strokeLineH(surf, cLine, cellOffX, cellH, cellW*2, 2);

	/* Draw binding labels */
	for (size_t i = 0; i < 4; ++i)
	{
		std::string lb = sourceDescString(src[i]);
		if (lb.empty())
			continue;

		const int x = lbOff[i*2+0];
		const int y = lbOff[i*2+1];
		p->drawText(surf, lb.c_str(), cellOffX+x+1, y, cellW-2, Center);
	}

	for (size_t i = 0; i < 4; ++i)
	{
		if (!dupFlag[i])
			continue;

		p->strokeRectInner(surf, cellOff[i*2]+1, cellOff[i*2+1]+1,
		        cellW-2, cellH-3, 1, 255, 0, 0);
	}
}

void BindingWidget::setHoveredCell(int cell)
{
	if (hoveredCell == cell)
		return;

	hoveredCell = cell;
	p->redraw();
}

void BindingWidget::motionHandler(int x, int y)
{
	setHoveredCell(cellIndex(x, y));
}

void BindingWidget::leaveHandler()
{
	setHoveredCell(-1);
}

void BindingWidget::clickHandler(int x, int y, uint8_t button)
{
	int cell = cellIndex(x, y);

	if (cell == -1)
		return;

	p->onBWidgetCellClicked(src[cell], vb.str, button);
}

int BindingWidget::cellIndex(int x, int y) const
{
	const int cellW = (rect.w*BW_CELL_R) / 2;
	const int cellH = rect.h / 2;
	const int cellOff = (1.0f-BW_CELL_R) * rect.w;

	if (x < cellOff)
		return -1;

	x -= cellOff;

	if (y < cellH)
		if (x < cellW)
			return 0;
		else
			return 1;
	else
		if (x < cellW)
			return 2;
		else
			return 3;

	return -1;
}

void BindingWidget::appendBindings(BDescVec &d) const
{
	for (size_t i = 0; i < 4; ++i)
	{
		if (src[i].type == Invalid)
			continue;

		BindingDesc desc;
		desc.src = src[i];
		desc.target = vb.code;
		d.push_back(desc);
	}
}

void Button::setHovered(bool val)
{
	if (hovered == val)
		return;

	hovered = val;
	p->redraw();
}

void Button::drawHandler(SDL_Surface *surf)
{
	if (hovered)
		p->fillRect(surf, cBgDark, 0, 0, rect.w, rect.h);

	p->strokeRectInner(surf, cLine, 0, 0, rect.w, rect.h, 2);
	p->drawText(surf, str, 0, rect.h/2, rect.w, Center);
}

void Button::motionHandler(int, int)
{
	setHovered(true);
}

void Button::leaveHandler()
{
	setHovered(false);
}

void Button::clickHandler(int, int, uint8_t button)
{
	if (button != SDL_BUTTON_LEFT)
		return;

	(p->*cb)();
}

void Label::setVisible(bool val)
{
	if (visible == val)
		return;

	visible = val;
	p->redraw();
}

void Label::drawHandler(SDL_Surface *surf)
{
	if (visible)
		p->drawText(surf, str, 0, rect.h/2, rect.w, Left, c);
}

SettingsMenu::SettingsMenu(RGSSThreadData &rtData)
{
	p = new SettingsMenuPrivate(rtData);
	p->state = Idle;

	p->hasFocus = false;
	p->destroyReq = false;

	p->window = SDL_CreateWindow("Key bindings",
	                             SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
	                             winSize.x, winSize.y, SDL_WINDOW_INPUT_FOCUS);
	p->winSurf = SDL_GetWindowSurface(p->window);
	p->winID = SDL_GetWindowID(p->window);

	p->font = SharedFontState::openBundled(fontSize);

	p->rgb = p->winSurf->format;

	const size_t layoutW = 4;
	const size_t layoutH = 3;
	assert(layoutW*layoutH == vButtonsN);

	const int bWidgetW = winSize.x / layoutH;
	const int bWidgetH = 64;
	const int bWidgetY = winSize.y - layoutW*bWidgetH - 48;

	for (int y = 0; y < 4; ++y)
		for (int x = 0; x < 3; ++x)
		{
			int i = y*3+x;
			BindingWidget w(i, p, IntRect(x*bWidgetW, bWidgetY+y*bWidgetH,
			                              bWidgetW, bWidgetH));
			p->bWidgets.push_back(w);
		}

	for (size_t i = 0; i< p->bWidgets.size(); ++i)
		p->widgets.push_back(&p->bWidgets[i]);

	BDescVec binds;
	rtData.bindingUpdateMsg.get(binds);
	p->setupBindingData(binds);

	/* Buttons */
	const int buttonH = 32;
	const int buttonY = winSize.y - buttonH - 8;

	IntRect btRects[] =
	{
	    IntRect(16, buttonY, 112, buttonH),
	    IntRect(winSize.x-16-64*2-8, buttonY, 64, buttonH),
	    IntRect(winSize.x-16-64, buttonY, 64, buttonH)
	};

	p->buttons.push_back(Button(p, btRects[0], "Reset defaults", &SMP::onResetToDefault));
	p->buttons.push_back(Button(p, btRects[1], "Cancel", &SMP::onCancel));
	p->buttons.push_back(Button(p, btRects[2], "Accept", &SMP::onAccept));

	for (size_t i = 0; i< p->buttons.size(); ++i)
		p->widgets.push_back(&p->buttons[i]);

	/* Labels */
	const char *info = "Use left click to bind a slot, right click to clear its binding";
	p->infoLabel = Label(p, IntRect(16, 6, winSize.x, 16), info, cText, cText, cText);

	const char *warn = "Warning: Same physical key bound to multiple slots";
	p->dupWarnLabel = Label(p, IntRect(16, 26, winSize.x, 16), warn, 255, 0, 0);

	p->widgets.push_back(&p->infoLabel);
	p->widgets.push_back(&p->dupWarnLabel);

	p->hovered = 0;
	p->captureDesc = 0;
	p->captureName = 0;

	p->updateDuplicateStatus();

	p->redraw();
}

SettingsMenu::~SettingsMenu()
{
	TTF_CloseFont(p->font);
	SDL_DestroyWindow(p->window);

	delete p;
}

bool SettingsMenu::onEvent(const SDL_Event &event)
{
	/* First, check whether this event is for us */
	switch (event.type)
	{
	case SDL_WINDOWEVENT :
	case SDL_MOUSEBUTTONDOWN :
	case SDL_MOUSEBUTTONUP :
	case SDL_MOUSEMOTION :
	case SDL_KEYDOWN :
	case SDL_KEYUP :
		/* We can do this because windowID has the same
		 * struct offset in all these event types */
		if (event.window.windowID != p->winID)
			return false;
		break;

	case SDL_JOYBUTTONDOWN :
	case SDL_JOYBUTTONUP :
	case SDL_JOYHATMOTION :
	case SDL_JOYAXISMOTION :
		if (!p->hasFocus)
			return false;
		break;

	/* Don't try to handle something we don't understand */
	default:
		return false;
	}

	/* Now process it.. */
	switch (event.type)
	{
	/* Ignore these event */
	case SDL_MOUSEBUTTONUP :
	case SDL_KEYUP :
		return true;

	case SDL_WINDOWEVENT :
		switch (event.window.event)
		{
		case SDL_WINDOWEVENT_SHOWN : // SDL is bugged and doesn't give us a first FOCUS_GAINED event
		case SDL_WINDOWEVENT_FOCUS_GAINED :
			p->hasFocus = true;
			break;

		case SDL_WINDOWEVENT_FOCUS_LOST :
			p->hasFocus = false;
			break;

		case SDL_WINDOWEVENT_EXPOSED :
			SDL_UpdateWindowSurface(p->window);
			break;

		case SDL_WINDOWEVENT_LEAVE:
			if (p->hovered)
			{
				p->hovered->leave();
				p->hovered = 0;
			}
			break;

		case SDL_WINDOWEVENT_CLOSE:
			p->onCancel();
		}

		return true;

	case SDL_MOUSEMOTION:
		p->onMotion(event.motion);
		return true;

	case SDL_KEYDOWN:
		if (p->state != AwaitingInput)
		{
			if (event.key.keysym.sym == SDLK_RETURN)
				p->onAccept();
			else if (event.key.keysym.sym == SDLK_ESCAPE)
				p->onCancel();

			return true;
		}

		/* Don't let the user bind keys that trigger
		 * mkxp functions */
		switch(event.key.keysym.scancode)
		{
		case SDL_SCANCODE_F1:
		case SDL_SCANCODE_F2:
		case SDL_SCANCODE_F12:
			return true;
		default:
			break;
		}

	case SDL_JOYBUTTONDOWN:
	case SDL_JOYHATMOTION:
	case SDL_JOYAXISMOTION:
		if (p->state != AwaitingInput)
			return true;
		break;

	case SDL_MOUSEBUTTONDOWN:
		p->onClick(event.button);
		return true;
	default:
		return true;
	}

	if (p->state == AwaitingInput)
		return p->onCaptureInputEvent(event);

	return true;
}

void SettingsMenu::raise()
{
	SDL_RaiseWindow(p->window);
}

bool SettingsMenu::destroyReq() const
{
	return p->destroyReq;
}
