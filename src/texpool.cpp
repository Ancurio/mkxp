/*
** texpool.cpp
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

#include "texpool.h"
#include "exception.h"
#include "sharedstate.h"
#include "glstate.h"
#include "boost-hash.h"
#include "debugwriter.h"

#include <list>
#include <utility>
#include <assert.h>
#include <string.h>

typedef std::pair<uint16_t, uint16_t> Size;

static uint32_t byteCount(Size &s)
{
	return s.first * s.second * 4;
}

struct CacheNode
{
	TEXFBO obj;
	std::list<TEXFBO>::iterator prioIter;

	bool operator==(const CacheNode &o) const
	{
		return obj == o.obj;
	}
};

typedef std::list<CacheNode> CNodeList;

struct TexPoolPrivate
{
	/* Contains all cached TexFBOs, grouped by size */
	BoostHash<Size, CNodeList> poolHash;

	/* Contains all cached TexFBOs, sorted by release time */
	std::list<TEXFBO> priorityQueue;

	/* Maximal allowed cache memory */
	const uint32_t maxMemSize;

	/* Current amound of memory consumed by the cache */
	uint32_t memSize;

	/* Current amount of TexFBOs cached */
	uint16_t objCount;

	/* Has this pool been disabled? */
	bool disabled;

	TexPoolPrivate(uint32_t maxMemSize)
	    : maxMemSize(maxMemSize),
	      memSize(0),
	      objCount(0),
	      disabled(false)
	{}
};

TexPool::TexPool(uint32_t maxMemSize)
{
	p = new TexPoolPrivate(maxMemSize);
}

TexPool::~TexPool()
{
	std::list<TEXFBO>::iterator iter;

	for (iter = p->priorityQueue.begin();
	     iter != p->priorityQueue.end();
	     ++iter)
	{
		TEXFBO obj = *iter;
		TEXFBO::fini(obj);
		--p->objCount;
	}

	assert(p->objCount == 0);

	delete p;
}

TEXFBO TexPool::request(int width, int height)
{
	CacheNode cnode;
	Size size(width, height);

	/* See if we can statisfy request from cache */
	CNodeList &bucket = p->poolHash[size];

	if (!bucket.empty())
	{
		/* Found one! */
		cnode = bucket.back();
		bucket.pop_back();

		p->priorityQueue.erase(cnode.prioIter);

		p->memSize -= byteCount(size);
		--p->objCount;

//		Debug() << "TexPool: <?+> (" << width << height << ")";

		return cnode.obj;
	}

	int maxSize = glState.caps.maxTexSize;
	if (width > maxSize || height > maxSize)
		throw Exception(Exception::MKXPError,
		                "Texture dimensions [%d, %d] exceed hardware capabilities",
		                width, height);

	/* Nope, create it instead */
	TEXFBO::init(cnode.obj);
	TEXFBO::allocEmpty(cnode.obj, width, height);
	TEXFBO::linkFBO(cnode.obj);

//	Debug() << "TexPool: <?-> (" << width << height << ")";

	return cnode.obj;
}

void TexPool::release(TEXFBO &obj)
{
	if (obj.tex == TEX::ID(0) || obj.fbo == FBO::ID(0))
	{
		TEXFBO::fini(obj);
		return;
	}

	if (p->disabled)
	{
		/* If we're disabled, delete without caching */
//		Debug() << "TexPool: <!#> (" << obj.width << obj.height << ")";
		TEXFBO::fini(obj);
		return;
	}

	Size size(obj.width, obj.height);

	uint32_t newMemSize = p->memSize + byteCount(size);

	/* If caching this object would spill over the allowed memory budget,
	 * delete least used objects until we're good again */
	while (newMemSize > p->maxMemSize)
	{
		if (p->objCount == 0)
			break;

//		Debug() << "TexPool: <!~> Size:" << p->memSize;

		/* Retrieve object with lowest priority for deletion */
		CacheNode last;
		last.obj = p->priorityQueue.back();
		Size removedSize(last.obj.width, last.obj.height);

		CNodeList &bucket = p->poolHash[removedSize];

		std::list<CacheNode>::iterator toRemove =
		        std::find(bucket.begin(), bucket.end(), last);
		assert(toRemove != bucket.end());
		bucket.erase(toRemove);

		p->priorityQueue.pop_back();

		TEXFBO::fini(last.obj);

		newMemSize -= byteCount(removedSize);
		--p->objCount;

//		Debug() << "TexPool: <!-> (" << last.obj.width << last.obj.height << ")";
	}

	p->memSize = newMemSize;

	/* Retain object */
	p->priorityQueue.push_front(obj);
	CacheNode cnode;
	cnode.obj = obj;
	cnode.prioIter = p->priorityQueue.begin();
	CNodeList &bucket = p->poolHash[size];
	bucket.push_back(cnode);

	++p->objCount;

//	Debug() << "TexPool: <!+> (" << obj.width << obj.height << ") Current size:" << p->memSize;
}

void TexPool::disable()
{
	p->disabled = true;
}


