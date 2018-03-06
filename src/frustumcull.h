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

#ifndef _FRUSTUM_CULL_H
#define _FRUSTUM_CULL_H

#include <cmath>


// Cull sphere against frustum planes
// plane normals are pointing into frustum

template <typename T4, typename T3, typename T>
static inline bool FrustumCull(T4 frustum[6], T3 center, T radius)
{
	for (int i = 0; i < 6; i++)
	{
		auto plane = frustum[i];
		auto distance = plane[0] * center[0] + plane[1] * center[1] + plane[2] * center[2] + plane[3];
		if (radius < -distance)
			return true;
	}
	return false;
}

// Alterantive variant using squared radius

template <typename T4, typename T3, typename T>
static inline bool FrustumCull2(T4 frustum[6], T3 center, T radius2)
{
	for (int i = 0; i < 6; i++)
	{
		auto plane = frustum[i];
		auto distance = plane[0] * center[0] + plane[1] * center[1] + plane[2] * center[2] + plane[3];
		if (radius2 > -std::abs(distance) * distance)
			return true;
	}
	return false;
}


// Cull aabb against frustum planes
// center = (min + max) / 2
// extent = (max - min) / 2

template <typename T4, typename T3>
static inline bool FrustumCull(T4 frustum[6], T3 center, T3 extent)
{
	for (int i = 0; i < 6; i++)
	{
		auto plane = frustum[i];
		T3 vertex(
			std::copysign(extent[0], plane[0]),
			std::copysign(extent[1], plane[1]),
			std::copysign(extent[2], plane[2]));
		T3 normal(plane[0], plane[1], plane[2]);
		if ((center + vertex).dot(normal) < -plane[3])
			return true;
	}
	return false;
}


// Cull sphere smaller than a minimum size in pixels
// angular_size = 2 * radius / distance
// pixel_size = screen_height * angular_size / fov
// cull_threshold = (min_angular_size / 2)^2

template <typename T>
static inline T ContributionCullThreshold(T resy, T fovy = M_PI_2, T min_pixel_size = 2)
{
	T min_angular_size = min_pixel_size * fovy / resy;
	return min_angular_size * min_angular_size * T(0.25);
}

template <typename T3, typename T>
static inline bool ContributionCull(T3 campos, T cull_threshold, T3 center, T radius)
{
	T3 d = center - campos;
	return radius * radius < d.dot(d) * cull_threshold;
}

// Alterantive variant using squared radius

template <typename T3, typename T>
static inline bool ContributionCull2(T3 campos, T cull_threshold, T3 center, T radius2)
{
	T3 d = center - campos;
	return radius2 < d.dot(d) * cull_threshold;
}


// Frustum cull functors

template <typename T4>
struct FrustumCuller
{
	T4 (&frustum)[6];

	FrustumCuller(T4 (&nfrustum)[6]) :
		frustum(nfrustum)
	{}

	template <typename T3, typename T>
	inline bool operator()(const T3 & center, T radius) const
	{
		return FrustumCull(frustum, center, radius);
	}

	template <typename T3, typename T>
	inline bool operator()(const T3 & center, const T3 & extent, T /*radius*/) const
	{
		return FrustumCull(frustum, center, extent);
	}
};

template <typename T4>
static inline FrustumCuller<T4> MakeFrustumCuller(T4 (&frustum)[6])
{
	return FrustumCuller<T4>(frustum);
}

template <typename T4, typename T3, typename T>
struct FrustumCullerPersp
{
	T4 (&frustum)[6];
	T3 & campos;
	T cull_threshold;

	FrustumCullerPersp(T4 (&nfrustum)[6], T3 & ncampos, T ncull_threshold) :
		frustum(nfrustum),
		campos(ncampos),
		cull_threshold(ncull_threshold)
	{}

	inline bool operator()(const T3 & center, T radius) const
	{
		return FrustumCull(frustum, center, radius) || ContributionCull(campos, cull_threshold, center, radius);
	}

	inline bool operator()(const T3 & center, const T3 & extent, T radius) const
	{
		return FrustumCull(frustum, center, extent) || ContributionCull(campos, cull_threshold, center, radius);
	}
};

template <typename T4, typename T3, typename T>
static inline FrustumCullerPersp<T4, T3, T> MakeFrustumCullerPersp(T4 (&frustum)[6], T3 campos, T cull_threshold)
{
	return FrustumCullerPersp<T4, T3, T>(frustum, campos, cull_threshold);
}

#endif
