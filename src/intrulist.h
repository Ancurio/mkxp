/*
** intrulist.h
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

#ifndef INTRULIST_H
#define INTRULIST_H

template <typename T>
struct IntruListLink
{
	IntruListLink<T> *prev;
	IntruListLink<T> *next;
	T *data;

	IntruListLink(T *data)
	    : prev(0),
	      next(0),
	      data(data)
	{}

	~IntruListLink()
	{
		if (prev && next)
		{
			next->prev = prev;
			prev->next = next;
		}
	}
};

template <typename T>
class IntruList
{
	IntruListLink<T> root;
	int size;

public:
	IntruList()
	    : root(0),
	      size(0)
	{
		root.prev = &root;
		root.next = &root;
	}

	void prepend(IntruListLink<T> &node)
	{
		root.next->prev = &node;
		node.prev = &root;
		node.next = root.next;
		root.next = &node;

		size++;
	}

	void append(IntruListLink<T> &node)
	{
		root.prev->next = &node;
		node.next = &root;
		node.prev = root.prev;
		root.prev = &node;

		size++;
	}

	void insertBefore(IntruListLink<T> &node,
	                  IntruListLink<T> &prev)
	{
		node.next = &prev;
		node.prev = prev.prev;
		prev.prev->next = &node;
		prev.prev = &node;

		size++;
	}

	void remove(IntruListLink<T> &node)
	{
		if (!node.next)
			return;

		node.prev->next = node.next;
		node.next->prev = node.prev;

		node.prev = 0;
		node.next = 0;

		size--;
	}

	void clear()
	{
		remove(root);
		root.prev = &root;
		root.next = &root;

		size = 0;
	}

	T *tail() const
	{
		IntruListLink<T> *node = root.prev;
		if (node == &root)
			return 0;

		return node->data;
	}

	IntruListLink<T> *begin()
	{
		return root.next;
	}

	IntruListLink<T> *end()
	{
		return &root;
	}

	bool isEmpty() const
	{
		return root.next == &root;
	}

	int getSize() const
	{
		return size;
	}
};

#endif // INTRULIST_H
