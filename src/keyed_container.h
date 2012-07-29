/************************************************************************/
/*                                                                      */
/* This file is part of VDrift.                                         */
/*                                                                      */
/* VDrift is free software: you can redistribute it and/or modify       */
/* it under the terms of the GNU General Public License as published by */
/* the Free Software Foundation, either version 3 of the License, or    */
/* (at your option) any later version.                                  */
/*                                                                      */
/* VDrift is distributed in the hope that it will be useful,            */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of       */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the        */
/* GNU General Public License for more details.                         */
/*                                                                      */
/* You should have received a copy of the GNU General Public License    */
/* along with VDrift.  If not, see <http://www.gnu.org/licenses/>.      */
/*                                                                      */
/************************************************************************/

#ifndef KEYED_CONTAINER_H
#define KEYED_CONTAINER_H

#include "joeserialize.h"
#include "macros.h"

#include <vector>
#include <cstring>
#include <cassert>
#include <map>
#include <set>
#include <deque>
#include <algorithm>

#define TRACK_CONTAINERS

class keyed_container_handle
{
template <typename> friend class keyed_container;
friend class joeserialize::Serializer;
friend struct keyed_container_hash;
private:
	typedef int INDEX;
	typedef int DATAVERSION;
	INDEX index;
	DATAVERSION version;
	#ifdef TRACK_CONTAINERS
	int containerid;
	#endif
	keyed_container_handle(INDEX newidx, DATAVERSION newver) : index(newidx),version(newver) {}

public:
	keyed_container_handle() : index(-1),version(-1)
	#ifdef TRACK_CONTAINERS
	,containerid(-1)
	#endif
	{}
	bool operator==(const keyed_container_handle & other) const {return (index == other.index) && (version == other.version);}
	bool operator!=(const keyed_container_handle & other) const {return !(operator==(other));}
	bool Serialize(joeserialize::Serializer & s)
	{
		_SERIALIZE_(s,index);
		_SERIALIZE_(s,version);
		#ifdef TRACK_CONTAINERS
		_SERIALIZE_(s,containerid);
		#endif
		return true;
	}
	bool operator<(const keyed_container_handle & other) const
	{
		long long mykey = index;
		mykey = mykey << 32;
		mykey = mykey | version;

		long long otherkey = other.index;
		otherkey = otherkey << 32;
		otherkey = otherkey | other.version;

		return (mykey < otherkey);
	}

	friend std::ostream & operator<< (std::ostream & os, const keyed_container_handle & other)
	{
		#ifdef TRACK_CONTAINERS
		os << other.containerid << " ";
		#endif
		os << other.index << " " << other.version;
		return os;
	}

	friend std::istream & operator>> (std::istream & is, keyed_container_handle & other)
	{
		#ifdef TRACK_CONTAINERS
		is >> other.containerid;
		#endif
		is >> other.index >> other.version;
		return is;
	}

	bool valid() const
	{
		return (index >= 0) && (version >= 0);
	}

	void invalidate()
	{
		index = -1;
		version = -1;
		#ifdef TRACK_CONTAINERS
		containerid = -1;
		#endif
	}
};
struct keyed_container_hash
{
	unsigned int operator()(const keyed_container_handle & h) const
	{
		long long key = h.index;
		key = key << 32;
		key = key | h.version;

		key = (~key) + (key << 18);
		key = key ^ (key >> 31);
		key = key * 21;
		key = key ^ (key >> 11);
		key = key + (key << 6);
		key = key ^ (key >> 22);
		return (int) key;
	}
};

/// This implementation uses a vector pool with allocated handles that have an extra layer of abstraction.
/// This is designed for fast lookup, extremely fast iteration, and low memory fragmentation, at the expense
/// of insert and erase speed. However, in general all operations are O(1), which is pretty darn good.
template <typename DATATYPE>
class keyed_container
{
friend class joeserialize::Serializer;
private:
	typedef int INDEX;
	typedef int DATAVERSION;
	struct HANDLEDATA
	{
		INDEX index;
		DATAVERSION version;
		HANDLEDATA() : index(-1), version(-1) {}
		HANDLEDATA(INDEX newidx, DATAVERSION newver) : index(newidx),version(newver) {}
		bool Serialize(joeserialize::Serializer & s)
		{
			_SERIALIZE_(s,index);
			_SERIALIZE_(s,version);
			return true;
		}
	};
public:
	typedef keyed_container_handle handle;
	typedef keyed_container_hash hash;

	typedef std::vector <INDEX> rmap_type;
	typedef std::vector <DATATYPE> container_type;

	typedef typename container_type::iterator iterator;
	typedef typename container_type::const_iterator const_iterator;

private:
	container_type pool;
	std::vector <HANDLEDATA> handlemap;
	rmap_type reverse_handlemap; ///< maps pool indices to handlemap indices
	std::vector <INDEX> freehandles;
	#ifdef TRACK_CONTAINERS
	int containerid;
	#endif

	///asserts that the item is found
	const DATATYPE & get_const(const handle & key) const
	{
		assert(key.index >= 0);
		assert(key.version >= 0);
		assert(!pool.empty());
		assert(key.index < (int)handlemap.size());
		#ifdef TRACK_CONTAINERS
		assert(key.containerid == containerid);
		#endif
		const HANDLEDATA & hdata = handlemap[key.index];
		assert(key.version == hdata.version);
		assert(hdata.index < (int)pool.size());
		assert(hdata.index >= 0);
		return pool[hdata.index];
	}

public:
	#ifdef TRACK_CONTAINERS
	keyed_container() : containerid((size_t)this) {}
	#endif

