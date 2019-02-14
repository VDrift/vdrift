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

#include "bezier.h"
#include "unittest.h"

#include <cmath>

std::ostream & operator << (std::ostream &os, const Bezier & b)
{
	os << "====" << std::endl;
	for (int x = 0; x < 4; x++)
	{
		for (int y = 0; y < 4; y++)
			os << b[y*4+x] << std::endl;
		os << "----" << std::endl;
	}
	os << "====" << std::endl;
	return os;
}

Aabb <float> Bezier::GetAABB() const
{
	float maxv[3] = {-1E38, -1E38, -1E38};
	float minv[3] = {+1E38, +1E38, +1E38};
	for (int x = 0; x < 4; x++)
	{
		for (int y = 0; y < 4; y++)
		{
			const Vec3 temp = points[x][y];
			for (int n = 0; n < 3; n++)
			{
				if (temp[n] > maxv[n])
					maxv[n] = temp[n];
				if (temp[n] < minv[n])
					minv[n] = temp[n];
			}
		}
	}

	Vec3 bboxmin(minv[0], minv[1], minv[2]);
	Vec3 bboxmax(maxv[0], maxv[1], maxv[2]);
	Aabb <float> box(bboxmin, bboxmax);
	return box;
}

void Bezier::SetFromCorners(const Vec3 & fl, const Vec3 & fr, const Vec3 & bl, const Vec3 & br)
{
	// assign corners
	points[0][0] = fl;
	points[0][3] = fr;
	points[3][0] = bl;
	points[3][3] = br;

	// calculate intermediate front and back points
	Vec3 t = (fr - fl) * (1/3.f);
	points[0][1] = fl + t;
	points[0][2] = fr - t;

	t = (br - bl) * (1/3.f);
	points[3][1] = bl + t;
	points[3][2] = br - t;

	// calculate intermediate left and right points
	for (int i = 0; i < 4; i++)
	{
		t = (points[3][i] - points[0][i]) * (1/3.f);
		points[1][i] = points[0][i] + t;
		points[2][i] = points[3][i] - t;
	}

	//CheckForProblems();
}

// Basis functions
inline float B0(float t) { return t*t*t; }
inline float B1(float t) { return 3*t*t*(1-t); }
inline float B2(float t) { return 3*t*(1-t)*(1-t); }
inline float B3(float t) { return (1-t)*(1-t)*(1-t); }

inline float T0(float t) { return 3*t*t; }
inline float T1(float t) { return 6*t-9*t*t; }
inline float T2(float t) { return 3-12*t+9*t*t; }
inline float T3(float t) { return -3*(1-t)*(1-t); }

inline float N0(float t) { return 6*t; }
inline float N1(float t) { return 6-18*t; }
inline float N2(float t) { return 18*t-12; }
inline float N3(float t) { return 6-6*t; }

// Shortest cubic spline through 4 on-curve points(chord approximation)
void Bezier::FitSpline(Vec3 p[])
{
	// use chord length for shortest(best) cubic spline approximation
	float c3 = (p[1] - p[0]).Magnitude();
	float c2 = (p[2] - p[1]).Magnitude();
	float c1 = (p[3] - p[2]).Magnitude();

	// cases where p[1] is close to p[2] might lead to instabilities(need some heuristic)
	if (50 * c2 < c1 + c3)
	{
		p[1] = p[0] + (p[1] - p[0]) * 0.98f;
		p[2] = p[3] + (p[2] - p[3]) * 0.98f;
		c3 = c3 * 0.98f;
		c2 = (p[2] - p[1]).Magnitude();
		c1 = c1 * 0.98f;
	}

	float t1 = c1 / (c1 + c2 + c3);
	float t2 = (c1 + c2) / (c1 + c2 + c3);

	// Solve M * x = y
	float m00 = B1(t1);
	float m01 = B2(t1);
	float m10 = B1(t2);
	float m11 = B2(t2);

	float detM = m00 * m11 - m01 * m10;
	if (std::abs(detM) > 1E-3f)
	{
		// y = p - p0 * B0(t) - p3 * B3(t)
		Vec3 y1 = p[1] - p[0] * B0(t1) - p[3] * B3(t1);
		Vec3 y2 = p[2] - p[0] * B0(t2) - p[3] * B3(t2);

		// Minv
		float s = 1 / detM;
		float n00 = s * m11;
		float n01 = -s * m01;
		float n10 = -s * m10;
		float n11 = s * m00;

		// x = Minv * y
		Vec3 x1 = y1 * n00 + y2 * n01;
		Vec3 x2 = y1 * n10 + y2 * n11;

		p[1] = x1;
		p[2] = x2;
	}
}

