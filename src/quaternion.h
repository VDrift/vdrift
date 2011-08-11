#ifndef _QUATERNION_H
#define _QUATERNION_H

#include "mathvector.h"
#include "joeserialize.h"

#include <vector>
#include <cassert>
#include <cmath>
#include <iostream>

template <typename T>
class QUATERNION
{
friend class joeserialize::Serializer;
private:
	T v[4]; //x y z w

public:
	typedef size_t size_type;

	QUATERNION()
	{
		LoadIdentity();
	}

	QUATERNION(const T & nx, const T & ny, const T & nz, const T & nw)
	{
		v[0] = nx;
		v[1] = ny;
		v[2] = nz;
		v[3] = nw;
	}

	///create quaternion from Euler angles ZYX convention
	QUATERNION(const T & x, const T & y, const T & z)
	{
		SetEulerZYX(x, y, z);
	}

	QUATERNION(const QUATERNION <T> & other)
	{
		*this = other;
	}

	///load the [1,(0,0,0)] quaternion
	void LoadIdentity()
	{
		v[3] = 1;
		v[0] = v[1] = v[2] = 0;
	}

	const T & operator[](size_type n) const
	{
		assert(n < 4);
		return v[n];
	}

	T & operator[](size_type n)
	{
		assert(n < 4);
		return v[n];
	}

	const T & x() const {return v[0];}
	const T & y() const {return v[1];}
	const T & z() const {return v[2];}
	const T & w() const {return v[3];}

	T & x() {return v[0];}
	T & y() {return v[1];}
	T & z() {return v[2];}
	T & w() {return v[3];}

	template <typename T2>
	const QUATERNION <T> & operator = (const QUATERNION <T2> & other)
	{
		for (size_type i = 0; i < 4; ++i)
			v[i] = other[i];

		return *this;
	}

	///return the magnitude of the quaternion
	const T Magnitude() const
	{
		return sqrt(v[3]*v[3]+v[0]*v[0]+v[1]*v[1]+v[2]*v[2]);
	}

	///normalize this quaternion
	void Normalize()
	{
		T len = Magnitude();
		for (size_t i = 0; i < 4; i++)
			v[i] /= len;
	}

	///set the given matrix to a matrix representation of this quaternion.
	/// no array bound checking is done.
	/// the matrix type can be any type that is accessible with [].
	template <typename T2>
	void GetMatrix4(T2 & destmat) const
	{
		T xx = v[0]*v[0];
		T xy = v[0]*v[1];
		T xz = v[0]*v[2];
		T xw = v[0]*v[3];

		T yy = v[1]*v[1];
		T yz = v[1]*v[2];
		T yw = v[1]*v[3];

		T zz = v[2]*v[2];
		T zw = v[2]*v[3];

		destmat[0] = 1.0 - 2.0*(yy+zz);
		destmat[1] = 2.0*(xy+zw);
		destmat[2] = 2.0*(xz-yw);
		destmat[3] = 0;

		destmat[4] = 2.0*(xy-zw);
		destmat[5] = 1.0-2.0*(xx+zz);
		destmat[6] = 2.0*(yz+xw);
		destmat[7] = 0;

		destmat[8] = 2.0*(xz+yw);
		destmat[9] = 2.0*(yz-xw);
		destmat[10] = 1.0-2.0*(xx+yy);
		destmat[11] = 0;

		destmat[12] = 0;
		destmat[13] = 0;
		destmat[14] = 0;
		destmat[15] = 1;
	}

	///set the given matrix to a matrix representation of this quaternion.
	/// no array bound checking is done.
	/// the matrix type can be any type that is accessible with [].
	template <typename T2>
	void GetMatrix3(T2 & destmat) const
	{
		T xx = v[0]*v[0];
		T xy = v[0]*v[1];
		T xz = v[0]*v[2];
		T xw = v[0]*v[3];

		T yy = v[1]*v[1];
		T yz = v[1]*v[2];
		T yw = v[1]*v[3];

		T zz = v[2]*v[2];
		T zw = v[2]*v[3];

		destmat[0] = 1.0 - 2.0*(yy+zz);
		destmat[1] = 2.0*(xy+zw);
		destmat[2] = 2.0*(xz-yw);

		destmat[3] = 2.0*(xy-zw);
		destmat[4] = 1.0-2.0*(xx+zz);
		destmat[5] = 2.0*(yz+xw);

		destmat[6] = 2.0*(xz+yw);
		destmat[7] = 2.0*(yz-xw);
		destmat[8] = 1.0-2.0*(xx+yy);
	}

