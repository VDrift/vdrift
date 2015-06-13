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

#ifndef _PTREE_H
#define _PTREE_H

#include <map>
#include <vector>
#include <string>
#include <sstream>
#include <iostream>

class PTree;

/// stream operator for a vector of values
template <typename T>
inline std::istream & operator>>(std::istream & stream, std::vector<T> & out)
{
	if (out.size() > 0)
	{
		/// set vector
		for (size_t i = 0; i < out.size() && !stream.eof(); ++i)
		{
			std::string str;
			std::getline(stream, str, ',');
			std::istringstream s(str);
			s >> out[i];
		}
	}
	else
	{
		/// fill vector
		while (stream.good())
		{
			std::string str;
			std::getline(stream, str, ',');
			std::istringstream s(str);
			T value;
			s >> value;
			out.push_back(value);
		}
	}
	return stream;
}

/// include callback to implement "include" functionality
struct Include
{
	virtual void operator()(PTree & node, std::string & value) = 0;
};

/*
# ini format
key1 = value1

[key2.key3]
key4 = value4

[key2]
key5 = value5
*/
void read_ini(std::istream & in, PTree & p, Include * inc = 0);
void write_ini(const PTree & p, std::ostream & out);

/*
; inf format
key1 value1
key2
{
    key3
    {
        key4 value4
    }
    key5 value5
}
*/
void read_inf(std::istream & in, PTree & p, Include * inc = 0);
void write_inf(const PTree & p, std::ostream & out);

/*
<!-- xml format -->
<key1>value1</key1>
<key2>
    <key3>
		<key4>value4</key4>
	</key3>
    <key5>value5</key5>
</key2>
*/
void read_xml(std::istream & in, PTree & p, Include * inc = 0);
void write_xml(const PTree & p, std::ostream & out);

/// property tree class
/// key and values are stored as strings
class PTree
{
public:
	typedef std::map<std::string, PTree> map;
	typedef map::const_iterator const_iterator;
	typedef map::iterator iterator;

	PTree();

	PTree(const std::string & value);

	/// children nodes begin
	const_iterator begin() const;

	/// children nodes end
	const_iterator end() const;

	/// get number of chidren nodes, 0 if leaf
	int size() const;

	/// depth first node traversal
	template <class T>
	void forEachRecursive(T & functor) const;

	/// get node name or leaf value
	const std::string & value() const;

	/// get node name or leaf value
	std::string & value();

	/// get parent node, null for root
	const PTree * parent() const;

	/// get key value
	/// compound keys are supported: car.wheel.size
	/// return false if not found
	template <typename T>
	bool get(const std::string & key, T & value) const;

	/// get key value, log not found error
	template <typename T>
	bool get(const std::string & key, T & value, std::ostream & error) const;

	/// set key value, create node if required
	template <typename T>
	PTree & set(const std::string & key, const T & value);

	/// overwrite value and children
	void set(const PTree & other);

	/// overwrite value and merge children
	void merge(const PTree & other);

	/// clear children and value
	void clear();

	/// get full node name (down to root)
	std::string fullname(const std::string & name = std::string()) const;

private:
	std::string _value;
	map _children;
	const PTree * _parent;

	/// get typed value from value string template
	template <typename T>
	void _get(const PTree & p, T & value) const;
};

// implementation

inline PTree::PTree() :
	_parent(0)
{
	// ctor
}

inline PTree::PTree(const std::string & value) :
	_value(value),
	_parent(0)
{
	// ctor
}

inline PTree::const_iterator PTree::begin() const
{
	return _children.begin();
}

inline PTree::const_iterator PTree::end() const
{
	return _children.end();
}

inline int PTree::size() const
{
	return _children.size();
}

template <class T>
inline void PTree::forEachRecursive(T & functor) const
{
	functor(*this);
	for (const_iterator i = begin(); i != end(); ++i)
	{
		i->second.forEachRecursive(functor);
	}
}

inline const std::string & PTree::value() const
{
	return _value;
}

inline std::string & PTree::value()
{
	return _value;
}

inline const PTree * PTree::parent() const
{
	return _parent;
}

template <typename T>
inline bool PTree::get(const std::string & key, T & value) const
{
	size_t next = key.find(".");
	const_iterator i = _children.find(key.substr(0, next));
	if (i != _children.end())
	{
		if (next >= key.length()-1)
		{
			_get(i->second, value);
			return true;
		}
		return i->second.get(key.substr(next+1), value);
	}
	return false;
}

template <typename T>
inline bool PTree::get(const std::string & key, T & value, std::ostream & error) const
{
	if (get(key, value))
	{
		return true;
	}

	error << fullname(key) << " not found." << std::endl;
	return false;
}

template <typename T>
inline PTree & PTree::set(const std::string & key, const T & value)
{
	size_t next = key.find(".");
	iterator i = _children.insert(std::make_pair(key.substr(0, next), PTree())).first;
	PTree & p = i->second;
	p._parent = this; ///< store parent pointer for error reporting
	if (next >= key.length()-1)
	{
		std::ostringstream s;
		s << value;
		p._value = s.str();
		return p;
	}
	p._value = i->first; ///< store node key for error reporting
	return p.set(key.substr(next), value);
}

inline void PTree::set(const PTree & other)
{
	_value = other._value;
	_children = other._children;
}

inline void PTree::merge(const PTree & other)
{
	_value = other._value;
	_children.insert(other._children.begin(), other._children.end());
}

inline void PTree::clear()
{
	_children.clear();
}

inline std::string PTree::fullname(const std::string & name) const
{
	std::string full_name;
	if (!name.empty())
	{
		full_name = '.' + name;
	}

	const PTree * node = this;
	while (node && !node->_value.empty())
	{
		full_name = '.' + node->_value + full_name;
		node = node->_parent;
	}

	return full_name;
}

template <typename T>
inline void PTree::_get(const PTree & p, T & value) const
{
	std::istringstream s(p._value);
	s >> value;
}

// specialization

template <>
inline void PTree::_get<std::string>(const PTree & p, std::string & value) const
{
	value = p._value;
}

template <>
inline void PTree::_get<bool>(const PTree & p, bool & value) const
{
	value = (p._value == "1" || p._value == "true" || p._value == "on");
}

template <>
inline void PTree::_get<const PTree *>(const PTree & p, const PTree * & value) const
{
	value = &p;
}

template <>
inline PTree & PTree::set(const std::string & key, const PTree & value)
{
	std::string::size_type n = key.find('.');
	std::pair<iterator, bool> pi = _children.insert(std::make_pair(key.substr(0, n), value));
	PTree & p = pi.first->second;
	if (pi.second && p._value.empty())
	{
		p._value = pi.first->first;
		p._parent = this;
	}
	if (n == std::string::npos)
	{
		return p;
	}
	return p.set(key.substr(n + 1), PTree());
}

#endif //_PTREE_H
