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

#ifndef _RADIX_H
#define _RADIX_H

#include <vector>

/// 4 bytes signed/unsigned radix sort with temporal coherence
/// Based on Pierre Terdimans "Radix Sort Revisited".
/// Only floats sort implemented currently.
/// Signed sort will fail in big endian machines (fixme).
class Radix
{
public:
	Radix();

	/// Process input list and gen a sorted input indices list.
	/// Returns false if the list is already/still sorted.
	/// greater_than_zero: hint that input values are greater than zero.
	bool sort(const std::vector<float> & input, bool greater_than_zero = false);

	/// Sort result as indices of input list in sorted order.
	const std::vector<unsigned> & getRanks() const { return m_ranks[m_ranks_id]; }

private:
	std::vector<unsigned> m_ranks[2];
	unsigned m_ranks_id;
};

#endif // _RADIX_H
