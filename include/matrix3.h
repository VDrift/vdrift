#ifndef _MATRIX3_H
#define _MATRIX3_H

#include <iostream>

#include "mathvector.h"

#include <cstring>

template <typename T>
class MATRIX3
{
	private:
		typedef size_t size_type;
		T data[9];
		
		T myfabs(const T & val) const
		{
			if (val < 0)
				return -val;
			return val;
		}
	
	public:
		MATRIX3() {LoadIdentity();}
		MATRIX3(const MATRIX3 <T> & other) {Set(other);}
		const MATRIX3 & operator=(const MATRIX3 <T> & other) {Set(other); return *this;}
		
		const T & operator[](size_type n) const
		{
			assert(n < 9);
			return data[n];
		}
	
		T & operator[](size_type n)
		{
			assert(n < 9);
			return data[n];
		}
		
		void DebugPrint(std::ostream & out) const
		{
			for (size_type i = 0; i < 9; i++)
			{
				out << i << ". " << data[i] << std::endl;
			}
		}
		
		MATRIX3 <T> Multiply(const MATRIX3 <T> & other) const
		{
			MATRIX3 out;
	
			for (int i = 0, i3 = 0; i < 3; i++,i3+=3)
			{
				for (int j = 0; j < 3; j++)
				{
					out.data[i3+j] = 0;
			
					for (int k = 0, k3 = 0; k < 3; k ++,k3+=3)
						out.data[i3+j] += data[i3+k]*other.data[k3+j];
				}
			}
	
			return out;
		}
		
		void LoadIdentity()
		{
			data[0] = data[4] = data[8] = 1;
			data[1] = data[2] = data[3] = data[5] = data[6] = data[7] = 0;
		}
		
		void Set(const T * newdata)
		{
			//std::memcpy(data,newdata,sizeof(T)*9); //high performance, but portability issues?
			for (int i = 0; i < 9; i++)
				data[i] = newdata[i];
		}
		
		void Set(const MATRIX3 <T> & other) {Set(other.data);}
		
		bool operator==(const MATRIX3 <T> & other) {return Equals(other);}
		
		bool Equals(const MATRIX3 <T> & other)
		{
			//return (memcmp(data,other.data,9*sizeof(T)) == 0); //high performance, but portability issues?
			for (int i = 0; i < 9; i++)
				if (data[i] != other.data[i])
					return false;
			
			return true;
		}
		
		void Scale(T scalar)
		{
			MATRIX3 <T> scalemat;
			scalemat.data[0] = scalemat.data[4] = scalemat.data[8] = scalar;
			scalemat.data[1] = scalemat.data[2] = scalemat.data[3] = scalemat.data[5] = scalemat.data[6] = scalemat.data[7] = 0;
			*this = Multiply(scalemat);
		}
		
		const T * GetArray() const {return data;}
		
		MATHVECTOR <T, 3> Multiply(const MATHVECTOR <T, 3> & v) const
		{
			MATHVECTOR <T, 3> output;
			output.Set(v[0]*data[0] + v[1]*data[3] + v[2]*data[6],
				   v[0]*data[1] + v[1]*data[4] + v[2]*data[7], 
       				   v[0]*data[2] + v[1]*data[5] + v[2]*data[8]);
			return output;
		}
		
		MATRIX3 <T> Inverse() const
		{
			T a = data[0];//
			T b = data[1];
			T c = data[2];
			T d = data[3];
			T e = data[4];//
			T f = data[5];
			T g = data[6];
			T h = data[7];
			T i = data[8];//
			T Div = -c*e*g+b*f*g+c*d*h-a*f*h-b*d*i+a*e*i;
			const T EPSILON(1e-10);
			//std::cout << "Div: " << Div << std::endl;
			//DebugPrint(std::cout);
			assert (myfabs(Div) > EPSILON); //matrix inversion failed
			
			T invdiv = 1.0 / Div;
			
			MATRIX3 m;
			m.data[0] = -f*h+e*i;
			m.data[1] = c*h-b*i;
			m.data[2] = -c*e+b*f;
			m.data[3] = f*g-d*i;
			m.data[4] = -c*g+a*i;
			m.data[5] = c*d-a*f;
			m.data[6] = -e*g+d*h;
			m.data[7] = b*g-a*h;
			m.data[8] = -b*d+a*e;
			
			for (int i = 0; i < 9; i++)
				m.data[i] *= invdiv;
			
			return m;
		}
		
		MATRIX3 <T> Transpose() const
		{
			//0 1 2
			//3 4 5
			//6 7 8
			
			MATRIX3 <T> out;
			out[0] = data[0];
			out[1] = data[3];
			out[2] = data[6];
			out[3] = data[1];
			out[4] = data[4];
			out[5] = data[7];
			out[6] = data[2];
			out[7] = data[5];
			out[8] = data[8];
			
			return out;
		}
};

#endif

