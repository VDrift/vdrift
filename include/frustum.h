#ifndef _FRUSTUM_H
#define _FRUSTUM_H

#include "mathvector.h"
#include "mathplane.h"

#include <iostream>

/*template <typename T>
class FRUSTUM
{
public:
	FRUSTUM() : planes(6) {}
	FRUSTUM(T cfrustum[][4]) : planes(6)
	{
		Set(cfrustum);
	}
	void Set(T cfrustum[][4])
	{
		for (int i = 0; i < 6; i++)
			planes[i].Set(cfrustum[i]);
	}

	void DebugPrint(std::ostream & output)
	{
		for (int i = 0; i < 6; i++)
			output << i << ". " << planes[i] << "\n";
	}

	inline const MATHPLANE <T> & operator[](const int n) const
	{
		assert(n >= 0);
		assert(n < 6);
		return planes[n];
	}

	inline MATHPLANE <T> & operator[](const int n)
	{
		assert(n >= 0);
		assert(n < 6);
		return planes[n];
	}

private:
	std::vector <MATHPLANE <T> > planes;
};*/

//typedef float FRUSTUM[6][4];

struct FRUSTUM
{
public:
	FRUSTUM() {}
	FRUSTUM(float cfrustum[][4])
	{
		Set(cfrustum);
	}
	void Set(float cfrustum[][4])
	{
		for (int i = 0; i < 6; i++)
			for (int n = 0; n < 4; n++)
				frustum[i][n] = cfrustum[i][n];
	}

