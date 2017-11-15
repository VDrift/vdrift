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

#ifndef _STRINGIDMAP
#define _STRINGIDMAP

#include <unordered_map>
#include <functional>
#include <string>

class StringId
{
	friend class StringIdMap;
public:
	StringId();

	bool valid() const;

	bool operator== (const StringId other) const;
	bool operator< (const StringId other) const;

	struct hash
	{
		std::size_t operator()(const StringId toHash) const {return std::hash<unsigned int>()(toHash.id);}
	};

private:
	unsigned int id;
};

/// Allows mapping of strings to a numeric ID.
class StringIdMap
{
public:
	bool valid(StringId id);

	/// Returns the string id if it already exists, otherwise creates an id and returns it.
	StringId addStringId(const std::string & str);

	/// Returns an invalid string ID if there's no ID for this string.
	StringId getStringId(const std::string & str) const;

	/// Returns an empty string if the id was not found.
	std::string getString(StringId id) const;

private:
	std::unordered_map <std::string, StringId> idmap;
	std::unordered_map <StringId, std::string, StringId::hash> stringmap;

	StringId makeStringId(unsigned int id) const;
};


inline StringId::StringId() : id(0)
{
	// Constructor.
}

inline bool StringId::valid() const
{
	return id > 0;
}

inline bool StringId::operator== (const StringId other) const
{
	return id == other.id;
}

inline bool StringId::operator< (const StringId other) const
{
	return id < other.id;
}

inline bool StringIdMap::valid(StringId id)
{
	return id.valid();
}

inline StringId StringIdMap::addStringId(const std::string & str)
{
	auto i = idmap.find(str);
	if (i != idmap.end())
		return i->second;

	StringId newId = makeStringId(idmap.size()+1);
	idmap.insert(std::make_pair(str,newId));
	stringmap.insert(std::make_pair(newId,str));
	return newId;
}

inline StringId StringIdMap::getStringId(const std::string & str) const
{
	auto i = idmap.find(str);
	if (i != idmap.end())
		return i->second;

	return StringId();
}

inline std::string StringIdMap::getString(StringId id) const
{
	auto i = stringmap.find(id);
	if (i != stringmap.end())
		return i->second;

	return "";
}

inline StringId StringIdMap::makeStringId(unsigned int id) const
{
	StringId newId;
	newId.id = id;
	return newId;
}

#endif