	MATHVECTOR<T, 3> AxisX() const
	{
		T xy = v[0]*v[1];
		T xz = v[0]*v[2];
		T yy = v[1]*v[1];
		T yw = v[1]*v[3];
		T zz = v[2]*v[2];
		T zw = v[2]*v[3];
		return MATHVECTOR<T, 3>(1.0-2.0*(yy+zz), 2.0*(xy+zw), 2.0*(xz-yw));
	}

	MATHVECTOR<T, 3> AxisY() const
	{
		T xx = v[0]*v[0];
		T xy = v[0]*v[1];
		T xw = v[0]*v[3];
		T yz = v[1]*v[2];
		T zz = v[2]*v[2];
		T zw = v[2]*v[3];
		return MATHVECTOR<T, 3>(2.0*(xy-zw), 1.0-2.0*(xx+zz), 2.0*(yz+xw));
	}

	MATHVECTOR<T, 3> AxisZ() const
	{
		T xx = v[0]*v[0];
		T xz = v[0]*v[2];
		T xw = v[0]*v[3];
		T yy = v[1]*v[1];
		T yz = v[1]*v[2];
		T yw = v[1]*v[3];
		return MATHVECTOR<T, 3>(2.0*(xz+yw), 2.0*(yz-xw), 1.0-2.0*(xx+yy));
	}

	///has the potential to return a un-normalized quaternion
	QUATERNION <T> operator*(const QUATERNION <T> & quat2 ) const
	{
		/*QUATERNION output(v[3]*quat2.v[0] + v[0]*quat2.v[3] + v[1]*quat2.v[2] - v[2]*quat2.v[1],
			v[3]*quat2.v[1] + v[1]*quat2.v[3] + v[2]*quat2.v[0] - v[0]*quat2.v[2],
			v[3]*quat2.v[2] + v[2]*quat2.v[3] + v[0]*quat2.v[1] - v[1]*quat2.v[0],
   			v[3]*quat2.v[3] - v[0]*quat2.v[0] - v[1]*quat2.v[1] - v[2]*quat2.v[2]);

		//output.Normalize();
		return output;*/

		T A, B, C, D, E, F, G, H;

		A = (v[3] + v[0])*(quat2.v[3] + quat2.v[0]);
		B = (v[2] - v[1])*(quat2.v[1] - quat2.v[2]);
		C = (v[3] - v[0])*(quat2.v[1] + quat2.v[2]);
		D = (v[1] + v[2])*(quat2.v[3] - quat2.v[0]);
		E = (v[0] + v[2])*(quat2.v[0] + quat2.v[1]);
		F = (v[0] - v[2])*(quat2.v[0] - quat2.v[1]);
		G = (v[3] + v[1])*(quat2.v[3] - quat2.v[2]);
		H = (v[3] - v[1])*(quat2.v[3] + quat2.v[2]);


		QUATERNION output(A - (E + F + G + H)*0.5,
			C + (E - F + G - H)*0.5,
			D + (E - F - G + H)*0.5,
			B + (-E - F + G + H)*0.5);
		return output;
	}

	///has the potential to return a un-normalized quaternion
	QUATERNION <T> operator*(const T & scalar ) const
	{
		QUATERNION output(v[0]*scalar, v[1]*scalar, v[2]*scalar, v[3]*scalar);

		//output.Normalize();
		return output;
	}

	///has the potential to return a un-normalized quaternion
	QUATERNION <T> operator+(const QUATERNION <T> & quat2) const
	{
		QUATERNION output(v[0]+quat2.v[0], v[1]+quat2.v[1], v[2]+quat2.v[2], v[3]+quat2.v[3]);

		//output.Normalize();
		return output;
	}

	template <typename T2>
	bool operator== (const QUATERNION <T2> & other) const
	{
		bool same(true);

		for (size_type i = 0; i < 4; i++)
		{
			same = same && (v[i] == other.v[i]);
		}

		return same;
	}

	template <typename T2>
	bool operator!= (const QUATERNION <T2> & other) const
	{
		return !(*this == other);
	}

	///returns the conjugate
	QUATERNION <T> operator-() const
	{
		QUATERNION qtemp;
		qtemp.v[3] = v[3];
		for (size_type i = 0; i < 3; i++)
		{
			qtemp.v[i] = -v[i];
		}
		return qtemp;
	}

