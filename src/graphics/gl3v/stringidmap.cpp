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

#include "stringidmap.h"

StringId::StringId() : id(0)
{
	// Constructor.
}

bool StringId::valid() const
{
	return id > 0;
}

bool StringId::operator== (const StringId other) const
{
	return id == other.id;
}

bool StringId::operator< (const StringId other) const
{
	return id < other.id;
}

bool StringIdMap::valid(StringId id)
{
	return id.valid();
}

StringId StringIdMap::addStringId(const std::string & str)
{
	std::unordered_map <std::string, StringId>::iterator i = idmap.find(str);
	if (i != idmap.end())
		return i->second;
	else
	{
		StringId newId = makeStringId(idmap.size()+1);
		idmap.insert(std::make_pair(str,newId));
		stringmap.insert(std::make_pair(newId,str));
		return newId;
	}
}

StringId StringIdMap::getStringId(const std::string & str) const
{
	std::unordered_map <std::string, StringId>::const_iterator i = idmap.find(str);
	if (i != idmap.end())
		return i->second;
	else
		return StringId();
}

std::string StringIdMap::getString(StringId id) const
{
	std::unordered_map <StringId, std::string, StringId::hash>::const_iterator i = stringmap.find(id);
	if (i != stringmap.end())
		return i->second;
	else
		return "";
}

StringId StringIdMap::makeStringId(unsigned int id) const
{
	StringId newId;
	newId.id = id;
	return newId;
}
