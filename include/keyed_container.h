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

//#define USEBOOST_FOR_KEYED_CONTAINER
//#define USE_COUNTING_INT_FOR_KEYED_CONTAINER


#ifndef USEBOOST_FOR_KEYED_CONTAINER

#ifdef USE_COUNTING_INT_FOR_KEYED_CONTAINER

class keyed_container_hash
{
	public:
	unsigned int operator()(unsigned int a) const
	{
		a -= (a<<6);
		a ^= (a>>17);
		a -= (a<<9);
		a ^= (a<<4);
		a -= (a<<3);
		a ^= (a<<10);
		a ^= (a>>15);
		return a;
	}
};

///base class for a unique number generator
template <typename KEYTYPE>
class unique_generator
{
	public:
		virtual KEYTYPE allocate() = 0;
		virtual void release(KEYTYPE key) = 0;
};

///simple, very fast counting unique number generator; requires operator++ and operator< from the key type
template <typename KEYTYPE>
class fast_counting_unique_generator : public unique_generator <KEYTYPE>
{
	friend class joeserialize::Serializer;
	private:
		KEYTYPE count;
		
	public:
		fast_counting_unique_generator() : count(0) {}
		
		virtual KEYTYPE allocate()
		{
			KEYTYPE out = count;
			count++;
			assert(out < count); //catch integer wrap
			return out;
		}
		virtual void release(KEYTYPE key)
		{
			//noop
		}
		void reset(int resetat = 0)
		{
			operator=(fast_counting_unique_generator());
			count = resetat;
		}
		bool Serialize(joeserialize::Serializer & s)
		{
			_SERIALIZE_(s,count);
			return true;
		}
};

///slower, safe counting unique number generator; requires operator++ and operator< from the key type
template <typename KEYTYPE>
class counting_unique_generator : public unique_generator <KEYTYPE>
{
	private:
		std::set <KEYTYPE> allocated_keys;
		std::set <KEYTYPE> free_keys;
		
	public:
		counting_unique_generator() {}
		
		virtual KEYTYPE allocate()
		{
			if (free_keys.empty())
			{
				if (allocated_keys.empty())
				{
					KEYTYPE out = 0;
					allocated_keys.insert(out);
					return out;
				}
				else
				{
					KEYTYPE old = *allocated_keys.rbegin();
					KEYTYPE out = old;
					out++;
					assert(old < out); //catch integer wrap
					allocated_keys.insert(out);
					return out;
				}
			}
			else
			{
				KEYTYPE out = *free_keys.begin();
				allocated_keys.insert(out);
				free_keys.erase(free_keys.begin());
				return out;
			}
		}
		
		virtual void release(KEYTYPE key)
		{
			free_keys.insert(key);
			allocated_keys.erase(key);
		}

		void reset()
		{
			operator=(counting_unique_generator());
		}
		
		unsigned int num_allocated_keys() const {return allocated_keys.size();}
		unsigned int num_free_keys() const {return free_keys.size();}
};

#include <tr1/unordered_map>

///an unordered non-unique container class that allows the user to take smart "handles" to objects
template <typename DATATYPE>
class keyed_container
{
friend class joeserialize::Serializer;
public:
	class UINTHANDLE
	{
		friend class joeserialize::Serializer;
		private:
			unsigned int value;
		
		public:
			UINTHANDLE() : value(0) {}
			UINTHANDLE(unsigned int newval) : value(newval) {}
			bool operator==(const UINTHANDLE & other) const {return value == other.value;}
			bool operator!=(const UINTHANDLE & other) const {return value != other.value;}
			bool operator<(const UINTHANDLE & other) const {return value < other.value;}
			UINTHANDLE & operator++() {value++;return *this;}
			UINTHANDLE operator++(int) {UINTHANDLE tmp(*this);value++;return tmp;}
			operator unsigned int() const {return value;}
			unsigned int empty_key() const {return 0;}
			unsigned int deleted_key() const {return 1;}
			bool Serialize(joeserialize::Serializer & s)
			{
				_SERIALIZE_(s,value);
				return true;
			}
	};
	typedef UINTHANDLE handle;
	typedef std::tr1::unordered_map <handle, DATATYPE, keyed_container_hash> container_type;
	typedef typename container_type::iterator iterator;
	typedef typename container_type::const_iterator const_iterator;
	
private:
	container_type data;
	fast_counting_unique_generator <handle> keygen;
	
public:
	keyed_container()
	{
		//data.set_empty_key(keygen.allocate()); //zero
		//data.set_deleted_key(keygen.allocate()); //one
		
		//reserve the first couple of keys so empty handles won't point to anything
		keygen.allocate();
		keygen.allocate();
	}
	
