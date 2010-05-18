#include "mesh_gen.h"

#include <cmath>

#include "vertexarray.h"
#include "mathvector.h"

static double sinD(double degrees)
{
	return (double) sin(degrees * M_PI / 180.000f);
}

static double cosD(double degrees)
{
	return (double) cos(degrees * M_PI / 180.000f);
}

namespace MESHGEN
{








void mesh_gen_tire(VERTEXARRAY *tire, float sectionWidth_mm, float aspectRatio, float rimDiameter_in)
{
	// configurable parameters - set to defaults
	unsigned int segmentsAround = 32;
	float innerRadius = 0.65f;
	float innerWidth = 0.60f;

	float sidewallRadius = 0.75f;
	float sidewallBulge = 0.05f;		// bulge from tread

	float shoulderRadius = 0.95f;
	float shoulderBulge = 0.05f;		// bulge from tread

	float treadRadius = 1.00f;
	float treadWidth = 0.60f;

	float vertexNormalLength = 1.0f;



	// use this to build a tire based on the tire code passed to this function
	// if the code under this if doesn't happen, then the parameters are meaningless
	if (true)
	{
		// use function parameters - comment out this section
		float sectionWidth_m = sectionWidth_mm / 1000.0f;
		float rimRadius_m = rimDiameter_in * 0.0254f * 0.5;


		innerRadius = rimRadius_m;
		treadWidth = sectionWidth_m - shoulderBulge;


		// aspect ratio is a little strange, but i didn't make this stuff up
		// en.wikipedia.org/wiki/Tire_code
		// auto.howstuffworks.com/tire2.htm
		if (aspectRatio <= 0)	// aspect ratio ommitted default to 82%
		{
		    aspectRatio = 82.0f;
		}



		////////////////////////////////////////////////////////
		// this isn't iso standard, but it is sorta important
		// math says a percent should be less than 1
		// but the tire code has it as a whole number
		// could use a uint, but floats are always best for 3d
		if (aspectRatio > 1)    // then he gave
		{
		    aspectRatio = aspectRatio / 100.0f;
		}


		// if he gave us a large number specifically over 200 we need to thing that its in mm
		if (aspectRatio > 2)		// he gave us a mm diameter for the entire tire
		{
		    treadRadius = aspectRatio / 2000.0f;
		}
		else		// otherwise: he gave us the normal percent
		{
		    treadRadius = sectionWidth_m * aspectRatio + rimRadius_m;

		}


		// fudge some values by what looks good.
		innerWidth = treadWidth;

		sidewallBulge = innerWidth * 0.075f;
		shoulderBulge = treadWidth * 0.100f;

		float totalSidewallHeight = sectionWidth_m * aspectRatio;
		sidewallRadius = totalSidewallHeight/3.0f + rimRadius_m;
		shoulderRadius = totalSidewallHeight/1.1f + rimRadius_m;
	}








	// non-configurable parameters
	unsigned int vertexRings = 8;
	float angleIncrement = 360.0f / (float) segmentsAround;

	/////////////////////////////////////
	//
	// vertices (temporary data)
	//
	unsigned int vertexesAround = segmentsAround + 1;
	unsigned int vertexCount = vertexesAround * vertexRings;
	unsigned int vertexFloatCount = vertexCount * 3;	// * 3 cause there are 3 floats in a vertex
	float *vertexData = new float [ vertexFloatCount ];

	// a re-used for loop variable
	unsigned int lv;

	// Right-side, Inner Ring
	for (lv=0 ; lv<vertexesAround ; lv++)
	{
		float *x = &vertexData[(lv+vertexesAround * 0) * 3 + 0];
		float *y = &vertexData[(lv+vertexesAround * 0) * 3 + 1];
		float *z = &vertexData[(lv+vertexesAround * 0) * 3 + 2];

		*x = 1.0f * (innerWidth / 2.0f);
		*y = innerRadius * cosD(angleIncrement * lv);
		*z = innerRadius * sinD(angleIncrement * lv);
	}
	// Right-side, Sidewall Ring
	for (lv=0 ; lv<vertexesAround ; lv++)
	{
		float *x = &vertexData[(lv+vertexesAround * 1) * 3 + 0];
		float *y = &vertexData[(lv+vertexesAround * 1) * 3 + 1];
		float *z = &vertexData[(lv+vertexesAround * 1) * 3 + 2];

		*x = 1.0f * (treadWidth / 2.0f + sidewallBulge);
		*y = sidewallRadius * cosD(angleIncrement * lv);
		*z = sidewallRadius * sinD(angleIncrement * lv);
	}
	// Right-side, Shoulder Ring
	for (lv=0 ; lv<vertexesAround ; lv++)
	{
		float *x = &vertexData[(lv+vertexesAround * 2) * 3 + 0];
		float *y = &vertexData[(lv+vertexesAround * 2) * 3 + 1];
		float *z = &vertexData[(lv+vertexesAround * 2) * 3 + 2];

		*x = 1.0f * (treadWidth / 2.0f + shoulderBulge);
		*y = shoulderRadius * cosD(angleIncrement * lv);
		*z = shoulderRadius * sinD(angleIncrement * lv);
	}
	// Right-side, Tread Ring
	for (lv=0 ; lv<vertexesAround ; lv++)
	{
		float *x = &vertexData[(lv+vertexesAround * 3) * 3 + 0];
		float *y = &vertexData[(lv+vertexesAround * 3) * 3 + 1];
		float *z = &vertexData[(lv+vertexesAround * 3) * 3 + 2];

		*x = 1.0f * (treadWidth / 2.0f);
		*y = treadRadius * cosD(angleIncrement * lv);
		*z = treadRadius * sinD(angleIncrement * lv);
	}


	// Left-side, Tread Ring
	for (lv=0 ; lv<vertexesAround ; lv++)
	{
		float *x = &vertexData[(lv+vertexesAround * 4) * 3 + 0];
		float *y = &vertexData[(lv+vertexesAround * 4) * 3 + 1];
		float *z = &vertexData[(lv+vertexesAround * 4) * 3 + 2];

		*x = -1.0f * (treadWidth / 2.0f);
		*y = treadRadius * cosD(angleIncrement * lv);
		*z = treadRadius * sinD(angleIncrement * lv);
	}
	// Left-side, Shoulder Ring
	for (lv=0 ; lv<vertexesAround ; lv++)
	{
		float *x = &vertexData[(lv+vertexesAround * 5) * 3 + 0];
		float *y = &vertexData[(lv+vertexesAround * 5) * 3 + 1];
		float *z = &vertexData[(lv+vertexesAround * 5) * 3 + 2];

		*x = -1.0f * (treadWidth / 2.0f + shoulderBulge);
		*y = shoulderRadius * cosD(angleIncrement * lv);
		*z = shoulderRadius * sinD(angleIncrement * lv);
	}
	// Left-side, Sidewall Ring
	for (lv=0 ; lv<vertexesAround ; lv++)
	{
		float *x = &vertexData[(lv+vertexesAround * 6) * 3 + 0];
		float *y = &vertexData[(lv+vertexesAround * 6) * 3 + 1];
		float *z = &vertexData[(lv+vertexesAround * 6) * 3 + 2];

		*x = -1.0f * (treadWidth / 2.0f + sidewallBulge);
		*y = sidewallRadius * cosD(angleIncrement * lv);
		*z = sidewallRadius * sinD(angleIncrement * lv);
	}
	// Left-side, Inner Ring
	for (lv=0 ; lv<vertexesAround ; lv++)
	{
		float *x = &vertexData[(lv+vertexesAround * 7) * 3 + 0];
		float *y = &vertexData[(lv+vertexesAround * 7) * 3 + 1];
		float *z = &vertexData[(lv+vertexesAround * 7) * 3 + 2];

		*x = -1.0f * (innerWidth / 2.0f);
		*y = innerRadius * cosD(angleIncrement * lv);
		*z = innerRadius * sinD(angleIncrement * lv);
	}





	/////////////////////////////////////
	//
	//  now let's build triangles
	//
	unsigned int triVIndexCount = 2 * segmentsAround * (vertexRings-1) * 3;	// 2 * triangles make a square,   * 3 indexes in a triangle
	unsigned int *triData = new unsigned int [ triVIndexCount ];

	unsigned int triIndex = 0;
	unsigned int circleSegment = 0;

	unsigned int *triVIndex0;
	unsigned int *triVIndex1;
	unsigned int *triVIndex2;

	for (circleSegment=0 ; circleSegment<segmentsAround; circleSegment++)
	{
		// 1st triangle (Right-side - Inner to Sidewall)
		triVIndex0 = &triData[ (triIndex+0)*3 + 0 ];
		triVIndex1 = &triData[ (triIndex+0)*3 + 1 ];
		triVIndex2 = &triData[ (triIndex+0)*3 + 2 ];
		*triVIndex0 = circleSegment;
		*triVIndex1 = circleSegment + vertexesAround;
		*triVIndex2 = circleSegment +1;
		// 2nd triangle
		triVIndex0 = &triData[ (triIndex+1)*3 + 0 ];
		triVIndex1 = &triData[ (triIndex+1)*3 + 1 ];
		triVIndex2 = &triData[ (triIndex+1)*3 + 2 ];
		*triVIndex0 = circleSegment+vertexesAround;
		*triVIndex1 = circleSegment+vertexesAround+1;
		*triVIndex2 = circleSegment +1;

		// 3rd triangle (Right-side - Sidewall to shoulder)
		triVIndex0 = &triData[ (triIndex+2)*3 + 0 ];
		triVIndex1 = &triData[ (triIndex+2)*3 + 1 ];
		triVIndex2 = &triData[ (triIndex+2)*3 + 2 ];
		*triVIndex0 = circleSegment+vertexesAround*1;
		*triVIndex1 = circleSegment+vertexesAround*2;
		*triVIndex2 = circleSegment+vertexesAround*1 +1;
		// 4th triangle
		triVIndex0 = &triData[ (triIndex+3)*3 + 0 ];
		triVIndex1 = &triData[ (triIndex+3)*3 + 1 ];
		triVIndex2 = &triData[ (triIndex+3)*3 + 2 ];
		*triVIndex0 = circleSegment+vertexesAround*2;
		*triVIndex1 = circleSegment+vertexesAround*2 +1;
		*triVIndex2 = circleSegment+vertexesAround*1 +1;


		// 5th triangle (Right-side - Shoulder to Tread)
		triVIndex0 = &triData[ (triIndex+4)*3 + 0 ];
		triVIndex1 = &triData[ (triIndex+4)*3 + 1 ];
		triVIndex2 = &triData[ (triIndex+4)*3 + 2 ];
		*triVIndex0 = circleSegment+vertexesAround*2;
		*triVIndex1 = circleSegment+vertexesAround*3;
		*triVIndex2 = circleSegment+vertexesAround*2 +1;
		// 6th triangle
		triVIndex0 = &triData[ (triIndex+5)*3 + 0 ];
		triVIndex1 = &triData[ (triIndex+5)*3 + 1 ];
		triVIndex2 = &triData[ (triIndex+5)*3 + 2 ];
		*triVIndex0 = circleSegment+vertexesAround*3;
		*triVIndex1 = circleSegment+vertexesAround*3 +1;
		*triVIndex2 = circleSegment+vertexesAround*2 +1;

		//////////////////////////////////////////////////////////////
		// 7th triangle (Right-side Tread to Left-side Tread)
		triVIndex0 = &triData[ (triIndex+6)*3 + 0 ];
		triVIndex1 = &triData[ (triIndex+6)*3 + 1 ];
		triVIndex2 = &triData[ (triIndex+6)*3 + 2 ];
		*triVIndex0 = circleSegment+vertexesAround*3;
		*triVIndex1 = circleSegment+vertexesAround*4;
		*triVIndex2 = circleSegment+vertexesAround*3 +1;
		// 8th triangle
		triVIndex0 = &triData[ (triIndex+7)*3 + 0 ];
		triVIndex1 = &triData[ (triIndex+7)*3 + 1 ];
		triVIndex2 = &triData[ (triIndex+7)*3 + 2 ];
		*triVIndex0 = circleSegment+vertexesAround*4;
		*triVIndex1 = circleSegment+vertexesAround*4 +1;
		*triVIndex2 = circleSegment+vertexesAround*3 +1;
		/////////////////////////////////////////////////////////////


		// 9th triangle (Left-side - Tread to Shoulder)
		triVIndex0 = &triData[ (triIndex+8)*3 + 0 ];
		triVIndex1 = &triData[ (triIndex+8)*3 + 1 ];
		triVIndex2 = &triData[ (triIndex+8)*3 + 2 ];
		*triVIndex0 = circleSegment+vertexesAround*4;
		*triVIndex1 = circleSegment+vertexesAround*5;
		*triVIndex2 = circleSegment+vertexesAround*4 +1;
		// 10th triangle
		triVIndex0 = &triData[ (triIndex+9)*3 + 0 ];
		triVIndex1 = &triData[ (triIndex+9)*3 + 1 ];
		triVIndex2 = &triData[ (triIndex+9)*3 + 2 ];
		*triVIndex0 = circleSegment+vertexesAround*5;
		*triVIndex1 = circleSegment+vertexesAround*5 +1;
		*triVIndex2 = circleSegment+vertexesAround*4 +1;


		// 11th triangle (Left-side - Shoulder to Sidewall)
		triVIndex0 = &triData[ (triIndex+10)*3 + 0 ];
		triVIndex1 = &triData[ (triIndex+10)*3 + 1 ];
		triVIndex2 = &triData[ (triIndex+10)*3 + 2 ];
		*triVIndex0 = circleSegment+vertexesAround*5;
		*triVIndex1 = circleSegment+vertexesAround*6;
		*triVIndex2 = circleSegment+vertexesAround*5 +1;
		// 12th triangle
		triVIndex0 = &triData[ (triIndex+11)*3 + 0 ];
		triVIndex1 = &triData[ (triIndex+11)*3 + 1 ];
		triVIndex2 = &triData[ (triIndex+11)*3 + 2 ];
		*triVIndex0 = circleSegment+vertexesAround*6;
		*triVIndex1 = circleSegment+vertexesAround*6 +1;
		*triVIndex2 = circleSegment+vertexesAround*5 +1;

		// 13th triangle (Left-side - Sidewall to Inner)
		triVIndex0 = &triData[ (triIndex+12)*3 + 0 ];
		triVIndex1 = &triData[ (triIndex+12)*3 + 1 ];
		triVIndex2 = &triData[ (triIndex+12)*3 + 2 ];
		*triVIndex0 = circleSegment+vertexesAround*6;
		*triVIndex1 = circleSegment+vertexesAround*7;
		*triVIndex2 = circleSegment+vertexesAround*6 +1;
		// 14th triangle
		triVIndex0 = &triData[ (triIndex+13)*3 + 0 ];
		triVIndex1 = &triData[ (triIndex+13)*3 + 1 ];
		triVIndex2 = &triData[ (triIndex+13)*3 + 2 ];
		*triVIndex0 = circleSegment+vertexesAround*7;
		*triVIndex1 = circleSegment+vertexesAround*7 +1;
		*triVIndex2 = circleSegment+vertexesAround*6 +1;

		triIndex +=14;
	}


	///////////////////////////////////////////////////////////
	//
	// UV Coordinates
	//
	// the tread pattern should always take up the middle 33%
	// of the texture
	// tried writting distance calculation support to get the
	// sides of the tire to always have even xyz <-> uv space
	// but it was sorta tedious.  and this hard coded approach
	// insures any tire texture will work ok on any size tire.
	// though wider tires will experience more tread stretching
	// unless they also have a texture with a more appropriate
	// resolution.
	unsigned int texCoordFloats = vertexCount * 2;
	float *texData = new float [ texCoordFloats ];


	for ( unsigned int uvl=0 ; uvl< vertexCount ; uvl++ )
	{
		// U coord

		float *u = &texData[ uvl * 2 ];
		*u = uvl % vertexesAround;
		*u = *u / segmentsAround;

		float *v = &texData[ uvl * 2 + 1 ];


		// *v = floor ( uvl * + 1 / vertexesAround );
		// *v = *v / (vertexRings-1);

		if ( uvl < vertexesAround*1 )
			*v = 0.00f;
		else if ( uvl < vertexesAround*2 )
			*v = 0.10f;
		else if ( uvl < vertexesAround*3 )
			*v = 0.25f;
		else if ( uvl < vertexesAround*4 )
			*v = 0.333333333f;
		else if ( uvl < vertexesAround*5 )
			*v = 0.666666666f;
		else if ( uvl < vertexesAround*6 )
			*v = 0.75f;
		else if ( uvl < vertexesAround*7 )
			*v = 0.90f;
		else if ( uvl < vertexesAround*8 )
			*v = 1.00f;
		else
		{
			// shouldn't be able to get here
			// maybe put an error message here
			//  "tiregen error: accessing code that shouldn't be reachable"
			//*v = 0.00f;
		}


		// so for the very last segment it will need something a little special
		// otherwise it draws the entire texture in that tiny last segment space
		/*if ( (uvl % segmentsAround == 0) && (uvl > vertexRings * 8) )
		{
			*u = 1.0f;
		}*/

	}










	//////////////////////////////////////////////
	// build some vertex normals
	float *normalData = new float[vertexFloatCount];


    MATHVECTOR <float, 3> tri1Edge;        // one of the edges of a triangle that goes around the tire's circle
    MATHVECTOR <float, 3> tri2Edge;        // one of the edges of a triangle that goes around the tire's circle
    MATHVECTOR <float, 3> triUpEdge;       // one of the edges that wraps around the tire's tread which both faces share, not used on the last vertex ring
    MATHVECTOR <float, 3> triDownEdge;     // the other edges that wraps around the tire's tread which both faces share, not used on the first vertex ring

	for (unsigned int nlv=0 ; nlv<vertexCount ; nlv++)
	{

		/*// one way, not too too bad, but not accurate
		// this one is messed up
		normalData[nlv*3 + 0] = vertexData[nlv*3 +0] * 0.15f;
		// these other 2 are actually in the correct direction since its a cylinder
		normalData[nlv*3 + 1] = vertexData[nlv*3 + 1] * 0.15f;
		normalData[nlv*3 + 2] = vertexData[nlv*3 + 2] * 0.15f;
		continue;*/




        // this is gonna be grizzly
        if ( nlv < vertexesAround*1 )       // first ring of vertexes
		{
            ///////////////////////////////////////////////////
            // first ring of vertexes
		    if ((nlv % vertexesAround) == 0 )           // first vertex
		    {
                tri1Edge.Set(
                    vertexData[nlv*3   ] - vertexData[(nlv+1)*3   ],
                    vertexData[nlv*3 +1] - vertexData[(nlv+1)*3 +1],
                    vertexData[nlv*3 +2] - vertexData[(nlv+1)*3 +2]
                    );

                tri2Edge.Set(
                    vertexData[nlv*3   ] - vertexData[(nlv+segmentsAround-1)*3   ],
                    vertexData[nlv*3 +1] - vertexData[(nlv+segmentsAround-1)*3 +1],
                    vertexData[nlv*3 +2] - vertexData[(nlv+segmentsAround-1)*3 +2]
                    );

                triUpEdge.Set(
                    vertexData[nlv*3   ] - vertexData[(nlv+segmentsAround+1)*3   ],
                    vertexData[nlv*3 +1] - vertexData[(nlv+segmentsAround+1)*3 +1],
                    vertexData[nlv*3 +2] - vertexData[(nlv+segmentsAround+1)*3 +2]
                    );
		    }
		    else if ((nlv % vertexesAround) == segmentsAround )          // first ring, last vertex
		    {
                tri1Edge.Set(
                    vertexData[nlv*3   ] - vertexData[(nlv-segmentsAround+1)*3   ],
                    vertexData[nlv*3 +1] - vertexData[(nlv-segmentsAround+1)*3 +1],
                    vertexData[nlv*3 +2] - vertexData[(nlv-segmentsAround+1)*3 +2]
                    );

                tri2Edge.Set(
                    vertexData[nlv*3   ] - vertexData[(nlv-1)*3   ],
                    vertexData[nlv*3 +1] - vertexData[(nlv-1)*3 +1],
                    vertexData[nlv*3 +2] - vertexData[(nlv-1)*3 +2]
                    );

                triUpEdge.Set(
                    vertexData[nlv*3   ] - vertexData[(nlv+segmentsAround+1)*3   ],
                    vertexData[nlv*3 +1] - vertexData[(nlv+segmentsAround+1)*3 +1],
                    vertexData[nlv*3 +2] - vertexData[(nlv+segmentsAround+1)*3 +2]
                    );
		    }
		    else                        // first ring, most vertexes
		    {

                tri1Edge.Set(
                    vertexData[nlv*3   ] - vertexData[(nlv+1)*3   ],
                    vertexData[nlv*3 +1] - vertexData[(nlv+1)*3 +1],
                    vertexData[nlv*3 +2] - vertexData[(nlv+1)*3 +2]
                    );

                tri2Edge.Set(
                    vertexData[nlv*3   ] - vertexData[(nlv-1)*3   ],
                    vertexData[nlv*3 +1] - vertexData[(nlv-1)*3 +1],
                    vertexData[nlv*3 +2] - vertexData[(nlv-1)*3 +2]
                    );

                triUpEdge.Set(
                    vertexData[nlv*3   ] - vertexData[(nlv+segmentsAround+1)*3   ],
                    vertexData[nlv*3 +1] - vertexData[(nlv+segmentsAround+1)*3 +1],
                    vertexData[nlv*3 +2] - vertexData[(nlv+segmentsAround+1)*3 +2]
                    );
		    }


		    MATHVECTOR <float, 3> faceNormal1 = triUpEdge.cross(tri1Edge);
            MATHVECTOR <float, 3> faceNormal2 = tri2Edge.cross(triUpEdge);

            MATHVECTOR <float, 3> vNormal = faceNormal1 + faceNormal2;
            vNormal = vNormal.Normalize();
            vNormal = vNormal * vertexNormalLength;

            normalData[nlv*3 + 0] = vNormal[0];
            normalData[nlv*3 + 1] = vNormal[1];
            normalData[nlv*3 + 2] = vNormal[2];

		}


		///////////////////////////////////////////////////
		// last ring of vertexes
		else if ( nlv >= vertexesAround*7 )
		{

		    if ((nlv % vertexesAround) == 0 )		    // last ring, first vertex
		    {
                tri1Edge.Set(
                    vertexData[nlv*3   ] - vertexData[(nlv+1)*3   ],
                    vertexData[nlv*3 +1] - vertexData[(nlv+1)*3 +1],
                    vertexData[nlv*3 +2] - vertexData[(nlv+1)*3 +2]
                    );

                tri2Edge.Set(
                    vertexData[nlv*3   ] - vertexData[(nlv+segmentsAround-1)*3   ],
                    vertexData[nlv*3 +1] - vertexData[(nlv+segmentsAround-1)*3 +1],
                    vertexData[nlv*3 +2] - vertexData[(nlv+segmentsAround-1)*3 +2]
                    );

                triUpEdge.Set(
                    vertexData[nlv*3   ] - vertexData[(nlv-segmentsAround-1)*3   ],
                    vertexData[nlv*3 +1] - vertexData[(nlv-segmentsAround-1)*3 +1],
                    vertexData[nlv*3 +2] - vertexData[(nlv-segmentsAround-1)*3 +2]
                    );
		    }
            else if ( nlv == vertexCount-1 )          // last ring, last vertex (last vertex in mesh)
		    {
                tri2Edge.Set(
                    vertexData[nlv*3   ] - vertexData[(nlv-1)*3   ],
                    vertexData[nlv*3 +1] - vertexData[(nlv-1)*3 +1],
                    vertexData[nlv*3 +2] - vertexData[(nlv-1)*3 +2]
                    );

                tri1Edge.Set(
                    vertexData[nlv*3   ] - vertexData[(nlv-segmentsAround+1)*3   ],
                    vertexData[nlv*3 +1] - vertexData[(nlv-segmentsAround+1)*3 +1],
                    vertexData[nlv*3 +2] - vertexData[(nlv-segmentsAround+1)*3 +2]
                    );

                triUpEdge.Set(
                    vertexData[nlv*3   ] - vertexData[(nlv-segmentsAround-1)*3   ],
                    vertexData[nlv*3 +1] - vertexData[(nlv-segmentsAround-1)*3 +1],
                    vertexData[nlv*3 +2] - vertexData[(nlv-segmentsAround-1)*3 +2]
                    );
		    }
		    else                                              // last ring, most vertexes
		    {

                tri2Edge.Set(
                    vertexData[nlv*3   ] - vertexData[(nlv+1)*3   ],
                    vertexData[nlv*3 +1] - vertexData[(nlv+1)*3 +1],
                    vertexData[nlv*3 +2] - vertexData[(nlv+1)*3 +2]
                    );

                tri1Edge.Set(
                    vertexData[nlv*3   ] - vertexData[(nlv-1)*3   ],
                    vertexData[nlv*3 +1] - vertexData[(nlv-1)*3 +1],
                    vertexData[nlv*3 +2] - vertexData[(nlv-1)*3 +2]
                    );

                triUpEdge.Set(
                    vertexData[nlv*3   ] - vertexData[(nlv+segmentsAround+1)*3   ],
                    vertexData[nlv*3 +1] - vertexData[(nlv+segmentsAround+1)*3 +1],
                    vertexData[nlv*3 +2] - vertexData[(nlv+segmentsAround+1)*3 +2]
                    );

		    }




            // all actual normal calculation takes place here
            MATHVECTOR <float, 3> faceNormal1 = tri1Edge.cross(triUpEdge);
            MATHVECTOR <float, 3> faceNormal2 = triUpEdge.cross(tri2Edge);

            MATHVECTOR <float, 3> vNormal = faceNormal1 + faceNormal2;
            vNormal = vNormal.Normalize();
            vNormal = vNormal * vertexNormalLength;

            normalData[nlv*3 + 0] = vNormal[0];
            normalData[nlv*3 + 1] = vNormal[1];
            normalData[nlv*3 + 2] = vNormal[2];




		}


        ///////////////////////////////////////////////////
        // this is for the majority of the rings
		else
		{
            ///////////////////////////////////////////////////
            // first vertex in ring
		    if ((nlv % vertexesAround) == 0 )
		    {
                tri1Edge.Set(
                    vertexData[nlv*3   ] - vertexData[(nlv+1)*3   ],
                    vertexData[nlv*3 +1] - vertexData[(nlv+1)*3 +1],
                    vertexData[nlv*3 +2] - vertexData[(nlv+1)*3 +2]
                    );

                tri2Edge.Set(
                    vertexData[nlv*3   ] - vertexData[(nlv+segmentsAround-1)*3   ],
                    vertexData[nlv*3 +1] - vertexData[(nlv+segmentsAround-1)*3 +1],
                    vertexData[nlv*3 +2] - vertexData[(nlv+segmentsAround-1)*3 +2]
                    );

                triUpEdge.Set(
                    vertexData[nlv*3   ] - vertexData[(nlv+segmentsAround+1)*3   ],
                    vertexData[nlv*3 +1] - vertexData[(nlv+segmentsAround+1)*3 +1],
                    vertexData[nlv*3 +2] - vertexData[(nlv+segmentsAround+1)*3 +2]
                    );

                triDownEdge.Set(
                    vertexData[(nlv-segmentsAround-1)*3   ] - vertexData[nlv*3   ],
                    vertexData[(nlv-segmentsAround-1)*3 +1] - vertexData[nlv*3 +1],
                    vertexData[(nlv-segmentsAround-1)*3 +2] - vertexData[nlv*3 +2]
                    );


		    }
		    else if ((nlv % vertexesAround ) == segmentsAround )          // last vertex in ring
		    {
                tri1Edge.Set(
                    vertexData[nlv*3   ] - vertexData[(nlv-segmentsAround+1)*3   ],
                    vertexData[nlv*3 +1] - vertexData[(nlv-segmentsAround+1)*3 +1],
                    vertexData[nlv*3 +2] - vertexData[(nlv-segmentsAround+1)*3 +2]
                    );

                tri2Edge.Set(
                    vertexData[nlv*3   ] - vertexData[(nlv-1)*3   ],
                    vertexData[nlv*3 +1] - vertexData[(nlv-1)*3 +1],
                    vertexData[nlv*3 +2] - vertexData[(nlv-1)*3 +2]
                    );

                triUpEdge.Set(
                    vertexData[nlv*3   ] - vertexData[(nlv+segmentsAround+1)*3   ],
                    vertexData[nlv*3 +1] - vertexData[(nlv+segmentsAround+1)*3 +1],
                    vertexData[nlv*3 +2] - vertexData[(nlv+segmentsAround+1)*3 +2]
                    );

                triDownEdge.Set(
                    vertexData[(nlv-segmentsAround-1)*3   ] - vertexData[nlv*3   ],
                    vertexData[(nlv-segmentsAround-1)*3 +1] - vertexData[nlv*3 +1],
                    vertexData[(nlv-segmentsAround-1)*3 +2] - vertexData[nlv*3 +2]
                    );

		    }
		    else                        // most vertexes
		    {

                tri1Edge.Set(
                    vertexData[nlv*3   ] - vertexData[(nlv+1)*3   ],
                    vertexData[nlv*3 +1] - vertexData[(nlv+1)*3 +1],
                    vertexData[nlv*3 +2] - vertexData[(nlv+1)*3 +2]
                    );

                tri2Edge.Set(
                    vertexData[nlv*3   ] - vertexData[(nlv-1)*3   ],
                    vertexData[nlv*3 +1] - vertexData[(nlv-1)*3 +1],
                    vertexData[nlv*3 +2] - vertexData[(nlv-1)*3 +2]
                    );

                triUpEdge.Set(
                    vertexData[nlv*3   ] - vertexData[(nlv+segmentsAround+1)*3   ],
                    vertexData[nlv*3 +1] - vertexData[(nlv+segmentsAround+1)*3 +1],
                    vertexData[nlv*3 +2] - vertexData[(nlv+segmentsAround+1)*3 +2]
                    );
                triDownEdge.Set(
                    vertexData[(nlv-segmentsAround-1)*3   ] - vertexData[nlv*3   ],
                    vertexData[(nlv-segmentsAround-1)*3 +1] - vertexData[nlv*3 +1],
                    vertexData[(nlv-segmentsAround-1)*3 +2] - vertexData[nlv*3 +2]
                    );


		    }


		    MATHVECTOR <float, 3> faceNormal1 = triUpEdge.cross(tri1Edge);
            MATHVECTOR <float, 3> faceNormal2 = tri2Edge.cross(triUpEdge);
            MATHVECTOR <float, 3> faceNormal3 = triDownEdge.cross(tri1Edge);
            MATHVECTOR <float, 3> faceNormal4 = tri2Edge.cross(triDownEdge);

            MATHVECTOR <float, 3> vNormal = faceNormal1 + faceNormal2 + faceNormal3 + faceNormal4;
            vNormal = vNormal.Normalize();
            vNormal = vNormal * vertexNormalLength;

            normalData[nlv*3 + 0] = vNormal[0];
            normalData[nlv*3 + 1] = vNormal[1];
            normalData[nlv*3 + 2] = vNormal[2];



		}






	}



	//////////////////////////////////////////////
	// VERTEXARRAY will copy this data
	tire->SetVertices(vertexData, vertexFloatCount);
	tire->SetFaces((int*)triData, triVIndexCount);
	tire->SetTexCoordSets(1);
	tire->SetTexCoords(0, texData, texCoordFloats);
	tire->SetNormals(normalData, vertexFloatCount);


	// free up the temp data
	delete vertexData;
	delete triData;
	delete texData;
	delete normalData;
}






}; //namespace
