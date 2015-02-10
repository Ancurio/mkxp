/*
** scene.cpp
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

#include "scene.h"
#include "sharedstate.h"

Scene::Scene()
{}

Scene::~Scene()
{
	/* Ensure elements don't unlink from a destructed Scene */
	IntruListLink<SceneElement> *iter;

	for (iter = elements.begin(); iter != elements.end(); iter = iter->next)
	{
		iter->data->scene = 0;
	}
}

void Scene::insert(SceneElement &element)
{
	IntruListLink<SceneElement> *iter;

	for (iter = elements.begin(); iter != elements.end(); iter = iter->next)
	{
		SceneElement *e = iter->data;

		if (element < *e)
		{
			elements.insertBefore(element.link, *iter);
			return;
		}
	}

	elements.append(element.link);
}

void Scene::insertAfter(SceneElement &element, SceneElement &after)
{
	IntruListLink<SceneElement> *iter;

	for (iter = &after.link; iter != elements.end(); iter = iter->next)
	{
		SceneElement *e = iter->data;

		if (element < *e)
		{
			elements.insertBefore(element.link, *iter);
			return;
		}
	}

	elements.append(element.link);
}

void Scene::reinsert(SceneElement &element)
{
	elements.remove(element.link);
	insert(element);
}

void Scene::notifyGeometryChange()
{
	IntruListLink<SceneElement> *iter;

	for (iter = elements.begin(); iter != elements.end(); iter = iter->next)
	{
		iter->data->onGeometryChange(geometry);
	}
}

void Scene::composite()
{
	IntruListLink<SceneElement> *iter;

	for (iter = elements.begin(); iter != elements.end(); iter = iter->next)
	{
		SceneElement *e = iter->data;

		if (e->visible)
			e->draw();
	}
}


SceneElement::SceneElement(Scene &scene, int z, int spriteY)
    : link(this),
      creationStamp(shState->genTimeStamp()),
      z(z),
      visible(true),
      scene(&scene),
      spriteY(spriteY)
{
	scene.insert(*this);
}

SceneElement::~SceneElement()
{
	unlink();
}

void SceneElement::setScene(Scene &scene)
{
	unlink();

	this->scene = &scene;

	scene.insert(*this);

	onGeometryChange(scene.getGeometry());
}

int SceneElement::getZ() const
{
	aboutToAccess();

	return z;
}

void SceneElement::setZ(int value)
{
	aboutToAccess();

	if (z == value)
		return;

	z = value;
	scene->reinsert(*this);
}

bool SceneElement::getVisible() const
{
	aboutToAccess();

	return visible;
}

void SceneElement::setVisible(bool value)
{
	aboutToAccess();

	visible = value;
}

bool SceneElement::operator<(const SceneElement &o) const
{
	/* Element draw order is decided by their Z value.
	 * If two Z values are equal, the later created object
	 * has priority */

	if (z <= o.z)
	{
		if (z == o.z)
		{
			if (rgssVer >= 2)
			{
				/* RGSS2: If two sprites' Z values collide,
				 * their Y coordinates decide draw order. Only
				 * on equal Y does the creation time take effect */
				if (spriteY != o.spriteY)
					return (spriteY < o.spriteY);
			}

			return (creationStamp < o.creationStamp);
		}

		return true;
	}

	return false;
}

void SceneElement::setSpriteY(int value)
{
	spriteY = value;
	scene->reinsert(*this);
}

void SceneElement::unlink()
{
	if (scene)
		scene->elements.remove(link);
}
