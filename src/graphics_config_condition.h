#ifndef _GRAPHICS_CONFIG_CONDITION_H
#define _GRAPHICS_CONFIG_CONDITION_H

#include <string>
#include <vector>
#include <map>
#include <fstream>
#include <iostream>
#include <set>

/// a space-delimited list of string conditions. putting ! before the condition negates it.
class GRAPHICS_CONFIG_CONDITION
{
public:
	void Parse(const std::string & str);
	bool Satisfied(const std::set <std::string> & conditions) const;

private:
	std::set <std::string> positive_conditions;
	std::set <std::string> negated_conditions;
};

#endif
