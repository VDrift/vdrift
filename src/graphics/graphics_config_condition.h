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

#ifndef _GRAPHICS_CONFIG_CONDITION_H
#define _GRAPHICS_CONFIG_CONDITION_H

#include <string>
#include <set>

/// A space-delimited list of string conditions.
/// Putting "!" before the condition negates it.
class GraphicsConfigCondition
{
public:
	void Parse(const std::string & str);
	bool Satisfied(const std::set <std::string> & conditions) const;

private:
	std::set <std::string> positive_conditions;
	std::set <std::string> negated_conditions;
};

#endif
