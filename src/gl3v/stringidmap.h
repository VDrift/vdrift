#ifndef _STRINGIDMAP
#define _STRINGIDMAP

#include "unordered_map.h"
#include <map>

class StringId
{
	friend class StringIdMap;
	public:
		StringId() : id(0) {}
		bool valid() const {return id > 0;}
		bool operator==(const StringId other) const {return id == other.id;}
		bool operator<(const StringId other) const {return id < other.id;}
		struct hash
		{
			std::size_t operator()(const StringId toHash) const {return std::tr1::hash<unsigned int>()(toHash.id);}
		};

	private:
		unsigned int id;
};

/// allows mapping of strings to a numeric ID
class StringIdMap
{
	public:
		static bool valid(StringId id) {return id.valid();}

		/// returns the string id if it already exists, otherwise creates an id and returns it
		StringId addStringId(const std::string & str)
		{
			std::tr1::unordered_map <std::string, StringId>::iterator i = idmap.find(str);
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

		/// returns an invalid string ID if there's no ID for this string
		StringId getStringId(const std::string & str) const
		{
			std::tr1::unordered_map <std::string, StringId>::const_iterator i = idmap.find(str);
			if (i != idmap.end())
				return i->second;
			else
				return StringId();
		}

		/// returns an empty string if the id was not found
		std::string getString(StringId id) const
		{
			std::tr1::unordered_map <StringId, std::string, StringId::hash>::const_iterator i = stringmap.find(id);
			if (i != stringmap.end())
			{
				return i->second;
			}
			else
				return "";
		}

	private:
		std::tr1::unordered_map <std::string, StringId> idmap;
		std::tr1::unordered_map <StringId, std::string, StringId::hash> stringmap;

		StringId makeStringId(unsigned int id) const
		{
			StringId newId;
			newId.id = id;
			return newId;
		}
};

#endif
