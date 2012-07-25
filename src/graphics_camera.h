#ifndef _GRAPHICS_CAMERA_H
#define _GRAPHICS_CAMERA_H

#include "mathvector.h"
#include "quaternion.h"

struct GRAPHICS_CAMERA
{
	float fov;
	float view_distance;
	MATHVECTOR <float, 3> pos;
	QUATERNION <float> orient;
	float w;
	float h;

	bool orthomode;
	MATHVECTOR <float, 3> orthomin;
	MATHVECTOR <float, 3> orthomax;

	GRAPHICS_CAMERA() :
		fov(45),
		view_distance(10000),
		w(1),
		h(1),
		orthomode(false)
		{}
};

#endif // _GRAPHICS_CAMERA_H
