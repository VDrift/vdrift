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
 * XML file format:
 *
 * <!-- comment -->
 * <key1>value1</key1>
 * <key2>
 *    <key3>
 *        <key4>value4</key4>
 *    </key3>
 *    <key5>value5</key5>
 * </key2>
 *
 */

#include "ptree.h"

static void read_xml(std::istream & in, PTree & node, Include * include, std::string key)
{
	std::string line, escape("/" + node.value());
	while (in.good())
	{
		std::getline(in, line, '\n');
		if (line.empty())
		{
			continue;
		}

		size_t begin = line.find_first_not_of(" \t\n<");
		size_t end = line.length();
		if (begin >= end || line[begin] == '!')
		{
			continue;
		}

		line = line.substr(begin, end);

		if (line.find(escape) == 0)
		{
			break;
		}

		if (key.length() == 0)
		{
			end = line.find(" ");
			key = line.substr(0, end);
			continue;
		}

		size_t next = line.find("</"+key);
		if (next < end)
		{
			// New property.
			std::string value = line.substr(0, next);

			// Include?
			if (include && key == "include")
			{
				(*include)(node, value);
			}
			else
			{
				node.set(key, value);
			}
		}
		else
		{
			// New node.
			end = line.find(" ");
			std::string child_key = line.substr(0, end);
			read_xml(in, node.set(key, PTree()), include, child_key);
		}
		key.clear();
	}
}

static void write_xml(const PTree & p, std::ostream & out, std::string indent)
{
	for (PTree::const_iterator i = p.begin(), e = p.end(); i != e; ++i)
	{
		if (i->second.size() == 0)
		{
			out << indent << "<" << i->first << ">" << i->second.value() << "</" << i->first << ">\n";
			write_xml(i->second, out, indent+"\t");
		}
		else
		{
			out << indent << "<" << i->first << ">\n";
			write_xml(i->second, out, indent+"\t");
			out << indent << "</" << i->first << ">\n";
		}
	}
}

void read_xml(std::istream & in, PTree & p, Include * inc)
{
	read_xml(in, p, inc, std::string());
}

void write_xml(const PTree & p, std::ostream & out)
{
	write_xml(p, out, std::string());
}