	///rotate the quaternion around the given axis by the given amount
	/// a is in radians.  the axis is assumed to be a unit vector
	void Rotate(const T & a, const T & ax, const T & ay, const T & az)
	{
		QUATERNION output;
		output.SetAxisAngle(a, ax, ay, az);
		(*this) = output * (*this);
		Normalize();
	}

	void Rotate(const T & a, const MATHVECTOR<T, 3> & axis)
	{
		QUATERNION output;
		output.SetAxisAngle(a, axis[0], axis[1], axis[2]);
		(*this) = output * (*this);
		Normalize();
	}

	///set the quaternion to rotation a around the given axis
	/// a is in radians.  the axis is assumed to be a unit vector
	void SetAxisAngle(const T & a, const T & ax, const T & ay, const T & az)
	{
		T sina2 = sin(a/2);

		v[3] = cos(a/2);
		v[0] = ax * sina2;
		v[1] = ay * sina2;
		v[2] = az * sina2;
	}

	///set the quaternion using Euler angles
	void SetEulerZYX(const T & x, const T & y, const T & z)
	{
		T cosx2 = cos(x/2);
		T cosy2 = cos(y/2);
		T cosz2 = cos(z/2);
		T sinx2 = sin(x/2);
		T siny2 = sin(y/2);
		T sinz2 = sin(z/2);
		v[0] = sinx2 * cosy2 * cosz2 - cosx2 * siny2 * sinz2;
		v[1] = cosx2 * siny2 * cosz2 + sinx2 * cosy2 * sinz2;
		v[2] = cosx2 * cosy2 * sinz2 - sinx2 * siny2 * cosz2;
		v[3] = cosx2 * cosy2 * cosz2 + sinx2 * siny2 * sinz2;
	}

	void GetEulerZYX(T & x, T & y, T & z)
	{
		x = atan2(2 * (v[1] * v[2] + v[0] * v[3]), -v[0] * v[0] - v[1] * v[1] + v[2] * v[2] + v[3] * v[3]);
		y = asin(-2 * (v[0] * v[2] - v[1] * v[3]));
		z = atan2(2 * (v[0] * v[1] + v[2] * v[3]),  v[0] * v[0] - v[1] * v[1] - v[2] * v[2] + v[3] * v[3]);
	}

	///rotate a vector (accessible with []) by this quaternion
	/// note that the output is saved back to the input vec variable
	template <typename T2>
	void RotateVector(T2 & vec) const
	{
		QUATERNION dirconj = -(*this);
		QUATERNION qtemp;
		qtemp.v[3] = 0;
		for (size_t i = 0; i < 3; i++)
			qtemp.v[i] = vec[i];

		QUATERNION qout = (*this) * qtemp * dirconj;

		for (size_t i = 0; i < 3; i++)
			vec[i] = qout.v[i];
	}

	///get the scalar angle (in radians) between two quaternions
	const T GetAngleBetween(const QUATERNION <T> & quat2) const
	{
		//establish a forward vector
		T forward[3];
		forward[0] = 0;
		forward[1] = 0;
		forward[2] = 1;

		//create vectors for quats
		T vec1[3];
		T vec2[3];
		for (size_t i = 0; i < 3; i++)
			vec1[i] = vec2[i] = forward[i];

		RotateVector(vec1);
		quat2.RotateVector(vec2);

		//return the angle between the vectors
		T dotprod(0);
		for (size_t i = 0; i < 3; i++)
			dotprod += vec1[i]*vec2[i];
		return acos(dotprod);
	}