	void Extract(float * proj, float * view)
	{
		float   clip[16];
		float   t;

		/* Combine the two matrices (multiply projection by modelview) */
		clip[ 0] = view[ 0] * proj[ 0] + view[ 1] * proj[ 4] + view[ 2] * proj[ 8] + view[ 3] * proj[12];
		clip[ 1] = view[ 0] * proj[ 1] + view[ 1] * proj[ 5] + view[ 2] * proj[ 9] + view[ 3] * proj[13];
		clip[ 2] = view[ 0] * proj[ 2] + view[ 1] * proj[ 6] + view[ 2] * proj[10] + view[ 3] * proj[14];
		clip[ 3] = view[ 0] * proj[ 3] + view[ 1] * proj[ 7] + view[ 2] * proj[11] + view[ 3] * proj[15];

		clip[ 4] = view[ 4] * proj[ 0] + view[ 5] * proj[ 4] + view[ 6] * proj[ 8] + view[ 7] * proj[12];
		clip[ 5] = view[ 4] * proj[ 1] + view[ 5] * proj[ 5] + view[ 6] * proj[ 9] + view[ 7] * proj[13];
		clip[ 6] = view[ 4] * proj[ 2] + view[ 5] * proj[ 6] + view[ 6] * proj[10] + view[ 7] * proj[14];
		clip[ 7] = view[ 4] * proj[ 3] + view[ 5] * proj[ 7] + view[ 6] * proj[11] + view[ 7] * proj[15];

		clip[ 8] = view[ 8] * proj[ 0] + view[ 9] * proj[ 4] + view[10] * proj[ 8] + view[11] * proj[12];
		clip[ 9] = view[ 8] * proj[ 1] + view[ 9] * proj[ 5] + view[10] * proj[ 9] + view[11] * proj[13];
		clip[10] = view[ 8] * proj[ 2] + view[ 9] * proj[ 6] + view[10] * proj[10] + view[11] * proj[14];
		clip[11] = view[ 8] * proj[ 3] + view[ 9] * proj[ 7] + view[10] * proj[11] + view[11] * proj[15];

		clip[12] = view[12] * proj[ 0] + view[13] * proj[ 4] + view[14] * proj[ 8] + view[15] * proj[12];
		clip[13] = view[12] * proj[ 1] + view[13] * proj[ 5] + view[14] * proj[ 9] + view[15] * proj[13];
		clip[14] = view[12] * proj[ 2] + view[13] * proj[ 6] + view[14] * proj[10] + view[15] * proj[14];
		clip[15] = view[12] * proj[ 3] + view[13] * proj[ 7] + view[14] * proj[11] + view[15] * proj[15];

		/* Extract the numbers for the RIGHT plane */
		frustum[0][0] = clip[ 3] - clip[ 0];
		frustum[0][1] = clip[ 7] - clip[ 4];
		frustum[0][2] = clip[11] - clip[ 8];
		frustum[0][3] = clip[15] - clip[12];

		/* Normalize the result */
		t = sqrt( frustum[0][0] * frustum[0][0] + frustum[0][1] * frustum[0][1] + frustum[0][2] * frustum[0][2] );
		frustum[0][0] /= t;
		frustum[0][1] /= t;
		frustum[0][2] /= t;
		frustum[0][3] /= t;

		/* Extract the numbers for the LEFT plane */
		frustum[1][0] = clip[ 3] + clip[ 0];
		frustum[1][1] = clip[ 7] + clip[ 4];
		frustum[1][2] = clip[11] + clip[ 8];
		frustum[1][3] = clip[15] + clip[12];

		/* Normalize the result */
		t = sqrt( frustum[1][0] * frustum[1][0] + frustum[1][1] * frustum[1][1] + frustum[1][2] * frustum[1][2] );
		frustum[1][0] /= t;
		frustum[1][1] /= t;
		frustum[1][2] /= t;
		frustum[1][3] /= t;

		/* Extract the BOTTOM plane */
		frustum[2][0] = clip[ 3] + clip[ 1];
		frustum[2][1] = clip[ 7] + clip[ 5];
		frustum[2][2] = clip[11] + clip[ 9];
		frustum[2][3] = clip[15] + clip[13];

		/* Normalize the result */
		t = sqrt( frustum[2][0] * frustum[2][0] + frustum[2][1] * frustum[2][1] + frustum[2][2] * frustum[2][2] );
		frustum[2][0] /= t;
		frustum[2][1] /= t;
		frustum[2][2] /= t;
		frustum[2][3] /= t;

		/* Extract the TOP plane */
		frustum[3][0] = clip[ 3] - clip[ 1];
		frustum[3][1] = clip[ 7] - clip[ 5];
		frustum[3][2] = clip[11] - clip[ 9];
		frustum[3][3] = clip[15] - clip[13];

		/* Normalize the result */
		t = sqrt( frustum[3][0] * frustum[3][0] + frustum[3][1] * frustum[3][1] + frustum[3][2] * frustum[3][2] );
		frustum[3][0] /= t;
		frustum[3][1] /= t;
		frustum[3][2] /= t;
		frustum[3][3] /= t;

		/* Extract the FAR plane */
		frustum[4][0] = clip[ 3] - clip[ 2];
		frustum[4][1] = clip[ 7] - clip[ 6];
		frustum[4][2] = clip[11] - clip[10];
		frustum[4][3] = clip[15] - clip[14];

		/* Normalize the result */
		t = sqrt( frustum[4][0] * frustum[4][0] + frustum[4][1] * frustum[4][1] + frustum[4][2] * frustum[4][2] );
		frustum[4][0] /= t;
		frustum[4][1] /= t;
		frustum[4][2] /= t;
		frustum[4][3] /= t;

		/* Extract the NEAR plane */
		frustum[5][0] = clip[ 3] + clip[ 2];
		frustum[5][1] = clip[ 7] + clip[ 6];
		frustum[5][2] = clip[11] + clip[10];
		frustum[5][3] = clip[15] + clip[14];

		/* Normalize the result */
		t = sqrt( frustum[5][0] * frustum[5][0] + frustum[5][1] * frustum[5][1] + frustum[5][2] * frustum[5][2] );
		frustum[5][0] /= t;
		frustum[5][1] /= t;
		frustum[5][2] /= t;
		frustum[5][3] /= t;
	}

	float frustum[6][4];
};

#endif
