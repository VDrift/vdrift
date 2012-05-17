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

		const RenderCacheVector & operator=(const RenderCacheVector & other) {Set(other); return *this;}

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