	bool Serialize(joeserialize::Serializer & s)
	{
		_SERIALIZE_(s,data);
		_SERIALIZE_(s,keygen);
		return true;
	}
	
	///asserts that the item was found and erased
	void erase(const handle & key)
	{
		iterator todel = data.find(key);
		assert (todel != data.end());
		data.erase(todel);
		keygen.release(key);
	}
	
	void clear() {data.clear();}
	
	///returns the handle of the inserted item
	handle insert(const DATATYPE & newitem)
	{
		std::pair<iterator, bool> result = data.insert(std::make_pair(keygen.allocate(), newitem));
		assert(result.second); //make sure we really did an insert
		return result.first->first;
	}
	
	DATATYPE & get(const handle & key)
	{
		iterator i = data.find(key);
		assert(i != data.end());
		return i->second;
	}
	
	const DATATYPE & get(const handle & key) const
	{
		const_iterator i = data.find(key);
		assert(i != data.end());
		return i->second;
	}
	
	bool contains(const handle & key) const
	{
		const_iterator i = data.find(key);
		return (i != data.end());
	}
	
	iterator find(const handle & key) {return data.find(key);}
	const_iterator find(const handle & key) const {return data.find(key);}
	
	iterator begin() {return data.begin();}
	const_iterator begin() const {return data.begin();}
	
	iterator end() {return data.end();}
	const_iterator end() const {return data.end();}
	
	unsigned int size() const {return data.size();}
	bool empty() const {return data.empty();}
};

#else // don't use boost, don't use counting int

#ifndef USE_TYPED_HANDLE
class keyed_container_handle
{
template <typename> friend class keyed_container;
friend class joeserialize::Serializer;
friend class keyed_container_hash;
private:
	typedef int INDEX;
	typedef int VERSION;
	INDEX index;
	VERSION version;
	keyed_container_handle(INDEX newidx, VERSION newver) : index(newidx),version(newver) {}
	
public:
	keyed_container_handle() : index(-1),version(-1) {}
	bool operator==(const keyed_container_handle & other) const {return (index == other.index) && (version == other.version);}
	bool operator!=(const keyed_container_handle & other) const {return !(operator==(other));}
	bool Serialize(joeserialize::Serializer & s)
	{
		_SERIALIZE_(s,index);
		_SERIALIZE_(s,version);
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
		os << other.index << " " << other.version;
		return os;
	}
	
	friend std::istream & operator>> (std::istream & is, keyed_container_handle & other)
	{
		is >> other.index >> other.version;
		return is;
	}
	
	bool valid() const
	{
		return (index >= 0) && (version >= 0);
	}
};

class keyed_container_hash
{
public:
	unsigned int operator()(const keyed_container_handle & h) const
	{
		long long key = h.index;
		key = key << 32;
		key = key | h.version;
		
		key = (~key) + (key << 18); // key = (key << 18) - key - 1;
		key = key ^ (key >> 31);
		key = key * 21; // key = (key + (key << 2)) + (key << 4);
		key = key ^ (key >> 11);
		key = key + (key << 6);
		key = key ^ (key >> 22);
		return (int) key;
	}
};
#endif //#ifndef USE_TYPED_HANDLE

#define TRACK_CONTAINERS

/// This implementation uses a vector pool with allocated handles that have an extra layer of abstraction.
/// This is designed for fast lookup, extremely fast iteration, and low memory fragmentation, at the expense
/// of insert and erase speed. However, in general all operations are O(1), which is pretty darn good.
template <typename DATATYPE>
class keyed_container
{
friend class joeserialize::Serializer;
private:
	typedef int INDEX;
	typedef int VERSION;
	struct HANDLEDATA
	{
		INDEX index;
		VERSION version;
		HANDLEDATA() : index(-1), version(-1) {}
		HANDLEDATA(INDEX newidx, VERSION newver) : index(newidx),version(newver) {}
		bool Serialize(joeserialize::Serializer & s)
		{
			_SERIALIZE_(s,index);
			_SERIALIZE_(s,version);
			return true;
		}
	};
public:
	#ifdef USE_TYPED_HANDLE
	class handle
	{
	friend class keyed_container;
	friend class joeserialize::Serializer;
	friend class keyed_container_hash;
	private:
		typedef int INDEX;
		typedef int VERSION;
		INDEX index;
		VERSION version;
		#ifdef TRACK_CONTAINERS
		int containerid;
		#endif
		handle(INDEX newidx, VERSION newver) : index(newidx),version(newver) {}
		
