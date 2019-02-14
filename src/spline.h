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

#ifndef _SPLINE_H
#define _SPLINE_H

#include "pairsort.h"

#include <vector>
#include <map>
#include <algorithm>
#include <cassert>

template <typename T>
class Spline
{
private:
	std::vector <std::pair <T, T> > points;
	std::vector <T> second_derivs;
	T first_slope;
	T last_slope;
	bool derivs_calculated;

	void CalculateDerivs()
	{
		size_t n = points.size ();
		assert (n > 1);
		T * a = new T[n];
		T * b = new T[n];
		T * c = new T[n];
		T * r = new T[n];

		// Fill in the arrays that represent the tridiagonal matrix.
		// a [0] is not used.
		T diff = points [1].first - points [0].first;
		b [0] = diff * T(1/3.0);
		c [0] = diff * T(1/6.0);
		r [0] = ( points [1].second - points [0].second ) / diff - first_slope;

		for ( size_t i = 1; i < n - 1; i++ )
		{
			T diff1 = points [i+1].first - points [i].first;
			T diff2 = points [i].first - points [i-1].first;

			a [i] = diff2 * T(1/6.0);
			b [i] = ( points [i+1].first - points [i-1].first ) * T(1/3.0);
			c [i] = diff1 * T(1/6.0);
			r [i] = ( points [i+1].second - points [i].second ) / diff1
				- ( points [i].second - points [i-1].second ) / diff2;
		}

		diff = points [n-1].first - points [n-2].first;
		a [n-1] = diff * T(1/6.0);
		b [n-1] = diff * T(1/3.0);
		// c [n-1] is not used.
		r [n-1] = last_slope - ( points [n-1].second - points [n-2].second ) / diff;

		// Gauss-Jordan Elimination
		for ( size_t i = 1; i < n; i++ )
		{
			// Replace row i with row i - k * row (i-1) such that A_{i,i-1} = 0.0.
			T factor = a [i] / b [i-1];
			// A_{i,i-1} is not used again, so it need not be calculated.
			b [i] -= factor * c [i-1];
			// A_{i,i+1} is unchanged because A_{i-1,i+1} = 0.0.
			r [i] -= factor * r [i-1];
		}

		// Back-Substitution

		// Solve for y"[N].
		second_derivs.resize ( n );
		second_derivs [n-1] = r [n-1] / b [n-1];
		for ( int i = n - 2; i >= 0; i-- )
		{
			// Use the solution for y"[i+1] to find y"[i].
			second_derivs [i] = ( r [i] - c [i] * second_derivs [i+1] ) / b [i];
		}

		delete [] a;
		delete [] b;
		delete [] c;
		delete [] r;

		derivs_calculated = true;
	}

public:
	Spline() : first_slope(0), last_slope(0), derivs_calculated(false) {}

	void Clear()
	{
		points.clear();
		second_derivs.clear();
		derivs_calculated = false;
	}

	void Reserve(size_t n)
	{
		points.reserve(n);
	}

	void AddPoint(const T x, const T y)
	{
		points.push_back(std::pair <T,T> (x,y));
		derivs_calculated = false;
	}

	// call calculate after adding points to initialize spline
	void Calculate()
	{
		PairSortFirst <T> sorter;
		std::sort(points.begin(), points.end(), sorter);
		CalculateDerivs();
	}

	std::pair<T, T> GetMaxY() const
	{
		std::pair<T, T> maxy(0, 0);
		for (size_t i = 0, n = points.size(); i < n ; i++)
			if (points[i].second > maxy.second)
				maxy = points[i];
		return maxy;
	}

	T Interpolate(T x) const
	{
		assert ( derivs_calculated );

		if ( points.size() == 1 )
			return points [0].second;

		// Bisect to find the interval that distance is on.
		size_t low = 0;
		size_t high = points.size () - 1;
		while ( high - low > 1 )
		{
			size_t index = ( high + low ) / 2;
			if ( points [index].first > x )
				high = index;
			else
				low = index;
		}

		// Make sure that x_high > x_low.
		const T diff = points [high].first - points [low].first;
		assert ( diff >= 0 );

		// Evaluate the coefficients for the cubic spline equation.
		const T a = ( points [high].first - x ) / diff;
		const T b = 1 - a;
		const T sq = diff * diff * T(1/6.0);
		const T a2 = a * a;
		const T b2 = b * b;

		// Return the interpolated value.
		return a * points [low].second
			+ b * points [high].second
			+ a * ( a2 - 1 ) * sq * second_derivs [low]
			+ b * ( b2 - 1 ) * sq * second_derivs [high];
	}
};

#endif