// adjust spline to go through p1, p2 for p1 == p2
void Bezier::FitMidPoint(Vec3 p[])
{
	p[0].Set(-150, 0, 0);
	p[1].Set(100, 60, 0);
	p[2].Set(100, 60, 0);
	p[3].Set(200, 0, 0);

	Vec3 d3 = p[0] - p[1];
	Vec3 d2 = p[1] - p[2];
	Vec3 d1 = p[2] - p[3];
	float c3 = d3.Magnitude();
	float c2 = d2.Magnitude();
	float c1 = d1.Magnitude();

	if (100 * c2 > c1 + c3) return;

	// chord length approximation(doesn't work that good)
	// pm for p = p1 = p2
	// p = p0 * B0(t) + pm * (B1(t) + B2(t)) + p3 * B3(t)
	//float t = c1 / (c1 + c3);
	//Vec3 pm = (p[1] - p[0] * B0(t) - p[3] * B3(t)) / (B1(t) + B2(t));

	// extrude along midpoint normal
	// nm = p0 * N0 + pm * (N1 + N2) + p3 * N3 with  pm = p1 + nm * s
	Vec3 nm = (d2 - d3).Normalize();
	Vec3 y = nm * (1/6.f) + p[1] - p[3];
	Vec3 a = p[0] - p[3];
	Vec3 b = -nm;

	// y = a * t + b * s
	// y = M * x with x = (t, s) and M = (a, b)
	// x = Minv * y
	float s;//, t;
	Vec3 det = a.cross(b);
	if (det[0] != 0)
	{
		float m00 = a[1];
		//float m01 = b[1];
		float m10 = a[2];
		//float m11 = b[2];

		// Minv
		float d = 1 / det[0];
		//float n00 = d * m11;
		//float n01 = -d * m01;
		float n10 = -d * m10;
		float n11 = d * m00;

		// x = Minv * y
		//t = y[1] * n00 + y[2] * n01; // need s only
		s = y[1] * n10 + y[2] * n11;
	}
	else if (det[1] != 0)
	{
		//m00 = a[2]; m01 = b[2]; m10 = a[0]; m11 = b[0];
		s = 1 / det[1] * (-y[2] * a[0] + y[0] * a[2]);
	}
	else if (det[2] != 0)
	{
		//m00 = a[0]; m01 = b[0]; m10 = a[1]; m11 = b[1];
		s = 1 / det[2] * (-y[0] * a[1] + y[1] * a[0]);
	}
	else
	{
		return;	// failure
	}

	Vec3 pm = p[1] + nm * s;

	p[1] = pm;
	p[2] = pm;
}

void Bezier::Reverse()
{
	Vec3 oldpoints[4][4];

	for (int n = 0; n < 4; n++)
		for (int i = 0; i < 4; i++)
			oldpoints[n][i] = points[n][i];

	for (int n = 0; n < 4; n++)
		for (int i = 0; i < 4; i++)
			points[n][i] = oldpoints[3-n][3-i];
}

Vec3 Bezier::Bernstein(float u, const Vec3 p[]) const
{
	float oneminusu = 1-u;
	Vec3 a = p[0]*(u*u*u);
	Vec3 b = p[1]*(3*u*u*oneminusu);
	Vec3 c = p[2]*(3*u*oneminusu*oneminusu);
	Vec3 d = p[3]*(oneminusu*oneminusu*oneminusu);
	return a+b+c+d;
}

Vec3 Bezier::BernsteinTangent(float u, const Vec3 p[]) const
{
	float oneminusu = 1-u;
	Vec3 a = (p[1]-p[0])*(3*u*u);
	Vec3 b = (p[2]-p[1])*(3*2*u*oneminusu);
	Vec3 c = (p[3]-p[2])*(3*oneminusu*oneminusu);
	return a+b+c;
}

