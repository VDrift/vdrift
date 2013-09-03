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

#ifndef _PAIRSORT_H
#define _PAIRSORT_H

#include <map>

template <typename T>
class PairSortFirst
{
	public:
	bool operator() (const std::pair <T, T> & p1, const std::pair <T, T> & p2) const
	{
		return p1.first < p2.first;
	}
};

template <typename T>
class PairSortSecond
{
	public:
	bool operator() (const std::pair <T, T> & p1, const std::pair <T, T> & p2) const
	{
		return p1.second < p2.second;
	}
};

#endif
