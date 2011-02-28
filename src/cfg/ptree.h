#ifndef _PTREE_H
#define _PTREE_H

#include <map>
#include <string>
#include <sstream>
#include <iostream>
#include <algorithm>

class PTree
{
public:
	typedef std::map<std::string, PTree> map;
	typedef map::const_iterator const_iterator;
	typedef map::iterator iterator;
	
	PTree() : _parent(0) {}
	PTree(const std::string & value) : _value(value), _parent(0) {}
	
	const_iterator begin() const
	{
		return _children.begin();
	}
	
	const_iterator end() const
	{
		return _children.end();
	}
	
	int size() const
	{
		return _children.size();
	}
	
	const std::string & value() const
	{
		return _value;
	}
	
	template <typename T> bool get(const std::string & key, T & value) const
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
	
	template <typename T> bool get(const std::string & key, T & value, std::ostream & error) const
	{
		if (get(key, value))
		{
			return true;
		}
		
		std::string full_key = key;
		const PTree * parent = this;
		while (parent)
		{
			full_key = parent->_value + '.' + full_key; // value contains node key
			parent = parent->_parent;
		}
		error << full_key << " not found." << std::endl;
		return false;
	}
	
	template <typename T> PTree & set(const std::string & key, const T & value)
	{
		size_t next = key.find(".");
		iterator i = _children.insert(std::make_pair(key.substr(0, next), PTree())).first;
		PTree & p = i->second;
		p._parent = this; // store parent pointer for error reporting
		if (next >= key.length()-1)
		{
			std::stringstream s;
			s << value;
			p._value = s.str();
			return p;
		}
		p._value = i->first; // store/copy node key for error reporting
		return p.set(key.substr(next), value);
	}
	
	PTree & set(const std::string & key)
	{
		std::string::const_iterator next = std::find(key.begin(), key.end(), '.');
		std::pair<iterator, bool> pi = _children.insert(std::make_pair(std::string(key.begin(), next), PTree()));
		PTree & p = pi.first->second;
		if (pi.second)
		{
			p._value = pi.first->first;
			p._parent = this;
		}
		if (next == key.end())
		{
			return p;
		}
		return p.set(std::string(next+1, key.end()));
	}
	
	void clear()
	{
		_children.clear();
	}
	
private:
	std::string _value;
	map _children;
	const PTree * _parent;
	
	template <typename T> void _get(const PTree & p, T & value) const
	{
		std::stringstream s(p._value);
		s >> value;
	}
};

// specializations
template <> inline void PTree::_get<std::string>(const PTree & p, std::string & value) const
{
	value = p._value;
}

template <> inline void PTree::_get<bool>(const PTree & p, bool & value) const
{
	value = (p._value == "1" || p._value == "true" || p._value == "on");
}

template <> inline void PTree::_get<const PTree *>(const PTree & p, const PTree * & value) const
{
	value = &p;
}

/*
# ini format
key1 = value1

[key2.key3]
key4 = value4

[key2]
key5 = value5
*/
void read_ini(std::istream & in, PTree & p);
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
void read_inf(std::istream & in, PTree & p);
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
void read_xml(std::istream & in, PTree & p);
void write_xml(const PTree & p, std::ostream & out);

#endif //_PTREE_H