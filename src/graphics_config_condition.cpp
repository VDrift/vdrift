#include "graphics_config_condition.h"

#include <sstream>

void GRAPHICS_CONFIG_CONDITION::Parse(const std::string & str)
{
	std::stringstream parser(str);
	while (parser)
	{
		std::string condition;
		parser >> condition;
		
		if (!condition.empty())
		{
			if (condition[0] == '!')
			{
				if (condition.size() > 1)
				{
					std::string cond = condition.substr(1);
					if (positive_conditions.find(cond) == positive_conditions.end())
						negated_conditions.insert(cond);
					else
						positive_conditions.erase(cond);
				}
			}
			else
			{
				if (negated_conditions.find(condition) == negated_conditions.end())
					positive_conditions.insert(condition);
				else
					negated_conditions.erase(condition);
			}
		}
	}
}

bool GRAPHICS_CONFIG_CONDITION::Satisfied(const std::set <std::string> & conditions) const
{
	for (std::set <std::string>::const_iterator i = positive_conditions.begin(); i != positive_conditions.end(); i++)
	{
		if (conditions.find(*i) == conditions.end())
		{
			return false;
		}
	}
	
	for (std::set <std::string>::const_iterator i = negated_conditions.begin(); i != negated_conditions.end(); i++)
	{
		if (conditions.find(*i) != conditions.end())
		{
			return false;
		}
	}
	
	return true;
}
