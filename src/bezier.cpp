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

std::ostream & operator << (std::ostream &os, const BEZIER & b)
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

BEZIER::BEZIER() :
	length(0),
	width(1),
	dist_from_start(0),
	next_patch(0),
	racing_line_fraction(0),
	racing_line_radius(0)
{
	// ctor
}

BEZIER::~BEZIER()
{
	// dtor
}

BEZIER::BEZIER(const BEZIER & other)
{
	CopyFrom(other);
}

BEZIER & BEZIER::operator=(const BEZIER & other)
{
	return CopyFrom(other);
}

AABB <float> BEZIER::GetAABB() const
{
	float maxv[3] = {0, 0, 0};
	float minv[3] = {0, 0, 0};
	bool havevals[6];
	for (int n = 0; n < 6; n++)
		havevals[n] = false;

	for (int x = 0; x < 4; x++)
	{
		for (int y = 0; y < 4; y++)
		{
			MATHVECTOR <float, 3> temp(points[x][y]);

			//cache for bbox stuff
			for ( int n = 0; n < 3; n++ )
			{
				if (!havevals[n])
				{
					maxv[n] = temp[n];
					havevals[n] = true;
				}
				else if (temp[n] > maxv[n])
					maxv[n] = temp[n];

				if (!havevals[n+3])
				{
					minv[n] = temp[n];
					havevals[n+3] = true;
				}
				else if (temp[n] < minv[n])
					minv[n] = temp[n];
			}
		}
	}

	MATHVECTOR <float, 3> bboxmin(minv[0], minv[1], minv[2]);
	MATHVECTOR <float, 3> bboxmax(maxv[0], maxv[1], maxv[2]);

	AABB <float> box;
	box.SetFromCorners(bboxmin, bboxmax);
	return box;
}

