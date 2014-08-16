/*
** scene.h
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

#ifndef SCENE_H
#define SCENE_H

#include "util.h"
#include "intrulist.h"
#include "etc.h"
#include "etc-internal.h"

class SceneElement;
class Viewport;
class WindowVX;
class Window;
struct ScanRow;
struct TilemapPrivate;

class Scene
{
public:
	struct Geometry
	{
		int xOrigin, yOrigin;
		IntRect rect;
	};

	Scene();
	virtual ~Scene();

	virtual void composite();
	virtual void requestViewportRender(Vec4& /*color*/, Vec4& /*flash*/, Vec4& /*tone*/) {}

	const Geometry &getGeometry() const { return geometry; }

protected:
	void insert(SceneElement &element);
	void insertAfter(SceneElement &element, SceneElement &after);
	void reinsert(SceneElement &element);

	/* Notify all elements that geometry has changed */
	void notifyGeometryChange();

	IntruList<SceneElement> elements;
	Geometry geometry;

	friend class SceneElement;
	friend class Window;
	friend class WindowVX;
	friend struct ScanRow;
};

class SceneElement
{
public:
	SceneElement(Scene &scene, int z = 0, bool isSprite = false);
	SceneElement(Scene &scene, int z, unsigned int cStamp);
	virtual ~SceneElement();

	void setScene(Scene &scene);

	DECL_ATTR_VIRT( Z,       int  )
	DECL_ATTR_VIRT( Visible, bool )

protected:
	/* A bit about OpenGL state:
	 *
	 *   If we're not inside the draw cycle (ie. the 'draw()'
	 * handle), you're free to change any GL state through
	 * gl-util, except for those in GLState which you should
	 * push/pop as needed.
	 *
	 *   If we're _drawing_, you should probably not touch most
	 * things in GLState. For scissored rendering, use push with
	 * setIntersect(), and then pop afterwards.
	 * Blendmode you can push/pop as you like. Do NOT touch viewport.
	 * Texture/Shader bindings you're free to modify without
	 * cleanup (and therefore you should expect dirty state).
	 * Do NOT touch the FBO::Draw binding. If you have to do work
	 * immediately before drawing that touches this (such as flushing
	 * Bitmaps), use the 'prepareDraw' signal in SharedState that
	 * will fire immediately before each frame draw.
	 */
	virtual void draw() = 0;

	// FIXME: This should be a signal
	virtual void onGeometryChange(const Scene::Geometry &) {}

	/* Compares two elements in terms of their display priority;
	 * elements with lower priority are drawn earlier */
	bool operator<(const SceneElement &o) const;

	void setSpriteY(int value);
	void unlink();

	IntruListLink<SceneElement> link;
	const unsigned int creationStamp;
	int z;
	bool visible;
	Scene *scene;

	friend class Scene;
	friend class Viewport;
	friend struct TilemapPrivate;

private:
	int spriteY;
	const bool isSprite;
};

#endif // SCENE_H
