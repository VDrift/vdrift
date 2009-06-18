#ifndef _MATRIX4_H
#define _MATRIX4_H

#include <iostream>
#include <cstring>

template <typename T>
class MATRIX4
{
	private:
		typedef size_t size_type;
		T data[16];
	
	public:
		MATRIX4() {LoadIdentity();}
		MATRIX4(const MATRIX4 <T> & other) {Set(other);}
		const MATRIX4 & operator=(const MATRIX4 <T> & other) {Set(other); return *this;}
		
		const T & operator[](size_type n) const
		{
			assert(n < 16);
			return data[n];
		}
	
		T & operator[](size_type n)
		{
			assert(n < 16);
			return data[n];
		}
		
		void DebugPrint(std::ostream & out) const
		{
			for (size_type i = 0; i < 16; i++)
			{
				out << i << ". " << data[i] << std::endl;
			}
		}
		
		MATRIX4 <T> Multiply(const MATRIX4 <T> & other) const
		{
			MATRIX4 out;
	
			for (int i = 0, i4 = 0; i < 4; i++,i4+=4)
			{
				for (int j = 0; j < 4; j++)
				{
					out.data[i4+j] = 0;
			
					for (int k = 0, k4 = 0; k < 4; k ++,k4+=4)
						out.data[i4+j] += data[i4+k]*other.data[k4+j];
				}
			}
	
			return out;
		}
		
		void LoadIdentity()
		{
			data[0] = data[5] = data[10] = data[15] = 1;
			data[1] = data[2] = data[3] = data[4] = data[6] = data[7] = data[8] = data[9] = data[11] = data[12] = data[13] = data[14] = 0;
		}
		
		inline void Set(const T * newdata)
		{
			std::memcpy(data,newdata,sizeof(T)*16); //high performance, but portability issues?
			/*for (int i = 0; i < 16; i++)
				data[i] = newdata[i];*/
		}
		
		inline void Set(const MATRIX4 <T> & other) {Set(other.data);}
		
		void Translate(const float tx, const float ty, const float tz)
		{
			data[12] += tx;
			data[13] += ty;
			data[14] += tz;
		}
		
		bool operator==(const MATRIX4 <T> & other) {return Equals(other);}
		
		bool Equals(const MATRIX4 <T> & other)
		{
			return (memcmp(data,other.data,16*sizeof(T)) == 0); //high performance, but portability issues?
			/*for (int i = 0; i < 16; i++)
				if (data[i] != other.data[i])
					return false;
			
			return true;*/
		}
		
		void TransformVectorIn(float & x, float & y, float & z) const
		{
			float outx = x * data[0] + y * data[1] + z * data[2];
			float outy = x * data[4] + y * data[5] + z * data[6];
			float outz = x * data[8] + y * data[9] + z * data[10];
			
			x = outx;
			y = outy;
			z = outz;
		}
		
		void TransformVectorOut(float & x, float & y, float & z) const
		{
			float outx = x * data[0] + y * data[4] + z * data[8] + data[12];
			float outy = x * data[1] + y * data[5] + z * data[9] + data[13];
			float outz = x * data[2] + y * data[6] + z * data[10] + data[14];
			
			x = outx;
			y = outy;
			z = outz;
		}
		
		void Scale(T scalar)
		{
			MATRIX4 <T> scalemat;
			scalemat.data[15] = 1;
			scalemat.data[0] = scalemat.data[5] = scalemat.data[10] = scalar;
			scalemat.data[1] = scalemat.data[2] = scalemat.data[3] = scalemat.data[4] = scalemat.data[6] = scalemat.data[7] = scalemat.data[8] = scalemat.data[9] = scalemat.data[11] = scalemat.data[12] = scalemat.data[13] = scalemat.data[14] = 0;
			*this = Multiply(scalemat);
		}
		
		const T * GetArray() const {return data;}
};

#endif
