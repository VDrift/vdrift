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

#include <sstream>
#include "graphics_config_condition.h"

void GraphicsConfigCondition::Parse(const std::string & str)
{
	std::istringstream parser(str);
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
				if (negated_conditions.find(condition) == negated_conditions.end())
					positive_conditions.insert(condition);
				else
					negated_conditions.erase(condition);
		}
	}
}

bool GraphicsConfigCondition::Satisfied(const std::set <std::string> & conditions) const
{
	// If we don't find all of our positive conditions in the conditions set, return false.
	for (std::set <std::string>::const_iterator i = positive_conditions.begin(); i != positive_conditions.end(); i++)
		if (conditions.find(*i) == conditions.end())
			return false;

	// If we find any of our negative conditions in the conditions set, return false.
	for (std::set <std::string>::const_iterator i = negated_conditions.begin(); i != negated_conditions.end(); i++)
		if (conditions.find(*i) != conditions.end())
			return false;

	return true;
}