	///interpolate between this quaternion and another by scalar amount t [0,1] and return the result
	QUATERNION <T> QuatSlerp (const QUATERNION <T> & quat2, const T & t) const
	{
		T to1[4];
		T omega, cosom, sinom, scale0, scale1;

		//calc cosine
		cosom = v[0] * quat2.v[0] + v[1] * quat2.v[1] + v[2] * quat2.v[2]
			+ v[3] * quat2.v[3];

		//adjust signs (if necessary)
		if (cosom < 0.0)
		{
			cosom = -cosom;
			to1[0] = -quat2.v[0];
			to1[1] = -quat2.v[1];
			to1[2] = -quat2.v[2];
			to1[3] = -quat2.v[3];
		}
		else
		{
			to1[0] = quat2.v[0];
			to1[1] = quat2.v[1];
			to1[2] = quat2.v[2];
			to1[3] = quat2.v[3];
		}

		const T DELTA(0.00001);

		//calculate coefficients
		if (1.0 - cosom > DELTA)
		{
			//standard case (slerp)
			omega = acos(cosom);
			sinom = sin(omega);
			scale0 = sin((1.0 - t) * omega) / sinom;
			scale1 = sin(t * omega) / sinom;
		}
		else
		{
			//"from" and "to" quaternions are very close
			//... so we can do a linear interpolation
			scale0 = 1.0 - t;
			scale1 = t;
		}

		//calculate final values
		QUATERNION <T> qout;
		qout.v[0] = scale0 * v[0] + scale1 * to1[0];
		qout.v[1] = scale0 * v[1] + scale1 * to1[1];
		qout.v[2] = scale0 * v[2] + scale1 * to1[2];
		qout.v[3] = scale0 * v[3] + scale1 * to1[3];
		qout.Normalize();
		return qout;
	}

	bool Serialize(joeserialize::Serializer & s)
	{
		if (!s.Serialize("x",v[0])) return false;
		if (!s.Serialize("y",v[1])) return false;
		if (!s.Serialize("z",v[2])) return false;
		if (!s.Serialize("w",v[3])) return false;
		return true;
	}

	/*///assuming the eye is at the given coordinates, set the orientation to look at center
	void LookAt(T eyex,
			T eyey,
			T eyez,
			T centerx,
			T centery,
			T centerz,
			T upx,
			T upy,
			T upz)
	{
		MATHVECTOR <T,3> forward(centerx-eyex, centery-eyey, centerz-eyez);
		MATHVECTOR <T,3> up(upx, upy, upz);

		forward = forward.Normalize();
		MATHVECTOR <T,3> side = (forward.cross(up)).Normalize();
		up = side.cross(forward);

		T m[16];
		for (int i = 0; i < 16; i++)
			m[i] = 0;
		m[15] = 1;

		// 0  1  2  3
		// 4  5  6  7
		// 8  9 10 11
		//12 13 14 15

		m[0] = side[0];
		m[4] = side[1];
		m[8] = side[2];

		m[1] = up[0];
		m[5] = up[1];
		m[9] = up[2];

		m[2] = -forward[0];
		m[6] = -forward[1];
		m[10] = -forward[2];

		SetMatrix4(m);
	}

	///set the orientation to the orientation specified in the given 4x4 matrix
	template <typename T2>
	void SetMatrix4(T2 mat)
	{
		T S;

		T t = 1 + mat[0] + mat[5] + mat[10];
		if ( t > 0.00000001 )
		{
			S = sqrt(t) * 2;
			v[0] = ( mat[9] - mat[6] ) / S;
			v[1] = ( mat[2] - mat[8] ) / S;
			v[2] = ( mat[4] - mat[1] ) / S;
			v[3] = 0.25 * S;
		}
		else
		{
		if ( mat[0] > mat[5] && mat[0] > mat[10] )  {       // Column 0:
			S  = sqrt( 1.0 + mat[0] - mat[5] - mat[10] ) * 2;
			v[0] = 0.25 * S;
			v[1] = (mat[4] + mat[1] ) / S;
			v[2] = (mat[2] + mat[8] ) / S;
			v[3] = (mat[9] - mat[6] ) / S;
		} else if ( mat[5] > mat[10] ) {                    // Column 1:
			S  = sqrt( 1.0 + mat[5] - mat[0] - mat[10] ) * 2;
			v[0] = (mat[4] + mat[1] ) / S;
			v[1] = 0.25 * S;
			v[2] = (mat[9] + mat[6] ) / S;
			v[3] = (mat[2] - mat[8] ) / S;
		} else {                                            // Column 2:
			S  = sqrt( 1.0 + mat[10] - mat[0] - mat[5] ) * 2;
			v[0] = (mat[2] + mat[8] ) / S;
			v[1] = (mat[9] + mat[6] ) / S;
			v[2] = 0.25 * S;
			v[3] = (mat[4] - mat[1] ) / S;
		}
		}

		Normalize();
		// *this = -*this;
	}*/
};

template <typename T>
std::ostream & operator << (std::ostream &os, const QUATERNION <T> & v)
{
	os << "x=" << v[0] << ", y=" << v[1] << ", z=" << v[2] << ", w=" << v[3];
	return os;
}

#endif
