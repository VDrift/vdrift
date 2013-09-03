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

#ifndef RENDERCACHEVECTOR_H
#define RENDERCACHEVECTOR_H

#include <vector>

/// a class very similar to std::vector but which will automatically resize the vector if operator[] is used outside
/// the bounds of the current size
template <typename T>
class RenderCacheVector
{
	public:
		typedef typename std::vector<T>::size_type size_type;
		typedef typename std::vector<T>::iterator iterator;
		typedef typename std::vector<T>::const_iterator const_iterator;

		iterator begin() {return data.begin();}
		const_iterator begin() const {return data.begin();}
		iterator end() {return data.end();}
		const_iterator end() const {return data.end();}

		RenderCacheVector & operator=(const RenderCacheVector & other) {Set(other); return *this;}

		void setResizeValue(const T & value)
		{
			resizeValue = value;
		}

		inline T & operator[](size_type n)
		{
			if (n >= data.size())
				data.resize(n+1, resizeValue);
			return data[n];
		}

		inline void Set(const RenderCacheVector & other)
		{
			data = other.data;
		}

		inline void resize(size_type n)
		{
			data.resize(n, resizeValue);
		}

		inline size_type size() const
		{
			return data.size();
		}

		inline void clear()
		{
			data.clear();
		}

	private:
		std::vector <T> data;
		T resizeValue;
};

#endif