Vec3 Bezier::SurfCoord(float px, float py) const
{
	//get splines along x axis
	Vec3 temp[4];
	for (int j = 0; j < 4; ++j)
	{
		temp[j] = Bernstein(px, points[j]);
	}

	return Bernstein(py, temp);
}

Vec3 Bezier::SurfNorm(float px, float py) const
{
	Vec3 tempy[4];
	Vec3 tempx[4];
	Vec3 temp2[4];

	//get splines along x axis
	for (int j = 0; j < 4; ++j)
	{
		tempy[j] = Bernstein(px, points[j]);
	}

	//get splines along y axis
	for (int j = 0; j < 4; ++j)
	{
		for (int i = 0; i < 4; ++i)
		{
			temp2[i] = points[i][j];
		}
		tempx[j] = Bernstein(py, temp2);
	}

	Vec3 tx = BernsteinTangent(px, tempx);
	Vec3 ty = BernsteinTangent(py, tempy);
	Vec3 n = -tx.cross(ty).Normalize();

	return n;
}

void Bezier::ReadFrom(std::istream &openfile)
{
	assert(openfile);

	for (int x = 0; x < 4; x++)
	{
		for (int y = 0; y < 4; y++)
		{
			openfile >> points[x][y][0];
			openfile >> points[x][y][1];
			openfile >> points[x][y][2];
		}
		//FitSpline(points[x]);
		//FitMidPoint(points[x]);
	}
}

void Bezier::ReadFromYZX(std::istream &openfile)
{
	assert(openfile);
	for (int x = 0; x < 4; x++)
	{
		for (int y = 0; y < 4; y++)
		{
			openfile >> points[x][y][1];
			openfile >> points[x][y][2];
			openfile >> points[x][y][0];
		}
	}
}

void Bezier::WriteTo(std::ostream &openfile) const
{
	assert(openfile);

	for (int x = 0; x < 4; x++)
	{
		for (int y = 0; y < 4; y++)
		{
			openfile << points[x][y][0] << " ";
			openfile << points[x][y][1] << " ";
			openfile << points[x][y][2] << std::endl;
		}
	}
}

bool Bezier::CollideSubDivQuadSimple(const Vec3 & origin, const Vec3 & direction, Vec3 &outtri) const
{
	Vec3 normal;
	return CollideSubDivQuadSimpleNorm(origin, direction, outtri, normal);
}

bool Bezier::CollideSubDivQuadSimpleNorm(const Vec3 & origin, const Vec3 & direction, Vec3 &outtri, Vec3 & normal) const
{
	bool col = false;
	const int COLLISION_QUAD_DIVS = 6;
	const float areacut = 0.5f;

	float t, u, v;

	float su = 0;
	float sv = 0;

	float umin = 0;
	float umax = 1;
	float vmin = 0;
	float vmax = 1;

	Vec3 ul = points[3][3];
	Vec3 ur = points[3][0];
	Vec3 br = points[0][0];
	Vec3 bl = points[0][3];

	for (int i = 0; i < COLLISION_QUAD_DIVS; i++)
	{
		float tu[2];
		float tv[2];

		//speedup for i == 0
		//if (i != 0)
		{
			tu[0] = umin;
			if (tu[0] < 0)
				tu[0] = 0;
			tu[1] = umax;
			if (tu[1] > 1)
				tu[1] = 1;

			tv[0] = vmin;
			if (tv[0] < 0)
				tv[0] = 0;
			tv[1] = vmax;

			if (tv[1] > 1)
				tv[1] = 1;

			ul = SurfCoord(tu[0], tv[0]);
			ur = SurfCoord(tu[1], tv[0]);
			br = SurfCoord(tu[1], tv[1]);
			bl = SurfCoord(tu[0], tv[1]);
		}

		col = IntersectQuadrilateralF(origin, direction, ul, ur, br, bl, t, u, v);

		if (col)
		{
			//expand quad UV to surface UV
			//su = u * (umax - umin) + umin;
			//sv = v * (vmax - vmin) + vmin;

			su = u * (tu[1] - tu[0]) + tu[0];
			sv = v * (tv[1] - tv[0]) + tv[0];

			//place max and min according to area hit
			vmax = sv + (0.5f*areacut)*(vmax - vmin);
			vmin = sv - (0.5f*areacut)*(vmax - vmin);
			umax = su + (0.5f*areacut)*(umax - umin);
			umin = su - (0.5f*areacut)*(umax - umin);
		}
		else
		{
			outtri = origin;
			return false;
		}
	}

	outtri = SurfCoord(su, sv);
	normal = SurfNorm(su, sv);
	return true;
}