	bool Serialize(joeserialize::Serializer & s)
	{
		_SERIALIZE_(s,pool);
		_SERIALIZE_(s,handlemap);
		_SERIALIZE_(s,reverse_handlemap);
		_SERIALIZE_(s,freehandles);
		#ifdef TRACK_CONTAINERS
		_SERIALIZE_(s,containerid);
		#endif
		return true;
	}

	///returns the handle of the inserted item
	handle insert(const DATATYPE & newitem)
	{
		//insert the item into our data pool
		pool.push_back(newitem);
		INDEX newidx = (int)pool.size()-1;

		//generate a handle
		INDEX hidx(-1);
		if (freehandles.empty())
		{
			//allocate a new handle
			HANDLEDATA hdata(newidx, 0);
			handlemap.push_back(hdata);
			hidx = (int)handlemap.size()-1;
		}
		else
		{
			//reuse an old handle
			hidx = freehandles.back();
			assert(hidx < (int)handlemap.size());
			freehandles.pop_back();
			handlemap[hidx].version++;
			handlemap[hidx].index = newidx;
		}

		//store reverse handlemap
		reverse_handlemap.push_back(hidx);
		assert(pool.size() == reverse_handlemap.size());

		#ifdef TRACK_CONTAINERS
		handle htemp = handle(hidx,handlemap[hidx].version);
		htemp.containerid = containerid;
		return htemp;
		#else
		return handle(hidx,handlemap[hidx].version);
		#endif
	}

	const DATATYPE & get(const handle & key) const
	{
		return get_const(key);
	}

	DATATYPE & get(const handle & key)
	{
		return const_cast <DATATYPE&> (get_const(key));
	}

	///asserts that the item was found and erased
	void erase(const handle & key)
	{
		//assert that our handle is valid
		assert(key.index >= 0);
		assert(key.version >= 0);
		assert(key.index < (int)handlemap.size());
		assert(key.version == handlemap[key.index].version);
		assert(handlemap[key.index].index < (int)pool.size());
		assert(!pool.empty());
		#ifdef TRACK_CONTAINERS
		assert(key.containerid == containerid);
		#endif

		//swap the data pool element to be erased to the end
		INDEX last = (int)pool.size()-1;
		INDEX moved = handlemap[key.index].index;
		if (moved != last)
		{
			//do the swap
			std::swap(pool[last],pool[moved]);

			//update the handlemap
			INDEX hlast = reverse_handlemap[last];
			INDEX hmoved = reverse_handlemap[moved];
			assert(key.index == hmoved);
			std::swap(handlemap[hlast].index, handlemap[hmoved].index);

			//update the reverse handlemap
			std::swap(reverse_handlemap[last],reverse_handlemap[moved]);
		}

		//erase the last element from the pool
		pool.pop_back();
		reverse_handlemap.pop_back();

		//free up the handlemap entry
		handlemap[key.index].version++; //invalidate existing handles
		freehandles.push_back(key.index); //mark it free
	}

	iterator begin() {return pool.begin();}
	const_iterator begin() const {return pool.begin();}

	iterator end() {return pool.end();}
	const_iterator end() const {return pool.end();}

	unsigned int size() const
	{
		assert(handlemap.size()>=freehandles.size());
		assert(pool.size() == handlemap.size()-freehandles.size());
		assert(pool.size() == reverse_handlemap.size());
		return pool.size();
	}

	bool empty() const
	{
		assert(pool.empty() == (size() == 0));
		return pool.empty();
	}

	/// perhaps unexpectedly, this is O(n)
	void clear()
	{
		if (empty())
			return;

		for (unsigned int i = 0; i < pool.size(); i++)
		{
			INDEX hidx = reverse_handlemap[i];
			freehandles.push_back(hidx);
			handlemap[hidx].version++;
		}
		pool.clear();
		reverse_handlemap.clear();
	}

	bool contains(const handle & key) const
	{
		#ifdef TRACK_CONTAINERS
		assert(key.containerid == containerid);
		#endif
		if (key.index >= 0 && key.version >= 0 && !pool.empty() && key.index < (int)handlemap.size() &&
			key.version == handlemap[key.index].version && handlemap[key.index].index < (int)pool.size() &&
			handlemap[key.index].index >= 0)
		{
			return true;
		}
		else
			return false;
	}

	iterator find(const handle & key)
	{
		#ifdef TRACK_CONTAINERS
		assert(key.containerid == containerid);
		#endif
		if (key.index >= 0 && key.version >= 0 && !pool.empty() && key.index < (int)handlemap.size() &&
			key.version == handlemap[key.index].version && handlemap[key.index].index < (int)pool.size() &&
			handlemap[key.index].index >= 0)
		{
			return begin()+handlemap[key.index].index;
		}
		else
			return end();
	}
	const_iterator find(const handle & key) const
	{
		#ifdef TRACK_CONTAINERS
		assert(key.containerid == containerid);
		#endif
		if (key.index >= 0 && key.version >= 0 && !pool.empty() && key.index < (int)handlemap.size() &&
			key.version == handlemap[key.index].version && handlemap[key.index].index < (int)pool.size() &&
			handlemap[key.index].index >= 0)
		{
			return begin()+handlemap[key.index].index;
		}
		else
			return end();
	}
};

#endif
