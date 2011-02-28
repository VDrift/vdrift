#include "ptree.h"

/*
# ini
key1 = value1

[key2]
key5 = value5

[key2.key3]
key4 = value4
*/

void read_ini(std::istream & in, PTree & node, PTree & root)
{
	std::string line, name;
	while(!std::getline(in, line, '\n').eof())
	{
		size_t begin = line.find_first_not_of(" \t[");
		size_t end = line.find_first_of(";#]", begin);
		if (begin >= end)
		{
			continue;
		}
		
		size_t next = line.find("=", begin);
		if (next >= end)
		{
			next = line.find_last_not_of(" \t]", end);
			name = line.substr(begin, next);
			//std::cerr << "[" << name << "]\n";
			read_ini(in, root.set(name), root);
			continue;
		}
		
		size_t next2 = line.find_first_not_of(" \t", next+1);
		next = line.find_last_not_of(" \t", next-1);
		if (next2 < end)
		{
			name = line.substr(begin, next+1);
			std::string value = line.substr(next2, end);
			//std::cerr << "<" << name << "> = <" << value << ">\n";
			node.set(name, value);
		}
	}
}

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

void read_ini(std::istream & in, PTree & node)
{
	read_ini(in, node, node);
}

void write_ini(const PTree & p, std::ostream & out)
{
	write_ini(p, out, std::string());
}