void Bezier::DeCasteljauHalveCurve(Vec3 * points4, Vec3 * left4, Vec3 * right4) const
{
	left4[0] = points4[0];
	left4[1] = (points4[0]+points4[1])*0.5f;
	Vec3 point23 = (points4[1]+points4[2])*0.5f;
	left4[2] = (left4[1]+point23)*0.5;

	right4[3] = points4[3];
	right4[2] = (points4[3]+points4[2])*0.5f;
	right4[1] = (right4[2]+point23)*0.5f;

	left4[3] = right4[0] = (right4[1]+left4[2])*0.5f;
}

bool Bezier::CheckForProblems() const
{
	Vec3 corners[4];
	corners[0] = points[0][0];
	corners[1] = points[0][3];
	corners[2] = points[3][3];
	corners[3] = points[3][0];

	bool problem = false;

	for (int i = 0; i < 4; i++)
	{
		Vec3 leg1(corners[(i+1)%4] - corners[i]);
		Vec3 leg2(corners[(i+2)%4] - corners[i]);
		Vec3 leg3(corners[(i+3)%4] - corners[i]);

		Vec3 dir1 = leg1.cross(leg2);
		Vec3 dir2 = leg1.cross(leg3);
		Vec3 dir3 = leg2.cross(leg3);

		if (dir1.dot(dir2) < -1E-4f)
			problem = true;
		if (dir1.dot(dir3) < -1E-4f)
			problem = true;
		if (dir3.dot(dir2) < -1E-4f)
			problem = true;

		/*if (problem)
		{
			std::cout << *this;
			std::cout << "i: " << i << ", " << (i+1)%4 << ", " << (i+2)%4 << std::endl;
			std::cout << corners[0] << std::endl;
			std::cout << corners[1] << std::endl;
			std::cout << corners[2] << std::endl;
			std::cout << corners[3] << std::endl;
			std::cout << leg1 << std::endl;
			std::cout << leg2 << std::endl;
			std::cout << dir1 << std::endl;
			std::cout << dir1.dot(dir2) << ", " <<dir1.dot(dir3) << ", " << dir3.dot(dir2) <<std::endl;
		}*/
	}

	//if (problem) cout << "Degenerate bezier patch detected" << endl;

	return problem;
}

