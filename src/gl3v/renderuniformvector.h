#ifndef RENDERUNIFORMVECTOR_H
#define RENDERUNIFORMVECTOR_H

#define MAXUNIFORMSIZE 16

#include <vector>
#include <cstring>

/// an optimized vector class for use in sending opengl uniforms to the renderer
/// it's set up for speed, not safety or memory use
template <typename T>
class RenderUniformVector
{
	public:
		typedef size_t size_type;
		typedef T* iterator;
		typedef const T* const_iterator;

		RenderUniformVector() : curSize(0) {}
		RenderUniformVector(const RenderUniformVector & other) {Set(other);}
		RenderUniformVector(const T * newData, size_type newSize) {Set(newData, newSize);}
		RenderUniformVector(const std::vector <T> & other)
		{
			Set(&other[0],other.size());
		}

		iterator begin() {return &data[0];}
		const_iterator begin() const {return &data[0];}
		iterator end() {return &data[curSize];}
		const_iterator end() const {return &data[curSize];}

		const RenderUniformVector & operator=(const RenderUniformVector & other) {Set(other); return *this;}

		inline const T & operator[](size_type n) const
		{
			#ifdef BE_SAFER
			assert(n < MAXUNIFORMSIZE);
			#endif
			return data[n];
		}

		inline T & operator[](size_type n)
		{
			#ifdef BE_SAFER
			assert(n < MAXUNIFORMSIZE);
			#endif
			return data[n];
		}

		inline void Set(const T * newdata, size_type newSize)
		{
			#ifdef BE_SAFER
			assert(newSize < MAXUNIFORMSIZE);
			#endif
			curSize = newSize;
			std::memcpy(data,newdata,sizeof(T)*curSize);
		}

		inline void Set(const RenderUniformVector & other)
		{
			Set(other.data, other.curSize);
		}

		inline void resize(size_type n)
		{
			#ifdef BE_SAFER
			assert(n < MAXUNIFORMSIZE);
			#endif
			curSize = n;
		}

		inline size_type size() const
		{
			return curSize;
		}

	private:
		T data[16];
		size_type curSize;
};

#undef MAXUNIFORMSIZE

#endif
