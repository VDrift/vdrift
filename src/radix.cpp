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

#include "radix.h"
#include <cassert>

// Return false if the list is already sorted.
template <typename T>
static inline bool ComputeCounters(
	unsigned counters[1024],
	const std::vector<T> & input,
	std::vector<unsigned> & ranks,
	bool ranks_valid)
{
	assert(sizeof(T) == 4);

	const unsigned char * bytes = (const unsigned char *)&input[0];
	const unsigned char * bytes_end = bytes + 4 * input.size();
	unsigned * c0 = &counters[0];
	unsigned * c1 = &counters[256];
	unsigned * c2 = &counters[512];
	unsigned * c3 = &counters[768];

	bool sorted = true;
	if (!ranks_valid)
	{
		// Check whether input is alredy sorted while accum the counters.
		typename std::vector<T>::const_iterator it = input.begin();
		T vprev = *it;
		while (bytes != bytes_end)
		{
			const T v = *it++;
			if (v < vprev)
			{
				// Input not sorted, early out here.
				sorted = false;
				break;
			}
			vprev = v;

			// Accumulate counters.
			c0[*bytes++]++;
			c1[*bytes++]++;
			c2[*bytes++]++;
			c3[*bytes++]++;
		}

		// If input values are already sorted, leave the list unchanged.
		if (sorted)
		{
			for (unsigned i = 0; i < input.size(); ++i)
			{
				ranks[i] = i;
			}
			return false;
		}
	}
	else
	{
		// Check whether input is still sorted according to indices list.
		const unsigned * id = &ranks[0];
		T vprev = input[*id];
		while (bytes != bytes_end)
		{
			const T v = input[*id++];
			if(v < vprev)
			{
				// Input not sorted, early out here.
				sorted = false;
				break;
			}
			vprev = v;

			// Accumulate counters.
			c0[*bytes++]++;
			c1[*bytes++]++;
			c2[*bytes++]++;
			c3[*bytes++]++;
		}

		// If input values are already sorted, return.
		if (sorted)
		{
			return false;
		}
	}

	// Finish counters accumulation.
	while (bytes != bytes_end)
	{
		c0[*bytes++]++;
		c1[*bytes++]++;
		c2[*bytes++]++;
		c3[*bytes++]++;
	}

	return true;
}

// binput pointing to nth byte, LSB to MSB
// counter for the nth byte of input, LSB to MSB
template <typename Type>
inline static void RadixPassPos(
	const unsigned pass,
	const unsigned num,
	const Type input[],
	const unsigned counters[],
	unsigned * offsets[],
	unsigned *& ranks0,
	unsigned *& ranks1,
	bool & ranks_valid)
{
	const unsigned char * binput = (const unsigned char *)input + pass;
	const unsigned * counter = &counters[pass * 256];

	// If all values have the same byte, sorting is useless.
	// Check first byte's counter to verify this.
	if (counter[binput[0]] == num)
		return;

	// Init offsets.
	offsets[0] = ranks1;
	for (unsigned i = 1; i < 256; ++i)
	{
		offsets[i] = offsets[i - 1] + counter[i - 1];
	}

	// Perform Radix Sort.
	if (!ranks_valid)
	{
		for (unsigned i = 0; i < num; ++i)
		{
			*offsets[binput[i * 4]]++ = i;
		}
		ranks_valid = true;
	}
	else
	{
		for (unsigned i = 0; i < num; ++i)
		{
			const unsigned id = ranks0[i];
			*offsets[binput[id * 4]]++ = id;
		}
	}

	// Swap pointers for next pass.
	unsigned * tmp = ranks0;
	ranks0 = ranks1;
	ranks1 = tmp;
}