	public:
		handle() : index(-1),version(-1)
		#ifdef TRACK_CONTAINERS
		,containerid(-1)
		#endif
		{}
		bool operator==(const handle & other) const {return (index == other.index) && (version == other.version);}
		bool operator!=(const handle & other) const {return !(operator==(other));}
		bool Serialize(joeserialize::Serializer & s)
		{
			_SERIALIZE_(s,index);
			_SERIALIZE_(s,version);
			#ifdef TRACK_CONTAINERS
			_SERIALIZE_(s,containerid);
			#endif
			return true;
		}
		bool operator<(const handle & other) const
		{
			long long mykey = index;
			mykey = mykey << 32;
			mykey = mykey | version;
			
			long long otherkey = other.index;
			otherkey = otherkey << 32;
			otherkey = otherkey | other.version;
			
			return (mykey < otherkey);
		}
		
		friend std::ostream & operator<< (std::ostream & os, const handle & other)
		{
			os << other.index << " " << other.version;
			return os;
		}
		
		friend std::istream & operator>> (std::istream & is, handle & other)
		{
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
	class hash
	{
	public:
		unsigned int operator()(const handle & h) const
		{
			long long key = h.index;
			key = key << 32;
			key = key | h.version;
			
			key = (~key) + (key << 18); // key = (key << 18) - key - 1;
			key = key ^ (key >> 31);
			key = key * 21; // key = (key + (key << 2)) + (key << 4);
			key = key ^ (key >> 11);
			key = key + (key << 6);
			key = key ^ (key >> 22);
			return (int) key;
		}
	};
	#else //#ifdef USE_TYPED_HANDLE
	typedef keyed_container_handle handle;
	#endif
	
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
	keyed_container() : containerid((int)this) {}
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

#else //#ifndef USEBOOST

#define _CHECK_HANDLE_IS_IN_CONTAINER_

#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>

///an unordered non-unique container class that allows the user to take smart "handles" to objects
template <typename DATATYPE>
class keyed_container
{
public:
	typedef boost::weak_ptr <DATATYPE> handle;
	typedef std::set <boost::shared_ptr <DATATYPE> > container_type;
	typedef typename container_type::iterator iterator;
	typedef typename container_type::const_iterator const_iterator;
	
private:
	container_type data;
	
public:
	///asserts that the item was found and erased
	void erase(const handle & key)
	{
		boost::shared_ptr <DATATYPE> locked_key = key.lock();
		assert(locked_key);
		iterator todel = data.find(locked_key);
		assert (todel != data.end());
		data.erase(todel);
	}
	
	void clear() {data.clear();}
	
	///returns the handle of the inserted item
	handle insert(const DATATYPE & newitem)
	{
		std::pair<iterator, bool> result = data.insert(boost::shared_ptr<DATATYPE>(new DATATYPE(newitem)));
		assert(result.second); //make sure we really did an insert
		return handle(*result.first);
	}
	
	DATATYPE & get(const handle & key)
	{
#ifdef _CHECK_HANDLE_IS_IN_CONTAINER_
		assert (contains(key));
#endif
		boost::shared_ptr <DATATYPE> locked_key = key.lock();
		assert(locked_key);
		return *locked_key;
	}
	
	const DATATYPE & get(const handle & key) const
	{
#ifdef _CHECK_HANDLE_IS_IN_CONTAINER_
		assert(contains(key));
#endif
		const boost::shared_ptr <DATATYPE> locked_key = key.lock();
		assert(locked_key);
		return *locked_key;
	}
	
	bool contains(const handle & key) const
	{
		const boost::shared_ptr <DATATYPE> locked_key = key.lock();
		if (!locked_key)
			return false;
		const_iterator i = data.find(locked_key);
		return (i != data.end());
	}
	
	iterator find(const handle & key) {boost::shared_ptr <DATATYPE> locked_key = key.lock();if(!locked_key) return data.end();return data.find(locked_key);}
	const_iterator find(const handle & key) const {const boost::shared_ptr <DATATYPE> locked_key = key.lock();assert(locked_key);if(!locked_key) return data.end();return data.find(locked_key);}
	
	iterator begin() {return data.begin();}
	const_iterator begin() const {return data.begin();}
	
	iterator end() {return data.end();}
	const_iterator end() const {return data.end();}
	
	unsigned int size() const {return data.size();}
	bool empty() const {return data.empty();}
};

template <typename DATATYPE>
bool operator==(const boost::weak_ptr <DATATYPE> & p1, const boost::weak_ptr <DATATYPE> & p2)
{
	const boost::shared_ptr <DATATYPE> & s1 = p1.lock();
	const boost::shared_ptr <DATATYPE> & s2 = p2.lock();
	assert(s1);
	assert(s2);
	return s1 == s2;
}

#endif //#ifndef USEBOOST #else

/*#include "reseatable_reference.h"

template <typename DATATYPE>
class keyed_index_container
{
public:
	class handle
	{
		friend class keyed_index_container;
		private:
			unsigned int index;
			reseatable_reference <keyed_index_container> parent_container;
			
		public:
			~handle() {Invalidate();}
			
			///invalidate the handle and deregister from the container
			void Invalidate()
			{
				if (parent_container)
				{
					parent_container->Deregister(this);
					parent_container.clear();
				}
			}
			
			///set up the handle
			void Set(unsigned int newindex, keyed_index_container & newparent)
			{
				parent_container = newparent;
				index = newindex;
				parent_container->Register(this);
			}
			
			bool operator==(const handle & other) const
			{
				return (index == other.index && other.parent_container && parent_container && &*parent_container == &*other.parent_container);
			}
			bool operator!=(const handle & other) const {return !(*this == other);}
	};
	typedef std::vector<DATATYPE> container_type;
	typedef std::vector <std::vector <handle *> > register_type;
	typedef typename container_type::iterator iterator;
	typedef typename container_type::const_iterator const_iterator;
	
private:
	container_type data;
	register_type handle_registrants;
	
	///this happens before any data is moved so registrants and the registrant vector can be updated
	void NotifyMove(unsigned int fromidx, unsigned int toidx)
	{
		assert(fromidx < handle_registrants.size());
		for (typename std::vector <handle *>::iterator i = handle_registrants[fromidx].begin(); i != handle_registrants[fromidx].end(); i++)
		{
			(*i)->index = toidx;
			handle_registrants[toidx].push_back(*i);
		}
		handle_registrants[fromidx].clear();
	}
	
	///this happens before any data is deleted so registrants and the registrant vector can be updated
	void NotifyDelete(unsigned int fromidx)
	{
		assert(fromidx < handle_registrants.size());
		for (typename std::vector <handle *>::iterator i = handle_registrants[fromidx].begin(); i != handle_registrants[fromidx].end(); i++)
		{
			(*i)->Invalidate();
		}
	}
	
public:
	~keyed_index_container()
	{
		clear();
	}
	
	///add the pointer to our registrant list
	void Register(handle * h)
	{
		assert(h);
		unsigned int idx = h->index;
		assert(idx < data.size());
		handle_registrants[idx].push_back(h);
	}
	
	///remove the pointer from our registrant list
	void Deregister(handle * h)
	{
		assert(h);
		unsigned int idx = h->index;
		assert(idx < handle_registrants.size());
		bool found = false;
		unsigned int todel = 0;
		for (int i = 0; i < handle_registrants[idx].size(); i++)
		{
			if (handle_registrants[idx][i] == h)
			{
				assert(!found);
				found = true;
				todel = i;
			}
		}
		
		assert(found);
		std::swap(handle_registrants[idx][todel],handle_registrants[idx][handle_registrants[idx].size()-1]);
		handle_registrants[idx].pop_back();
	}
	
	void erase(const handle & key)
	{
		unsigned int idx = key.index;
		assert(idx < data.size());
		if (data.size() > 1)
		{
			NotifyMove(data.size()-1, idx);
			std::swap(data[idx], data[data.size()-1]);
			std::swap(handle_registrants[idx], handle_registrants[data.size()-1]);
		}
		NotifyDelete(data.size()-1);
		data.pop_back();
		handle_registrants.pop_back();
	}
	
	void clear()
	{
		for (int n = 0; n < handle_registrants.size(); n++)
		{
			for (int i = 0; i < handle_registrants[n].size(); i++)
			{
				handle_registrants[n][i]->Invalidate();
			}
		}
		
		handle_registrants.clear();
		data.clear();
	}
	
	///returns the handle of the inserted item
	handle insert(const DATATYPE & newitem)
	{
		data.push_back(newitem);
		handle_registrants.push_back(std::vector <handle *>());
		assert(data.size() == handle_registrants.size());
		handle newhandle;
		newhandle.Set(data.size()-1, *this); //does registration
		return newhandle;
	}
	
	DATATYPE & get(const handle & key)
	{
		unsigned int idx = key.index;
		assert(idx < data.size());
		assert(&key.parent_container.get() == this);
		return data[idx];
	}
	
	const DATATYPE & get(const handle & key) const
	{
		unsigned int idx = key.index;
		assert(idx < data.size());
		assert(&key.parent_container.get() == this);
		return data[idx];
	}
	
	bool contains(const handle & key) const
	{
		unsigned int idx = key.index;
		if (idx < data.size() && key.parent_container && &key.parent_container.get() == this)
			return true;
		else
			return false;
	}
	
	iterator begin() {return data.begin();}
	const_iterator begin() const {return data.begin();}
	
	iterator end() {return data.end();}
	const_iterator end() const {return data.end();}
	
	unsigned int size() const {return data.size();}
	bool empty() const {return data.empty();}
};*/

#endif
