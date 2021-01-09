/*
** boost-hash.h
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

#ifndef BOOSTHASH_H
#define BOOSTHASH_H

#include <boost/unordered/unordered_map.hpp>
#include <boost/unordered/unordered_set.hpp>

#include <utility>

/* Wrappers around the boost unordered template classes,
 * exposing an interface similar to Qt's QHash/QSet */

template<typename K, typename V>
class BoostHash
{
private:
	typedef boost::unordered_map<K, V> BoostType;
	typedef std::pair<K, V> PairType;
	BoostType p;

public:
	typedef typename BoostType::const_iterator const_iterator;

	inline bool contains(const K &key) const
	{
		const_iterator iter = p.find(key);

		return (iter != p.cend());
	}

	inline void insert(const K &key, const V &value)
	{
		p.insert(PairType(key, value));
	}

	inline void remove(const K &key)
	{
		p.erase(key);
	}

	inline const V value(const K &key) const
	{
		const_iterator iter = p.find(key);

		if (iter == p.cend())
			return V();

		return iter->second;
	}

	inline const V value(const K &key, const V &defaultValue) const
	{
		const_iterator iter = p.find(key);

		if (iter == p.cend())
			return defaultValue;

		return iter->second;
	}

	inline V &operator[](const K &key)
	{
		return p[key];
	}

	inline const_iterator cbegin() const
	{
		return p.cbegin();
	}

	inline const_iterator cend() const
	{
		return p.cend();
	}
    
    inline void clear()
    {
        p.clear();
    }
};

template<typename K>
class BoostSet
{
private:
	typedef boost::unordered_set<K> BoostType;
	BoostType p;

public:
	typedef typename BoostType::const_iterator const_iterator;

	inline bool contains(const K &key)
	{
		const_iterator iter = p.find(key);

		return (iter != p.cend());
	}

	inline void insert(const K &key)
	{
		p.insert(key);
	}

	inline void remove(const K &key)
	{
		p.erase(key);
	}

	inline const_iterator cbegin() const
	{
		return p.cbegin();
	}

	inline const_iterator cend() const
	{
		return p.cend();
	}
};

#endif // BOOSTHASH_H
