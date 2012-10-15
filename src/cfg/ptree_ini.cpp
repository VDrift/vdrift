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

/*
 * INI file structure:
 *
 * # comment
 * key1 = value1
 *
 * [key2]
 * key5 = value5
 *
 * [key2.key3]
 * key4 = value4
 *
 */

#include "ptree.h"

struct ini
{
	std::istream & in;
	PTree & root;
	Include * include;

	ini(std::istream & in, PTree & root, Include * inc) :
		in(in), root(root), include(inc)
	{
		// Constructor.
	}

	void read()
	{
		read(root);
	}

	void read(PTree & node)
	{
		std::string line, name;
		while (in.good())
		{
			std::getline(in, line, '\n');
			if (line.empty())
			{
				continue;
			}

			size_t begin = line.find_first_not_of(" \t[");
			size_t end = line.find_first_of(";#]\r", begin);
			if (begin >= end)
			{
				continue;
			}

			size_t next = line.find("=", begin);
			if (next >= end)
			{
				// New node.
				next = line.find_last_not_of(" \t\r]", end);
				name = line.substr(begin, next);
				read(root.set(name, PTree()));
				continue;
			}

			size_t next2 = line.find_first_not_of(" \t\r", next+1);
			next = line.find_last_not_of(" \t", next-1);
			if (next2 >= end)
			{
				continue;
			}

			// New property.
			name = line.substr(begin, next+1);
			//std::string value = line.substr(next2, end-next2-1);
			if (include && line.at(next2) == '&')//name == "include")
			{
				// Value is a reference, include.
				std::string value = line.substr(next2+1, end-next2-1);
				//(*include)(node, value);
				(*include)(node.set(name, value), value);
			}
			else
			{
				std::string value = line.substr(next2, end-next2);
				node.set(name, value);
			}
		}
	}
};

static void write_ini(const PTree & p, std::ostream & out, std::string key_name)
{
	for (PTree::const_iterator i = p.begin(), e = p.end(); i != e; ++i)
	{
		if (i->second.size() == 0)
		{
			out << i->first << " = " << i->second.value() << "\n";
		}
	}
	out << "\n";

	for (PTree::const_iterator i = p.begin(), e = p.end(); i != e; ++i)
	{
		if (i->second.size() > 0)
		{
			out << "[" << key_name + i->first << "]\n";
			write_ini(i->second, out, key_name + i->first + ".");
		}
	}
}

void read_ini(std::istream & in, PTree & p, Include * inc)
{
	ini reader(in, p, inc);
	reader.read();
}

void write_ini(const PTree & p, std::ostream & out)
{
	write_ini(p, out, std::string());
}