// binput pointing to first MSB
// counter for the MSB
template <typename Type>
inline static void RadixPassNeg(
	const unsigned pass,
	const unsigned num,
	const Type input[],
	const unsigned counters[],
	unsigned * offsets[],
	unsigned *& ranks0,
	unsigned *& ranks1,
	bool & ranks_valid)
{
	const unsigned char * binput = (const unsigned char *)input + pass;
	const unsigned * uinput = (const unsigned *)input;
	const unsigned * counter = &counters[pass * 256];

	// If all values have the same byte, sorting is useless.
	// Check first byte's counter to verify this.
	if (counter[binput[0]] == num)
	{
		// Still need to reverse the order of current list if all values are negative.
		if (binput[0] >= 128)
		{
			if (!ranks_valid)
			{
				for (unsigned i = 0; i < num; ++i)
				{
					ranks1[i] = num - i - 1;
				}
				ranks_valid = true;
			}
			else
			{
				for (unsigned i = 0; i < num; ++i)
				{
					ranks1[i] = ranks0[num - i - 1];
				}
			}

			// Swap pointers for next pass.
			unsigned * tmp = ranks0;
			ranks0 = ranks1;
			ranks1 = tmp;
		}

		return;
	}

	// A way to compute the number of negatives values is simply to sum the 128
	// last values of the last counter, responsible for the sign.
	// 128 last values because the 128 first ones are related to positive numbers.
	unsigned negvalues = 0;
	for (unsigned i = 128; i < 256; ++i)
	{
		negvalues += counter[i];
	}

	// Create biased offsets, in order for negative numbers to be sorted as well.

	// First positive number takes place after the negative ones.
	offsets[0] = &ranks1[negvalues];

	// 1 to 128 for positive numbers
	for (unsigned i = 1; i < 128; ++i)
	{
		offsets[i] = offsets[i - 1] + counter[i - 1];
	}

	// Have to reverse the sorting order for negative numbers.
	offsets[255] = ranks1;

	// Fix the wrong order for negative values.
	for (unsigned i = 0; i < 127; ++i)
	{
		offsets[254 - i] = offsets[255 - i] + counter[255 - i];
	}

	// Fix the wrong place for negative values.
	for (unsigned i = 128; i < 256; ++i)
	{
		offsets[i] += counter[i];
	}

	// Perform Radix Sort.
	if (!ranks_valid)
	{
		for (unsigned i = 0; i < num; ++i)
		{
			const unsigned radix = uinput[i] >> 24;
			if (radix < 128)
				*offsets[radix]++ = i;   // Number is positive.
			else
				*(--offsets[radix]) = i; // Number is negative, flip sorting order.
		}
		ranks_valid = true;
	}
	else
	{
		for (unsigned i = 0; i < num; ++i)
		{
			const unsigned radix = uinput[ranks0[i]] >> 24;
			if (radix < 128)
				*offsets[radix]++ = ranks0[i];   // Number is positive
			else
				*(--offsets[radix]) = ranks0[i]; // Number is negative, flip sorting order.
		}
	}

	// Swap pointers for next pass.
	unsigned * tmp = ranks0;
	ranks0 = ranks1;
	ranks1 = tmp;
}

Radix::Radix() : m_ranks_id(0)
{
	// ctor
}

bool Radix::sort(const std::vector<float> & input, bool greater_than_zero)
{
	unsigned counters[256 * 4] = {};
	unsigned * offsets[256] = {};

	unsigned num = input.size();
	bool ranks_valid = m_ranks[0].size() == num;
	if (!ranks_valid)
	{
		m_ranks[0].resize(num);
		m_ranks[1].resize(num);
		m_ranks_id = 0;
	}

	// Compute counters and early out if input is already/still sorted
	if (!ComputeCounters(counters, input, m_ranks[m_ranks_id], ranks_valid))
		return false;

	// Radix sort, 4 passes LSB to MSB
	unsigned * ranks0 = &m_ranks[m_ranks_id][0];
	unsigned * ranks1 = &m_ranks[(m_ranks_id + 1) & 1][0];
	for (unsigned pass = 0; pass < 4; ++pass)
	{
		if (greater_than_zero || pass != 3)
		{
			// Handle positive values.
			RadixPassPos(pass, num, &input[0], counters, offsets, ranks0, ranks1, ranks_valid);
		}
		else
		{
			// Handle negative values.
			RadixPassNeg(pass, num, &input[0], counters, offsets, ranks0, ranks1, ranks_valid);
		}
	}

	// Set sorted indices list.
	m_ranks_id = (ranks0 == &m_ranks[0][0]) ? 0 : 1;

	return true;
}


#include "unittest.h"
#include <cstdlib>

QT_TEST(radix_test)
{
	Radix rsort;

	// test signed valiues
	std::vector<float> input(20, 0.0);
	for (unsigned i = 0; i < input.size(); ++i)
	{
		input[i] = float(rand() - 16384);
	}

	rsort.sort(input);

	// verify sort result
	float v0 = input[rsort.getRanks()[0]];
	for (unsigned i = 0; i < input.size(); ++i)
	{
		float v1 = input[rsort.getRanks()[i]];
		QT_CHECK_LESS_OR_EQUAL(v0, v1);
		v0 = v1;
	}

	// check temporal coherence
	bool resort = rsort.sort(input);
	QT_CHECK(!resort);

	// test positive values only
	for (unsigned i = 0; i < input.size(); ++i)
	{
		input[i] = float(rand());
	}

	resort = rsort.sort(input, true);
	QT_CHECK(resort);

	// verify sort result
	v0 = input[rsort.getRanks()[0]];
	for (unsigned i = 0; i < input.size(); ++i)
	{
		float v1 = input[rsort.getRanks()[i]];
		QT_CHECK_LESS_OR_EQUAL(v0, v1);
		v0 = v1;
	}
}