void BEZIER::SetFromCorners(
	const MATHVECTOR <float, 3> & fl,
	const MATHVECTOR <float, 3> & fr,
	const MATHVECTOR <float, 3> & bl,
	const MATHVECTOR <float, 3> & br)
{
	MATHVECTOR <float, 3> temp;

	//assign corners
	points[0][0] = fl;
	points[0][3] = fr;
	points[3][3] = br;
	points[3][0] = bl;

	//calculate intermediate front and back points
	temp = fr - fl;
	points[0][1] = fl + temp * 1.0/3.0;
	points[0][2] = fl + temp * 2.0/3.0;

	temp = br - bl;
	points[3][1] = bl + temp * 1.0/3.0;
	points[3][2] = bl + temp * 2.0/3.0;

	//calculate intermediate left and right points
	for (int i = 0; i < 4; ++i)
	{
		temp = points[3][i] - points[0][i];
		points[1][i] = points[0][i] + temp * 1.0/3.0;
		points[2][i] = points[0][i] + temp * 2.0/3.0;
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
void BEZIER::FitSpline(MATHVECTOR <float, 3> p[])
{
	// use chord length for shortest(best) cubic spline approximation
	float c3 = (p[1] - p[0]).Magnitude();
	float c2 = (p[2] - p[1]).Magnitude();
	float c1 = (p[3] - p[2]).Magnitude();

	// cases where p[1] is close to p[2] might lead to instabilities(need some heuristic)
	if (50 * c2 < c1 + c3)
	{
		p[1] = p[0] + (p[1] - p[0]) * 0.98;
		p[2] = p[3] + (p[2] - p[3]) * 0.98;
		c3 = c3 * 0.98;
		c2 = (p[2] - p[1]).Magnitude();
		c1 = c1 * 0.98;
	}

	float t1 = c1 / (c1 + c2 + c3);
	float t2 = (c1 + c2) / (c1 + c2 + c3);

	// Solve M * x = y
	float m00 = B1(t1);
	float m01 = B2(t1);
	float m10 = B1(t2);
	float m11 = B2(t2);

	float detM = m00 * m11 - m01 * m10;
	if (fabs(detM) > 1E-3)
	{
		// y = p - p0 * B0(t) - p3 * B3(t)
		MATHVECTOR <float, 3> y1 = p[1] - p[0] * B0(t1) - p[3] * B3(t1);
		MATHVECTOR <float, 3> y2 = p[2] - p[0] * B0(t2) - p[3] * B3(t2);

		// Minv
		float s = 1 / detM;
		float n00 = s * m11;
		float n01 = -s * m01;
		float n10 = -s * m10;
		float n11 = s * m00;

		// x = Minv * y
		MATHVECTOR <float, 3> x1 = y1 * n00 + y2 * n01;
		MATHVECTOR <float, 3> x2 = y1 * n10 + y2 * n11;

		p[1] = x1;
		p[2] = x2;
	}
}

// adjust spline to go through p1, p2 for p1 == p2
void BEZIER::FitMidPoint(MATHVECTOR <float, 3> p[])
{
	p[0].Set(-150, 0, 0);
	p[1].Set(100, 60, 0);
	p[2].Set(100, 60, 0);
	p[3].Set(200, 0, 0);

	MATHVECTOR <float, 3> d3 = p[0] - p[1];
	MATHVECTOR <float, 3> d2 = p[1] - p[2];
	MATHVECTOR <float, 3> d1 = p[2] - p[3];
	float c3 = d3.Magnitude();
	float c2 = d2.Magnitude();
	float c1 = d1.Magnitude();

	if (100 * c2 > c1 + c3) return;

	// chord length approximation(doesn't work that good)
	// pm for p = p1 = p2
	// p = p0 * B0(t) + pm * (B1(t) + B2(t)) + p3 * B3(t)
	//float t = c1 / (c1 + c3);
	//MATHVECTOR <float, 3> pm = (p[1] - p[0] * B0(t) - p[3] * B3(t)) / (B1(t) + B2(t));

	// extrude along midpoint normal
	// nm = p0 * N0 + pm * (N1 + N2) + p3 * N3 with  pm = p1 + nm * s
	MATHVECTOR <float, 3> nm = (d2 - d3).Normalize();
	MATHVECTOR <float, 3> y = nm / 6.0 + p[1] - p[3];
	MATHVECTOR <float, 3> a = p[0] - p[3];
	MATHVECTOR <float, 3> b = -nm;

	// y = a * t + b * s
	// y = M * x with x = (t, s) and M = (a, b)
	// x = Minv * y
	float s;//, t;
	MATHVECTOR <float, 3> det = a.cross(b);
	if (det[0] != 0.0)
	{
		float m00 = a[1];
		//float m01 = b[1];
		float m10 = a[2];
		//float m11 = b[2];

		// Minv
		float d = 1.0 / det[0];
		//float n00 = d * m11;
		//float n01 = -d * m01;
		float n10 = -d * m10;
		float n11 = d * m00;

		// x = Minv * y
		//t = y[1] * n00 + y[2] * n01; // need s only
		s = y[1] * n10 + y[2] * n11;
	}
	else if (det[1] != 0.0)
	{
		//m00 = a[2]; m01 = b[2]; m10 = a[0]; m11 = b[0];
		s = 1.0 / det[1] * (-y[2] * a[0] + y[0] * a[2]);
	}
	else if (det[2] != 0.0)
	{
		//m00 = a[0]; m01 = b[0]; m10 = a[1]; m11 = b[1];
		s = 1.0 / det[2] * (-y[0] * a[1] + y[1] * a[0]);
	}
	else
	{
		return;	// failure
	}

	MATHVECTOR <float, 3> pm = p[1] + nm * s;

	p[1] = pm;
	p[2] = pm;
}

void BEZIER::Attach(BEZIER & next)
{
	// we want C1 continuity between attached patches
	// this means the second front interior point has to be collinear
	// to the first interior point of the next patch
	// additionally they have to have the same distance to the joint point

	// get left tangent
	MATHVECTOR<float, 3> dl1 = GetBL() - GetFL();
	MATHVECTOR<float, 3> dl2 = next.GetFL() - next.GetBL();
	float dl1_len = dl1.Magnitude();
	float dl2_len = dl2.Magnitude();
	MATHVECTOR<float, 3> nl = dl1 / dl1_len + dl2 / dl2_len;	// normal
	MATHVECTOR<float, 3> bl = nl.cross(dl2);					// binormal
	MATHVECTOR<float, 3> tl;									// tangent
	float tl_len = std::min(dl1_len, dl2_len) / 3.0;			// tangent length
	if (bl.MagnitudeSquared() > 0.0001)
	{
		tl = bl.cross(nl).Normalize();
		//tl_len = std::min(-tl.dot(dl1), tl.dot(dl2)) / 3.0;	// proj length
	}
	else
	{
		tl = dl2 / dl2_len;
		//tl_len = std::min(dl1_len, dl2_len) / 3.0;
	}

	// get right tangent
	MATHVECTOR<float, 3> dr1 = GetBR() - GetFR();
	MATHVECTOR<float, 3> dr2 = next.GetFR() - next.GetBR();
	float dr1_len = dr1.Magnitude();
	float dr2_len = dr2.Magnitude();
	MATHVECTOR<float, 3> nr = dr1 / dr1_len + dr2 / dr2_len;
	MATHVECTOR<float, 3> br = nr.cross(dr2);
	MATHVECTOR<float, 3> tr;
	float tr_len = std::min(dr1_len, dr2_len) / 3.0;
	if (br.MagnitudeSquared() > 0.0001)
	{
		tr = br.cross(nr).Normalize();
		//tr_len = std::min(-tr.dot(dr1), tr.dot(dr2)) / 3.0;
	}
	else
	{
		tr = dr2 / dr2_len;
		//tr_len = std::min(dr1_len, dr2_len) / 3.0;
	}

	// adjust interior control points
	points[1][0] = points[0][0] - tl * tl_len;
	points[1][1] = points[1][0] * 2 / 3.0 + points[1][3] * 1 / 3.0;
	points[1][2] = points[1][0] * 1 / 3.0 + points[1][3] * 2 / 3.0;
	points[1][3] = points[0][3] - tr * tr_len;
	next.points[2][0] = next.points[3][0] + tl * tl_len;
	next.points[2][1] = next.points[2][0] * 2 / 3.0 + next.points[2][3] * 1 / 3.0;
	next.points[2][2] = next.points[2][0] * 1 / 3.0 + next.points[2][3] * 2 / 3.0;
	next.points[2][3] = next.points[3][3] + tr * tr_len;

	// length along center
	length = (dl1_len + dr2_len) * 0.5;

	// width at patch back
	width = (GetBL() - GetBR()).Magnitude();

	// set next patch racing line info to some sensible values
	// radius at track center from circle through 3 points approximation
	next.racing_line = (next.GetBL() - next.GetBR()) * 0.5;
	next.racing_line_fraction = 0.5;
	next.racing_line_radius = 10000;
	float sinalpha = dl1.cross(dl2).Magnitude() / (dl1_len * dl2_len);
	if (std::fabs(sinalpha) > 0.001)
	{
		// assume dl collinear to dr
		float d1_len = (dl1_len + dr1_len) * 0.5;
		float d2_len = (dl2_len + dr2_len) * 0.5;
		if (d1_len > d2_len)
		{
			MATHVECTOR<float, 3> p3 = (next.GetFL() + next.GetFR()) * 0.5;
			MATHVECTOR<float, 3> p2 = (GetFL() + GetFR()) * 0.5;
			MATHVECTOR<float, 3> p1 = p2 + dl1 / dl1_len * d2_len;
			next.racing_line_radius = (p1 - p3).Magnitude() / (2.0 * sinalpha);
		}
		else
		{
			MATHVECTOR<float, 3> p1 = (GetBL() + GetBR()) * 0.5;
			MATHVECTOR<float, 3> p2 = (GetFL() + GetFR()) * 0.5;
			MATHVECTOR<float, 3> p3 = p2 + dl2 / dl2_len * d1_len;
			next.racing_line_radius = (p1 - p3).Magnitude() / (2.0 * sinalpha);
		}
	}

	// store the pointer to next patch
	next_patch = &next;
}

void BEZIER::Reverse()
{
	MATHVECTOR <float, 3> oldpoints[4][4];

	for (int n = 0; n < 4; n++)
		for (int i = 0; i < 4; i++)
			oldpoints[n][i] = points[n][i];

	for (int n = 0; n < 4; n++)
		for (int i = 0; i < 4; i++)
			points[n][i] = oldpoints[3-n][3-i];
}

MATHVECTOR <float, 3> BEZIER::Bernstein(float u, const MATHVECTOR <float, 3> p[]) const
{
	float oneminusu(1.0f-u);
	MATHVECTOR <float, 3> a = p[0]*(u*u*u);
	MATHVECTOR <float, 3> b = p[1]*(3*u*u*oneminusu);
	MATHVECTOR <float, 3> c = p[2]*(3*u*oneminusu*oneminusu);
	MATHVECTOR <float, 3> d = p[3]*(oneminusu*oneminusu*oneminusu);
	return a+b+c+d;
}

MATHVECTOR <float, 3> BEZIER::BernsteinTangent(float u, const MATHVECTOR <float, 3> p[]) const
{
	float oneminusu(1.0f-u);
	MATHVECTOR <float, 3> a = (p[1]-p[0])*(3*u*u);
	MATHVECTOR <float, 3> b = (p[2]-p[1])*(3*2*u*oneminusu);
	MATHVECTOR <float, 3> c = (p[3]-p[2])*(3*oneminusu*oneminusu);
	return a+b+c;
}

MATHVECTOR <float, 3> BEZIER::SurfPoint(float px, float py) const
{
	// get splines along x axis
	MATHVECTOR <float, 3> temp[4];
	for (int j = 0; j < 4; ++j)
	{
		temp[j] = Bernstein(px, points[j]);
	}
	return Bernstein(py, temp);
}

MATHVECTOR <float, 3> BEZIER::SurfNorm(float px, float py) const
{
	MATHVECTOR <float, 3> tempy[4];
	MATHVECTOR <float, 3> tempx[4];
	MATHVECTOR <float, 3> temp2[4];

	// get splines along x axis
	for (int j = 0; j < 4; ++j)
	{
		tempy[j] = Bernstein(px, points[j]);
	}

	// get splines along y axis
	for (int j = 0; j < 4; ++j)
	{
		for (int i = 0; i < 4; ++i)
		{
			temp2[i] = points[i][j];
		}
		tempx[j] = Bernstein(py, temp2);
	}

	MATHVECTOR <float, 3> tx = BernsteinTangent(px, tempx);
	MATHVECTOR <float, 3> ty = BernsteinTangent(py, tempy);
	MATHVECTOR <float, 3> n = -tx.cross(ty).Normalize();

	return n;
}

const BEZIER * BEZIER::GetNextClosestPatch(const MATHVECTOR<float, 3> & point) const
{
	float dist2 = (point - GetFrontCenter()).MagnitudeSquared();
	const BEZIER * p0 = this;
	const BEZIER * p1 = next_patch;
	while (p1)
	{
		float next_dist2 = (point - p1->GetFrontCenter()).MagnitudeSquared();
		if (next_dist2 > dist2)
		{
			return p0;
		}
		dist2 = next_dist2;
		p0 = p1;
		p1 = p0->next_patch;
		assert(p1 != this);
	}
	return p0;
}

BEZIER & BEZIER::CopyFrom(const BEZIER &other)
{
	for (int x = 0; x < 4; ++x)
	{
		for (int y = 0; y < 4; ++y)
		{
			points[x][y] = other.points[x][y];
		}
	}
	length = other.length;
	width = other.width;
	dist_from_start = other.dist_from_start;
	next_patch = other.next_patch;
	racing_line = other.racing_line;
	racing_line_fraction = other.racing_line_fraction;
	racing_line_radius = other.racing_line_radius;
	return *this;
}

void BEZIER::ReadFrom(std::istream &openfile)
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

void BEZIER::WriteTo(std::ostream &openfile) const
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

bool BEZIER::CollideSubDivQuadSimpleNorm(
	const MATHVECTOR <float, 3> & origin,
	const MATHVECTOR <float, 3> & direction,
	MATHVECTOR <float, 3> & outtri,
	MATHVECTOR <float, 3> & normal) const
{
	bool col = false;
	const int COLLISION_QUAD_DIVS = 6;
	const float areacut = 0.5;

	float t, u, v;

	float su = 0;
	float sv = 0;

	float umin = 0;
	float umax = 1;
	float vmin = 0;
	float vmax = 1;

	MATHVECTOR <float, 3> ul = points[3][3];
	MATHVECTOR <float, 3> ur = points[3][0];
	MATHVECTOR <float, 3> br = points[0][0];
	MATHVECTOR <float, 3> bl = points[0][3];

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

			ul = SurfPoint(tu[0], tv[0]);
			ur = SurfPoint(tu[1], tv[0]);
			br = SurfPoint(tu[1], tv[1]);
			bl = SurfPoint(tu[0], tv[1]);
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
			vmax = sv + (0.5*areacut)*(vmax - vmin);
			vmin = sv - (0.5*areacut)*(vmax - vmin);
			umax = su + (0.5*areacut)*(umax - umin);
			umin = su - (0.5*areacut)*(umax - umin);
		}
		else
		{
			outtri = origin;
			return false;
		}
	}

	outtri = SurfPoint(su, sv);
	normal = SurfNorm(su, sv);
	return true;
}

