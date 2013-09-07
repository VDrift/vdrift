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

#ifndef _MATRIX4_H
#define _MATRIX4_H

#include "mathvector.h"

#include <iostream>
#include <cstring>
#include <cmath>
#include <cassert>

template <typename T>
class Matrix4
{
	private:
		typedef size_t size_type;
		T data[16];

	public:
		Matrix4() {LoadIdentity();}
		Matrix4(const Matrix4 <T> & other) {Set(other);}
		Matrix4 & operator=(const Matrix4 <T> & other) {Set(other); return *this;}

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
			for (size_type row = 0; row < 4; row++)
			{
				for (size_type col = 0; col < 4; col++)
				{
					out << data[col*4+row] << "\t";
				}
				out << std::endl;
			}
			out << std::endl;
		}

		// this is actually other * this, not this * other
		Matrix4 <T> Multiply(const Matrix4 <T> & other) const
		{
			Matrix4 out;

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

		/// multiply the vector by this matrix and store the result in the vector
		/// the vector must be of length 4 but its bounds are not enforced, so
		/// be careful
		template <typename U>
		void MultiplyVector4(U * vector) const
		{
			U in[4];
			for (int i = 0; i < 4; i++)
			{
				in[i] = vector[i];
				vector[i] = 0;
			}

			for (int r = 0; r < 4; r++)
			{
				for (int i = 0; i < 4; i++)
				{
					vector[r] += in[i]*data[i*4+r];
				}
			}
		}

		void LoadIdentity()
		{
			data[0] = data[5] = data[10] = data[15] = 1;
			data[1] = data[2] = data[3] = data[4] = data[6] = data[7] = data[8] = data[9] = data[11] = data[12] = data[13] = data[14] = 0;
		}

		inline void Set(const T * newdata)
		{
			std::memcpy(data,newdata,sizeof(T)*16);
		}

		template <typename T2>
		void Set(const T2 * newdata)
		{
			for (int i = 0; i < 16; i++)
				data[i] = newdata[i];
		}

		inline void Set(const Matrix4 <T> & other) {Set(other.data);}

		void Translate(const float tx, const float ty, const float tz)
		{
			data[12] += tx;
			data[13] += ty;
			data[14] += tz;
		}

		bool operator==(const Matrix4 <T> & other) {return Equals(other);}
		bool operator!=(const Matrix4 <T> & other) {return !Equals(other);}

		bool Equals(const Matrix4 <T> & other)
		{
			return (memcmp(data,other.data,16*sizeof(T)) == 0); //high performance, but portability issues?
			/*for (int i = 0; i < 16; i++)
				if (data[i] != other.data[i])
					return false;

			return true;*/
		}

		void TransformVectorIn(float & x, float & y, float & z) const
		{
			x -= data[12];
			y -= data[13];
			z -= data[14];

			float outx = x * data[0] + y * data[1] + z * data[2];
			float outy = x * data[4] + y * data[5] + z * data[6];
			float outz = x * data[8] + y * data[9] + z * data[10];

			x = outx;
			y = outy;
			z = outz;
		}

		void TransformVectorOut(float & x, float & y, float & z) const
		{
			float outx = x * data[0] + y * data[4] + z * data[8];
			float outy = x * data[1] + y * data[5] + z * data[9];
			float outz = x * data[2] + y * data[6] + z * data[10];

			x = outx + data[12];
			y = outy + data[13];
			z = outz + data[14];
		}

		void Scale(T scalar)
		{
			Matrix4 <T> scalemat;
			scalemat.data[15] = 1;
			scalemat.data[0] = scalemat.data[5] = scalemat.data[10] = scalar;
			scalemat.data[1] = scalemat.data[2] = scalemat.data[3] = scalemat.data[4] = scalemat.data[6] = scalemat.data[7] = scalemat.data[8] = scalemat.data[9] = scalemat.data[11] = scalemat.data[12] = scalemat.data[13] = scalemat.data[14] = 0;
			*this = Multiply(scalemat);
		}

		const T * GetArray() const {return data;}
		T * GetArray() {return data;}

		void Perspective(T fovy, T aspect, T znear, T zfar)
		{
			const T pi = 3.14159265;
			T f = 1.0 / tan(0.5 * fovy * pi / 180.0);
			data[0] = f / aspect; data[1] = 0; data[2] = 0; data[3] = 0;
			data[4] = 0; data[5] = f; data[6] = 0; data[7] = 0;
			data[8] = 0; data[9] = 0; data[10] = (zfar + znear) / (znear - zfar); data[11] = -1;
			data[12] = 0; data[13] = 0; data[14] = 2 * zfar * znear / (znear - zfar); data[15] = 0;
		}

		void InvPerspective(T fovy, T aspect, T znear, T zfar)
		{
			const T pi = 3.14159265;
			T f = 1.0 / tan(0.5 * fovy * pi / 180.0);
			data[0] = aspect / f; data[1] = 0; data[2] = 0; data[3] = 0;
			data[4] = 0; data[5] = 1 / f; data[6] = 0; data[7] = 0;
			data[8] = 0; data[9] = 0; data[10] = 0; data[11] = (znear - zfar) / (2 * zfar * znear);
			data[12] = 0; data[13] = 0; data[14] = -1; data[15] = (zfar + znear) / (2 * zfar * znear);
		}

		Matrix4<T> Inverse() const
		{
			Matrix4<T> Inv;

			T A0 = data[ 0]*data[ 5] - data[ 1]*data[ 4];
			T A1 = data[ 0]*data[ 6] - data[ 2]*data[ 4];
			T A2 = data[ 0]*data[ 7] - data[ 3]*data[ 4];
			T A3 = data[ 1]*data[ 6] - data[ 2]*data[ 5];
			T A4 = data[ 1]*data[ 7] - data[ 3]*data[ 5];
			T A5 = data[ 2]*data[ 7] - data[ 3]*data[ 6];
			T B0 = data[ 8]*data[13] - data[ 9]*data[12];
			T B1 = data[ 8]*data[14] - data[10]*data[12];
			T B2 = data[ 8]*data[15] - data[11]*data[12];
			T B3 = data[ 9]*data[14] - data[10]*data[13];
			T B4 = data[ 9]*data[15] - data[11]*data[13];
			T B5 = data[10]*data[15] - data[11]*data[14];

			T Det = A0*B5 - A1*B4 + A2*B3 + A3*B2 - A4*B1 + A5*B0;
			assert (fabs(Det) > 1e-10); //matrix inversion failed

			Inv[ 0] = + data[ 5]*B5 - data[ 6]*B4 + data[ 7]*B3;
			Inv[ 4] = - data[ 4]*B5 + data[ 6]*B2 - data[ 7]*B1;
			Inv[ 8] = + data[ 4]*B4 - data[ 5]*B2 + data[ 7]*B0;
			Inv[12] = - data[ 4]*B3 + data[ 5]*B1 - data[ 6]*B0;
			Inv[ 1] = - data[ 1]*B5 + data[ 2]*B4 - data[ 3]*B3;
			Inv[ 5] = + data[ 0]*B5 - data[ 2]*B2 + data[ 3]*B1;
			Inv[ 9] = - data[ 0]*B4 + data[ 1]*B2 - data[ 3]*B0;
			Inv[13] = + data[ 0]*B3 - data[ 1]*B1 + data[ 2]*B0;
			Inv[ 2] = + data[13]*A5 - data[14]*A4 + data[15]*A3;
			Inv[ 6] = - data[12]*A5 + data[14]*A2 - data[15]*A1;
			Inv[10] = + data[12]*A4 - data[13]*A2 + data[15]*A0;
			Inv[14] = - data[12]*A3 + data[13]*A1 - data[14]*A0;
			Inv[ 3] = - data[ 9]*A5 + data[10]*A4 - data[11]*A3;
			Inv[ 7] = + data[ 8]*A5 - data[10]*A2 + data[11]*A1;
			Inv[11] = - data[ 8]*A4 + data[ 9]*A2 - data[11]*A0;
			Inv[15] = + data[ 8]*A3 - data[ 9]*A1 + data[10]*A0;

			T InvDet = 1.0 / Det;
			for (int i = 0; i < 16; i++) Inv[i] *= InvDet;

			return Inv;
		}

		void SetFrustum(T left, T right, T bottom, T top, T near, T far)
		{
			T A = (right+left)/(right-left);
			T B = (top+bottom)/(top-bottom);
			T C = (far+near)/(near-far);
			T D = 2.0*far*near/(near-far);

			data[0] = 2.0*near/(right-left);
			data[1] = 0.0;
			data[2] = 0.0;
			data[3] = 0.0;

			data[4] = 0.0;
			data[5] = 2.0*near/(top-bottom);
			data[6] = 0.0;
			data[7] = 0.0;

			data[8] = A;
			data[9] = B;
			data[10]= C;
			data[11]= -1.0;

			data[12]= 0.0;
			data[13]= 0.0;
			data[14]= D;
			data[15]= 0.0;
		}

		void SetPerspective(T fovy, T aspect, T zNear, T zFar)
		{
			T ymax = zNear * tan(fovy * M_PI / 360.0);
			T ymin = -ymax;

			T xmin = ymin * aspect;
			T xmax = ymax * aspect;

			SetFrustum(xmin, xmax, ymin, ymax, zNear, zFar);
		}

		void SetOrthographic(T left, T right, T bottom, T top, T near, T far)
		{
			T tx = -(right+left)/(right-left);
			T ty = -(top+bottom)/(top-bottom);
			T tz = -(far+near)/(far-near);

			data[0] = 2.0/(right-left);
			data[1] = 0.0;
			data[2] = 0.0;
			data[3] = 0.0;

			data[4] = 0.0;
			data[5] = 2.0/(top-bottom);
			data[6] = 0.0;
			data[7] = 0.0;

			data[8] = 0.0;
			data[9] = 0.0;
			data[10]= -2.0/(far-near);
			data[11]= 0.0;

			data[12]= tx;
			data[13]= ty;
			data[14]= tz;
			data[15]= 1.0;
		}

		void OrthoNormalize()
		{
			// ensure orthonormal basis vectors

			// extract and normalize the x basis
			MathVector <T, 3> xBasis;
			xBasis.Set(&data[0]);
			xBasis = xBasis.Normalize();

			// extract and normalize the y basis
			MathVector <T, 3> yBasis;
			yBasis.Set(&data[4]);
			yBasis = yBasis.Normalize();

			// generate an orthonormal z basis
			MathVector <T, 3> zBasis;
			zBasis = xBasis.cross(yBasis);

			// save our orthonormal basis vectors back to the matrix
			std::memcpy(&data[0], &xBasis[0], 3*sizeof(T));
			std::memcpy(&data[4], &yBasis[0], 3*sizeof(T));
			std::memcpy(&data[8], &zBasis[0], 3*sizeof(T));
		}

		void ForceAffine()
		{
			data[3] = data[7] = data[11] = 0.0;
			data[15] = 1.0;
		}

		/// given an axis and a normalized angle, set the rotation portion of this matrix
		void SetRotation(T angle, T x, T y, T z)
		{
			T c = cos(angle);
			T s = sin(angle);

			data[0] = x*x+(1-x*x)*c;
			data[4] = x*y*(1-c)+z*s;
			data[8] = x*z*(1-c)-y*s;

			data[1] = x*y*(1-c)-z*s;
			data[5] = y*y+(1-y*y)*c;
			data[9] = y*z*(1-c)+x*s;

			data[2] = x*z*(1-c)+y*s;
			data[6] = y*z*(1-c)-x*s;
			data[10]= z*z+(1-z*z)*c;
		}
};

template <typename T>
std::ostream & operator << (std::ostream &os, const Matrix4 <T> & m)
{
	m.DebugPrint(os);
	return os;
}

typedef Matrix4 <float> Mat4;

#endif