bool Bezier::IntersectQuadrilateralF(
	const Vec3 & orig, const Vec3 & dir,
	const Vec3 & v_00, const Vec3 & v_10,
	const Vec3 & v_11, const Vec3 & v_01,
	float &t, float &u, float &v) const
{
	const float EPSILON = 1E-6f;

	// Reject rays that are parallel to Q, and rays that intersect the plane
	// of Q either on the left of the line V00V01 or below the line V00V10.
	Vec3 E_01 = v_10 - v_00;
	Vec3 E_03 = v_01 - v_00;
	Vec3 P = dir.cross(E_03);
	float det = E_01.dot(P);

	if (std::abs(det) < EPSILON) return false;

	Vec3 T = orig - v_00;
	float alpha = T.dot(P) / det;

	if (alpha < 0) return false;

	Vec3 Q = T.cross(E_01);
	float beta = dir.dot(Q) / det;

	if (beta < 0) return false;

	if (alpha + beta > 1)
	{
		// Reject rays that that intersect the plane of Q either on
		// the right of the line V11V10 or above the line V11V00.
		Vec3 E_23 = v_01 - v_11;
		Vec3 E_21 = v_10 - v_11;
		Vec3 P_prime = dir.cross(E_21);
		float det_prime = E_23.dot(P_prime);

		if (std::abs(det_prime) < EPSILON) return false;

		Vec3 T_prime = orig - v_11;
		float alpha_prime = T_prime.dot(P_prime) / det_prime;

		if (alpha_prime < 0) return false;

		Vec3 Q_prime = T_prime.cross(E_23);
		float beta_prime = dir.dot(Q_prime) / det_prime;

		if (beta_prime < 0) return false;
	}

	// Compute the ray parameter of the intersection point, and
	// reject the ray if it does not hit Q.
	t = E_03.dot(Q) / det;

	if (t < 0) return false;

	// Compute the barycentric coordinates of the fourth vertex.
	// These do not depend on the ray, and can be precomputed
	// and stored with the quadrilateral.
	float alpha_11, beta_11;
	Vec3 E_02 = v_11 - v_00;
	Vec3 n = E_01.cross(E_03);

	if ((std::abs(n[0]) >= std::abs(n[1]))
		    && (std::abs(n[0]) >= std::abs(n[2])))
	{
		alpha_11 = (E_02[1] * E_03[2] - E_02[2] * E_03[1]) / n[0];
		beta_11 = (E_01[1] * E_02[2] - E_01[2] * E_02[1]) / n[0];
	}
	else if ((std::abs(n[1]) >= std::abs(n[0]))
			 && (std::abs(n[1]) >= std::abs(n[2])))
	{
		alpha_11 = (E_02[2] * E_03[0] - E_02[0] * E_03[2]) / n[1];
		beta_11 = (E_01[2] * E_02[0] - E_01[0] * E_02[2]) / n[1];
	}
	else
	{
		alpha_11 = (E_02[0] * E_03[1] - E_02[1] * E_03[0]) / n[2];
		beta_11 = (E_01[0] * E_02[1] - E_01[1] * E_02[0]) / n[2];
	}

	// Compute the bilinear coordinates of the intersection point.
	if (std::abs(alpha_11 - 1) < EPSILON)
	{
		// Q is a trapezium.
		u = alpha;
		if (std::abs(beta_11 - 1) < EPSILON) v = beta; // Q is a parallelogram.
		else v = beta / (u * (beta_11 - 1) + 1); // Q is a trapezium.
	}
	else if (std::abs(beta_11 - 1) < EPSILON)
	{
		// Q is a trapezium.
		v = beta;
		if ((v * (alpha_11 - 1) + 1) == 0)
		{
			return false;
		}
		u = alpha / (v * (alpha_11 - 1) + 1);
	}
	else
	{
		float A = 1 - beta_11;
		float B = alpha * (beta_11 - 1) - beta * (alpha_11 - 1) - 1;
		float C = alpha;
		float D = B * B - 4 * A * C;
		if (D < 0) return false;
		float Q = -0.5f * (B + ((B < 0 ? -1 : 1) * std::sqrt(D)));
		u = Q / A;
		if ((u < 0) || (u > 1)) u = C / Q;
		v = beta / (u * (beta_11 - 1) + 1);
	}

	return true;
}

QT_TEST(bezier_test)
{
	Vec3 p[4], l[4], r[4];
	p[0].Set(-1,0,0);
	p[1].Set(-1,1,0);
	p[2].Set(1,1,0);
	p[3].Set(1,0,0);
	Bezier b;
	b.DeCasteljauHalveCurve(p,l,r);
	QT_CHECK_EQUAL(l[0],(Vec3(-1,0,0)));
	QT_CHECK_EQUAL(l[1],(Vec3(-1,0.5,0)));
	QT_CHECK_EQUAL(l[2],(Vec3(-0.5,0.75,0)));
	QT_CHECK_EQUAL(l[3],(Vec3(0,0.75,0)));

	QT_CHECK_EQUAL(r[3],(Vec3(1,0,0)));
	QT_CHECK_EQUAL(r[2],(Vec3(1,0.5,0)));
	QT_CHECK_EQUAL(r[1],(Vec3(0.5,0.75,0)));
	QT_CHECK_EQUAL(r[0],(Vec3(0,0.75,0)));

	b.SetFromCorners(Vec3(1,0,1),Vec3(-1,0,1),Vec3(1,0,-1),Vec3(-1,0,-1));
	QT_CHECK(!b.CheckForProblems());
}
