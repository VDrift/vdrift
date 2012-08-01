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
 * INF file structure:
 *
 * ; comment
 * key1 value1
 * key2
 * {
 *     key3
 *     {
 *         key4 value4
 *     }
 *     key5 value5
 * }
 *
 */

#include "ptree.h"

static void read_inf(std::istream & in, PTree & node, Include * include, bool child)
{
	std::string line, name;
	while (in.good())
	{
		std::getline(in, line, '\n');
		if (line.empty())
		{
			continue;
		}

		size_t begin = line.find_first_not_of(" \t");
		size_t end = line.find_first_of(";#");
		if (begin >= end)
		{
			continue;
		}

		line = line.substr(begin, end);
		if (line[0] == '{' && name.length())
		{
			// New node.
			read_inf(in, node.set(name, PTree()), include, true);
			continue;
		}

		if (line[0] == '}' && child)
		{
			break;
		}

		size_t next = line.find(" ");
		end = line.length();
		name = line.substr(0, next);
		if (next < end)
		{
			// New property.
			std::string value = line.substr(next+1, end);

			// Include?
			if (include && name == "include")
			{
				(*include)(node, value);
			}
			else
			{
				node.set(name, value);
			}
			name.clear();
		}
	}
}

static void write_inf(const PTree & p, std::ostream & out, std::string indent)
{
	for (PTree::const_iterator i = p.begin(), e = p.end(); i != e; ++i)
	{
		if (i->second.size() == 0)
		{
			out << indent << i->first << " " << i->second.value() << "\n";
			write_inf(i->second, out, indent+"\t");
		}
		else
		{
			out << indent << i->first << "\n";
			out << indent << "{" << "\n";
			write_inf(i->second, out, indent+"\t");
			out << indent << "}" << "\n";
		}
	}
}

void read_inf(std::istream & in, PTree & p, Include * inc)
{
	read_inf(in, p, inc, false);
}

void write_inf(const PTree & p, std::ostream & out)
{
	write_inf(p, out, std::string());
}
