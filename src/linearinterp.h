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

#ifndef _LINEARINTERP_H
#define _LINEARINTERP_H

#include "pairsort.h"

#include <vector>
#include <map>
#include <algorithm>
#include <cassert>

template <typename T>
class LinearInterp
{
public:
	enum BoundaryEnum
	{
		CONSTANTSLOPE,
  		CONSTANTVALUE
	};

	LinearInterp() : empty_value(0), mode(CONSTANTVALUE) {}

	LinearInterp(T empty_value_) : empty_value(empty_value_), mode(CONSTANTVALUE) {} ///< Interpolate will return the given empty_value if no points exist

	void Clear()
	{
		points.clear();
	}

	void Reserve(size_t n)
	{
		points.reserve(n);
	}

	void AddPoint(const T x, const T y)
	{
		points.push_back(std::pair <T,T> (x,y));
		PairSortFirst <T> sorter;
		std::sort(points.begin(), points.end(), sorter);
	}

	T Interpolate(T x) const
	{
		if (points.empty())
			return empty_value;

		if (points.size() == 1)
			return points[0].second;

		size_t low = 0;
		size_t high = points.size () - 1;

		// handle the case where the value is out of bounds
		if (mode == CONSTANTVALUE)
		{
			if (x > points[high].first)
				return points[high].second;
			else if (x < points[low].first)
				return points[low].second;
		}

		// Bisect to find the interval that distance is on.
		while (high - low > 1)
		{
			size_t index = (high + low) / 2;
			if (points[index].first > x)
				high = index;
			else
				low = index;
		}

		const T dx = points[high].first - points[low].first;
		const T dy = points[high].second - points[low].second;
		assert(dx > 0);

		return points[low].second + dy / dx * (x - points[low].first);
	}

	/// if the mode is set to CONSTANTSLOPE, then values outside of the bounds will be extrapolated based
	/// on the slope of the closest points.  if set to CONSTANTVALUE, the values outside of the bounds will
	/// be set to the value of the closest point.
	void SetBoundaryMode(const BoundaryEnum & value)
	{
		mode = value;
	}

private:
	std::vector <std::pair <T, T> > points;
	T empty_value;
	BoundaryEnum mode;
};

#endif