void BEZIER::DeCasteljauHalveCurve(MATHVECTOR <float, 3> * points4, MATHVECTOR <float, 3> * left4, MATHVECTOR <float, 3> * right4) const
{
	left4[0] = points4[0];
	left4[1] = (points4[0]+points4[1])*0.5;
	MATHVECTOR <float, 3> point23 = (points4[1]+points4[2])*0.5;
	left4[2] = (left4[1]+point23)*0.5;

	right4[3] = points4[3];
	right4[2] = (points4[3]+points4[2])*0.5;
	right4[1] = (right4[2]+point23)*0.5;

	left4[3] = right4[0] = (right4[1]+left4[2])*0.5;
}

bool BEZIER::CheckForProblems() const
{
	MATHVECTOR <float, 3> corners[4];
	corners[0] = points[0][0];
	corners[1] = points[0][3];
	corners[2] = points[3][3];
	corners[3] = points[3][0];

	bool problem = false;

	for (int i = 0; i < 4; i++)
	{
		MATHVECTOR <float, 3> leg1(corners[(i+1)%4] - corners[i]);
		MATHVECTOR <float, 3> leg2(corners[(i+2)%4] - corners[i]);
		MATHVECTOR <float, 3> leg3(corners[(i+3)%4] - corners[i]);

		MATHVECTOR <float, 3> dir1 = leg1.cross(leg2);
		MATHVECTOR <float, 3> dir2 = leg1.cross(leg3);
		MATHVECTOR <float, 3> dir3 = leg2.cross(leg3);

		if (dir1.dot(dir2) < -0.0001)
			problem = true;
		if (dir1.dot(dir3) < -0.0001)
			problem = true;
		if (dir3.dot(dir2) < -0.0001)
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

bool BEZIER::IntersectQuadrilateralF(
	const MATHVECTOR <float, 3> & orig, const MATHVECTOR <float, 3> & dir,
	const MATHVECTOR <float, 3> & v_00, const MATHVECTOR <float, 3> & v_10,
	const MATHVECTOR <float, 3> & v_11, const MATHVECTOR <float, 3> & v_01,
	float &t, float &u, float &v) const
{
	const float EPSILON = 0.000001;

	// Reject rays that are parallel to Q, and rays that intersect the plane
	// of Q either on the left of the line V00V01 or below the line V00V10.
	MATHVECTOR <float, 3> E_01 = v_10 - v_00;
	MATHVECTOR <float, 3> E_03 = v_01 - v_00;
	MATHVECTOR <float, 3> P = dir.cross(E_03);
	float det = E_01.dot(P);

	if (std::abs(det) < EPSILON) return false;

	MATHVECTOR <float, 3> T = orig - v_00;
	float alpha = T.dot(P) / det;

	if (alpha < 0.0) return false;

	MATHVECTOR <float, 3> Q = T.cross(E_01);
	float beta = dir.dot(Q) / det;

	if (beta < 0.0) return false;

	if (alpha + beta > 1.0)
	{
		// Reject rays that that intersect the plane of Q either on
		// the right of the line V11V10 or above the line V11V00.
		MATHVECTOR <float, 3> E_23 = v_01 - v_11;
		MATHVECTOR <float, 3> E_21 = v_10 - v_11;
		MATHVECTOR <float, 3> P_prime = dir.cross(E_21);
		float det_prime = E_23.dot(P_prime);

		if (std::abs(det_prime) < EPSILON) return false;

		MATHVECTOR <float, 3> T_prime = orig - v_11;
		float alpha_prime = T_prime.dot(P_prime) / det_prime;

		if (alpha_prime < 0.0) return false;

		MATHVECTOR <float, 3> Q_prime = T_prime.cross(E_23);
		float beta_prime = dir.dot(Q_prime) / det_prime;

		if (beta_prime < 0.0) return false;
	}

	// Compute the ray parameter of the intersection point, and
	// reject the ray if it does not hit Q.
	t = E_03.dot(Q) / det;

	if (t < 0.0) return false;

	// Compute the barycentric coordinates of the fourth vertex.
	// These do not depend on the ray, and can be precomputed
	// and stored with the quadrilateral.
	float alpha_11, beta_11;
	MATHVECTOR <float, 3> E_02 = v_11 - v_00;
	MATHVECTOR <float, 3> n = E_01.cross(E_03);

	if ((std::abs(n[0]) >= std::abs(n[1]))
		    && (std::abs(n[0]) >= std::abs(n[2])))
	{
		alpha_11 = ((E_02[1] * E_03[2]) - (E_02[2] * E_03[1])) / n[0];
		beta_11 = ((E_01[1] * E_02[2]) - (E_01[2]  * E_02[1])) / n[0];
	}
	else if ((std::abs(n[1]) >= std::abs(n[0]))
			 && (std::abs(n[1]) >= std::abs(n[2])))
	{
		alpha_11 = ((E_02[2] * E_03[0]) - (E_02[0] * E_03[2])) / n[1];
		beta_11 = ((E_01[2] * E_02[0]) - (E_01[0]  * E_02[2])) / n[1];
	}
	else
	{
		alpha_11 = ((E_02[0] * E_03[1]) - (E_02[1] * E_03[0])) / n[2];
		beta_11 = ((E_01[0] * E_02[1]) - (E_01[1]  * E_02[0])) / n[2];
	}

	// Compute the bilinear coordinates of the intersection point.
	if (std::abs(alpha_11 - (1.0)) < EPSILON)
	{
		// Q is a trapezium.
		u = alpha;
		if (std::abs(beta_11 - (1.0)) < EPSILON) v = beta; // Q is a parallelogram.
		else v = beta / ((u * (beta_11 - (1.0))) + (1.0)); // Q is a trapezium.
	}
	else if (std::abs(beta_11 - (1.0)) < EPSILON)
	{
		// Q is a trapezium.
		v = beta;
		if ( ((v * (alpha_11 - (1.0))) + (1.0)) == 0 )
		{
			return false;
		}
		u = alpha / ((v * (alpha_11 - (1.0))) + (1.0));
	}
	else
	{
		float A = (1.0) - beta_11;
		float B = (alpha * (beta_11 - (1.0)))
				- (beta * (alpha_11 - (1.0))) - (1.0);
		float C = alpha;
		float D = (B * B) - ((4.0) * A * C);
		if (D < 0) return false;
		float Q = (-0.5) * (B + ((B < (0.0) ? (-1.0) : (1.0))
				* std::sqrt(D)));
		u = Q / A;
		if ((u < (0.0)) || (u > (1.0))) u = C / Q;
		v = beta / ((u * (beta_11 - (1.0))) + (1.0));
	}

	return true;
}

void BEZIER::CalculateDistFromStart(BEZIER & start_patch)
{
	start_patch.dist_from_start = 0.0;
	BEZIER * curr_patch = start_patch.next_patch;
	float total_dist = start_patch.length;
	while (curr_patch && curr_patch != &start_patch)
	{
		curr_patch->dist_from_start = total_dist;
		total_dist += curr_patch->length;
		curr_patch = curr_patch->next_patch;
	}
}

QT_TEST(bezier_test)
{
	MATHVECTOR <float, 3> p[4], l[4], r[4];
	p[0].Set(-1,0,0);
	p[1].Set(-1,1,0);
	p[2].Set(1,1,0);
	p[3].Set(1,0,0);
	BEZIER b;
	b.DeCasteljauHalveCurve(p, l, r);
	QT_CHECK_EQUAL(l[0],(MATHVECTOR <float, 3>(-1,0,0)));
	QT_CHECK_EQUAL(l[1],(MATHVECTOR <float, 3>(-1,0.5,0)));
	QT_CHECK_EQUAL(l[2],(MATHVECTOR <float, 3>(-0.5,0.75,0)));
	QT_CHECK_EQUAL(l[3],(MATHVECTOR <float, 3>(0,0.75,0)));

	QT_CHECK_EQUAL(r[3],(MATHVECTOR <float, 3>(1,0,0)));
	QT_CHECK_EQUAL(r[2],(MATHVECTOR <float, 3>(1,0.5,0)));
	QT_CHECK_EQUAL(r[1],(MATHVECTOR <float, 3>(0.5,0.75,0)));
	QT_CHECK_EQUAL(r[0],(MATHVECTOR <float, 3>(0,0.75,0)));

	MATHVECTOR <float, 3> p0(1,0,1);
	MATHVECTOR <float, 3> p1(-1,0,1);
	MATHVECTOR <float, 3> p2(1,0,-1);
	MATHVECTOR <float, 3> p3(-1,0,-1);
	b.SetFromCorners(p0, p1, p2, p3);
	QT_CHECK(!b.CheckForProblems());

	l[0].Set(4, 0, 0);
	l[1].Set(0, 0, 4);
	l[2].Set(-4, 0, 0);
	l[3].Set(0, 0, -4);
	r[0].Set(2, 0, 0);
	r[1].Set(0, 0, 2);
	r[2].Set(-2, 0, 0);
	r[3].Set(0, 0, -2);
	BEZIER c[4];
	c[0].SetFromCorners(l[0], r[0], l[3], r[3]);
	c[1].SetFromCorners(l[1], r[1], l[0], r[0]);
	c[2].SetFromCorners(l[2], r[2], l[1], r[1]);
	c[3].SetFromCorners(l[3], r[3], l[2], r[2]);
	c[0].Attach(c[1]);
	c[1].Attach(c[2]);
	c[2].Attach(c[3]);
	c[3].Attach(c[0]);
	QT_CHECK_EQUAL(c[0].GetRacingLineRadius(), 3);
	QT_CHECK_EQUAL(c[1].GetRacingLineRadius(), 3);
	QT_CHECK_EQUAL(c[2].GetRacingLineRadius(), 3);
	QT_CHECK_EQUAL(c[3].GetRacingLineRadius(), 3);
}
