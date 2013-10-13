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
class Window;
class ScanRow;

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
	friend class ScanRow;
};

class SceneElement
{
public:
	SceneElement(Scene &scene, int z = 0);
	SceneElement(Scene &scene, int z, unsigned int cStamp);
	virtual ~SceneElement();

	void setScene(Scene &scene);

	DECL_ATTR_VIRT( Z,       int  )
	DECL_ATTR_VIRT( Visible, bool )

	/* Disposable classes reimplement this to
	 * check if they're disposed before access */
	virtual void aboutToAccess() const {}

protected:
	virtual void draw() = 0;
	virtual void onGeometryChange(const Scene::Geometry &) {}

	/* Compares to elements in terms of their display priority;
	 * elements with lower priority are drawn earlier */
	bool operator<(const SceneElement &o) const;

	void unlink();

	IntruListLink<SceneElement> link;
	const unsigned int creationStamp;
	int z;
	bool visible;
	Scene *scene;

	friend class Scene;
	friend class Viewport;
};

#endif // SCENE_H
