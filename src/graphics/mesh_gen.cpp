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

#include "mesh_gen.h"
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

TireSpec::TireSpec() :
	sectionWidth(0.185f),
	aspectRatio(0.45f),
	rimDiameter(0.43f)
{
	set(sectionWidth, aspectRatio, rimDiameter);
}

void TireSpec::set(float sectionwidth, float aspectratio, float rimdiameter)
{
	segments = 32;
	sectionWidth = sectionwidth;
	aspectRatio = aspectratio;
	rimDiameter = rimdiameter;
	innerRadius = rimDiameter * 0.5f;
	innerWidth = sectionWidth * 0.75f;
	sidewallRadius = innerRadius + sectionWidth * aspectRatio * 0.5f;
	shoulderRadius = innerRadius + sectionWidth * aspectRatio * 0.9f;
	treadRadius = innerRadius + sectionWidth * aspectRatio * 1.0f;
	treadWidth = sectionWidth * 0.75f;
	sidewallBulge = innerWidth * 0.05f;
	shoulderBulge = treadWidth * 0.05f;
}

RimSpec::RimSpec() :
	sectionWidth(0.185f),
	aspectRatio(0.45f),
	rimDiameter(0.43f)
{
	set(sectionWidth, aspectRatio, rimDiameter);
}

void RimSpec::set(float sectionwidth, float aspectratio, float rimdiameter)
{
	segments = 32;
	sectionWidth = sectionwidth;
	aspectRatio = aspectratio;
	rimDiameter = rimdiameter;
	innerRadius = rimDiameter * 0.5f;
	innerWidth = sectionWidth * 0.75f;
	flangeOuterRadius = innerRadius * 1.08f;
	flangeInnerRadius = innerRadius - 0.01f;
	flangeOutsideWidth = sectionWidth * 0.75f;
	flangeInnerWidth = flangeOutsideWidth - 0.02f;
}

void CreateTire(VERTEXARRAY & tire, const TireSpec & p)
{
	// non-configurable parameters
	int vertexRings = 8;
	float angleIncrement = 360.0f / (float) p.segments;

	/////////////////////////////////////
	// vertices (temporary data)
	int vertexesAround = p.segments + 1;
	int vertexCount = vertexesAround * vertexRings;
	int vertexFloatCount = vertexCount * 3;	// * 3 cause there are 3 floats in a vertex
	std::vector<float> vertexData(vertexFloatCount);

	// Right-side, Inner Ring
	for (int lv=0 ; lv<vertexesAround ; lv++)
	{
		vertexData[(lv+vertexesAround * 0) * 3 + 0] = 1.0f * (p.innerWidth / 2.0f);
		vertexData[(lv+vertexesAround * 0) * 3 + 1] = p.innerRadius * cosD(angleIncrement * lv);
		vertexData[(lv+vertexesAround * 0) * 3 + 2] = p.innerRadius * sinD(angleIncrement * lv);
	}
	// Right-side, Sidewall Ring
	for (int lv=0 ; lv<vertexesAround ; lv++)
	{
		vertexData[(lv+vertexesAround * 1) * 3 + 0] = 1.0f * (p.treadWidth / 2.0f + p.sidewallBulge);
		vertexData[(lv+vertexesAround * 1) * 3 + 1] = p.sidewallRadius * cosD(angleIncrement * lv);
		vertexData[(lv+vertexesAround * 1) * 3 + 2] = p.sidewallRadius * sinD(angleIncrement * lv);
	}
	// Right-side, Shoulder Ring
	for (int lv=0 ; lv<vertexesAround ; lv++)
	{
		vertexData[(lv+vertexesAround * 2) * 3 + 0] = 1.0f * (p.treadWidth / 2.0f + p.shoulderBulge);
		vertexData[(lv+vertexesAround * 2) * 3 + 1] = p.shoulderRadius * cosD(angleIncrement * lv);
		vertexData[(lv+vertexesAround * 2) * 3 + 2] = p.shoulderRadius * sinD(angleIncrement * lv);
	}
	// Right-side, Tread Ring
	for (int lv=0 ; lv<vertexesAround ; lv++)
	{
		vertexData[(lv+vertexesAround * 3) * 3 + 0] = 1.0f * (p.treadWidth / 2.0f);
		vertexData[(lv+vertexesAround * 3) * 3 + 1] = p.treadRadius * cosD(angleIncrement * lv);
		vertexData[(lv+vertexesAround * 3) * 3 + 2] = p.treadRadius * sinD(angleIncrement * lv);
	}

	// Left-side, Tread Ring
	for (int lv=0 ; lv<vertexesAround ; lv++)
	{
		vertexData[(lv+vertexesAround * 4) * 3 + 0] = -1.0f * (p.treadWidth / 2.0f);
		vertexData[(lv+vertexesAround * 4) * 3 + 1] = p.treadRadius * cosD(angleIncrement * lv);
		vertexData[(lv+vertexesAround * 4) * 3 + 2] = p.treadRadius * sinD(angleIncrement * lv);
	}
	// Left-side, Shoulder Ring
	for (int lv=0 ; lv<vertexesAround ; lv++)
	{
		vertexData[(lv+vertexesAround * 5) * 3 + 0] = -1.0f * (p.treadWidth / 2.0f + p.shoulderBulge);
		vertexData[(lv+vertexesAround * 5) * 3 + 1] = p.shoulderRadius * cosD(angleIncrement * lv);
		vertexData[(lv+vertexesAround * 5) * 3 + 2] = p.shoulderRadius * sinD(angleIncrement * lv);
	}
	// Left-side, Sidewall Ring
	for (int lv=0 ; lv<vertexesAround ; lv++)
	{
		vertexData[(lv+vertexesAround * 6) * 3 + 0] = -1.0f * (p.treadWidth / 2.0f + p.sidewallBulge);
		vertexData[(lv+vertexesAround * 6) * 3 + 1] = p.sidewallRadius * cosD(angleIncrement * lv);
		vertexData[(lv+vertexesAround * 6) * 3 + 2] = p.sidewallRadius * sinD(angleIncrement * lv);
	}
	// Left-side, Inner Ring
	for (int lv=0 ; lv<vertexesAround ; lv++)
	{
		vertexData[(lv+vertexesAround * 7) * 3 + 0] = -1.0f * (p.innerWidth / 2.0f);
		vertexData[(lv+vertexesAround * 7) * 3 + 1] = p.innerRadius * cosD(angleIncrement * lv);
		vertexData[(lv+vertexesAround * 7) * 3 + 2] = p.innerRadius * sinD(angleIncrement * lv);
	}

	/////////////////////////////////////
	//  now let's build triangles
	int triVIndexCount = 2 * p.segments * (vertexRings-1) * 3;	// 2 * triangles make a square,   * 3 indexes in a triangle
	std::vector<int> triData(triVIndexCount);

	int triIndex = 0;
	for (int circleSegment=0; circleSegment<p.segments; circleSegment++)
	{
		// 1st triangle (Right-side - Inner to Sidewall)
		triData[(triIndex+0)*3 + 0 ] = circleSegment;
		triData[(triIndex+0)*3 + 1 ] = circleSegment+vertexesAround;
		triData[(triIndex+0)*3 + 2 ] = circleSegment+1;

		// 2nd triangle
		triData[(triIndex+1)*3 + 0 ] = circleSegment+vertexesAround;
		triData[(triIndex+1)*3 + 1 ] = circleSegment+vertexesAround+1;
		triData[(triIndex+1)*3 + 2 ] = circleSegment+1;

		// 3rd triangle (Right-side - Sidewall to shoulder)
		triData[(triIndex+2)*3 + 0 ] = circleSegment+vertexesAround*1;
		triData[(triIndex+2)*3 + 1 ] = circleSegment+vertexesAround*2;
		triData[(triIndex+2)*3 + 2 ] = circleSegment+vertexesAround*1+1;

		// 4th triangle
		triData[(triIndex+3)*3 + 0 ] = circleSegment+vertexesAround*2;
		triData[(triIndex+3)*3 + 1 ] = circleSegment+vertexesAround*2+1;
		triData[(triIndex+3)*3 + 2 ] = circleSegment+vertexesAround*1+1;

		// 5th triangle (Right-side - Shoulder to Tread)
		triData[(triIndex+4)*3 + 0 ] = circleSegment+vertexesAround*2;
		triData[(triIndex+4)*3 + 1 ] = circleSegment+vertexesAround*3;
		triData[(triIndex+4)*3 + 2 ] = circleSegment+vertexesAround*2+1;

		// 6th triangle
		triData[(triIndex+5)*3 + 0 ] = circleSegment+vertexesAround*3;
		triData[(triIndex+5)*3 + 1 ] = circleSegment+vertexesAround*3+1;
		triData[(triIndex+5)*3 + 2 ] = circleSegment+vertexesAround*2+1;

		//////////////////////////////////////////////////////////////
		// 7th triangle (Right-side Tread to Left-side Tread)
		triData[(triIndex+6)*3 + 0 ] = circleSegment+vertexesAround*3;
		triData[(triIndex+6)*3 + 1 ] = circleSegment+vertexesAround*4;
		triData[(triIndex+6)*3 + 2 ] = circleSegment+vertexesAround*3+1;

		// 8th triangle
		triData[(triIndex+7)*3 + 0 ] = circleSegment+vertexesAround*4;
		triData[(triIndex+7)*3 + 1 ] = circleSegment+vertexesAround*4+1;
		triData[(triIndex+7)*3 + 2 ] = circleSegment+vertexesAround*3+1;

		/////////////////////////////////////////////////////////////
		// 9th triangle (Left-side - Tread to Shoulder)
		triData[(triIndex+8)*3 + 0 ] = circleSegment+vertexesAround*4;
		triData[(triIndex+8)*3 + 1 ] = circleSegment+vertexesAround*5;
		triData[(triIndex+8)*3 + 2 ] = circleSegment+vertexesAround*4+1;

		// 10th triangle
		triData[(triIndex+9)*3 + 0 ] = circleSegment+vertexesAround*5;
		triData[(triIndex+9)*3 + 1 ] = circleSegment+vertexesAround*5+1;
		triData[(triIndex+9)*3 + 2 ] = circleSegment+vertexesAround*4+1;

		// 11th triangle (Left-side - Shoulder to Sidewall)
		triData[(triIndex+10)*3 + 0 ] = circleSegment+vertexesAround*5;
		triData[(triIndex+10)*3 + 1 ] = circleSegment+vertexesAround*6;
		triData[(triIndex+10)*3 + 2 ] = circleSegment+vertexesAround*5+1;

		// 12th triangle
		triData[(triIndex+11)*3 + 0 ] = circleSegment+vertexesAround*6;
		triData[(triIndex+11)*3 + 1 ] = circleSegment+vertexesAround*6+1;
		triData[(triIndex+11)*3 + 2 ] = circleSegment+vertexesAround*5+1;

		// 13th triangle (Left-side - Sidewall to Inner)
		triData[(triIndex+12)*3 + 0 ] = circleSegment+vertexesAround*6;
		triData[(triIndex+12)*3 + 1 ] = circleSegment+vertexesAround*7;
		triData[(triIndex+12)*3 + 2 ] = circleSegment+vertexesAround*6+1;

		// 14th triangle
		triData[(triIndex+13)*3 + 0 ] = circleSegment+vertexesAround*7;
		triData[(triIndex+13)*3 + 1 ] = circleSegment+vertexesAround*7+1;
		triData[(triIndex+13)*3 + 2 ] = circleSegment+vertexesAround*6+1;

		triIndex +=14;
	}

	///////////////////////////////////////////////////////////
	//
	// Texture Coordinates
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
	int texCoordFloats = vertexCount * 2;
	std::vector<float> texData(texCoordFloats);

	for ( int uvl=0 ; uvl< vertexCount ; uvl++ )
	{
		// U coord
		float u = uvl % vertexesAround;
		u = u / p.segments;
        texData[ uvl * 2 ] = u;

		// V coord
		float v = 0.00f;
		// v = floor ( uvl * + 1 / vertexesAround );
		// v = v / (vertexRings-1);
		if ( uvl < vertexesAround*1 )
			v = 0.00f;
		else if ( uvl < vertexesAround*2 )
			v = 0.10f;
		else if ( uvl < vertexesAround*3 )
			v = 0.25f;
		else if ( uvl < vertexesAround*4 )
			v = 0.333333333f;
		else if ( uvl < vertexesAround*5 )
			v = 0.666666666f;
		else if ( uvl < vertexesAround*6 )
			v = 0.75f;
		else if ( uvl < vertexesAround*7 )
			v = 0.90f;
		else if ( uvl < vertexesAround*8 )
			v = 1.00f;
		else
		{
			// shouldn't be able to get here
			// maybe put an error message here
			//  "tiregen error: accessing code that shouldn't be reachable"
			assert(0);
		}
		texData[ uvl * 2 + 1 ] = v;

		// so for the very last segment it will need something a little special
		// otherwise it draws the entire texture in that tiny last segment space
		/*if ( (uvl % p.segments == 0) && (uvl > vertexRings * 8) )
		{
			*u = 1.0f;
		}*/
	}

	//////////////////////////////////////////////
	// build some vertex normals
	std::vector<float> normalData;
	normalData.resize(vertexFloatCount);

    MATHVECTOR <float, 3> tri1Edge;        // one of the edges of a triangle that goes around the tire's circle
    MATHVECTOR <float, 3> tri2Edge;        // one of the edges of a triangle that goes around the tire's circle
    MATHVECTOR <float, 3> triUpEdge;       // one of the edges that wraps around the tire's tread which both faces share, not used on the last vertex ring
    MATHVECTOR <float, 3> triDownEdge;     // the other edges that wraps around the tire's tread which both faces share, not used on the first vertex ring

	for (int nlv=0 ; nlv<vertexCount ; nlv++)
	{
		/*// one way, not too bad, but not accurate
		// this one is messed up
		normalData[nlv*3 + 0] = vertexData[nlv*3 +0] * 0.15f;  //0.05f;
		// these other 2 are actually in the correct direction since its a cylinder
		normalData[nlv*3 + 1] = vertexData[nlv*3 + 1] * 0.15f;
		normalData[nlv*3 + 2] = vertexData[nlv*3 + 2] * 0.15f;*/

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
                    vertexData[nlv*3   ] - vertexData[(nlv+p.segments-1)*3   ],
                    vertexData[nlv*3 +1] - vertexData[(nlv+p.segments-1)*3 +1],
                    vertexData[nlv*3 +2] - vertexData[(nlv+p.segments-1)*3 +2]
                    );

                triUpEdge.Set(
                    vertexData[nlv*3   ] - vertexData[(nlv+p.segments+1)*3   ],
                    vertexData[nlv*3 +1] - vertexData[(nlv+p.segments+1)*3 +1],
                    vertexData[nlv*3 +2] - vertexData[(nlv+p.segments+1)*3 +2]
                    );
		    }
		    else if ((nlv % vertexesAround) == p.segments)          // first ring, last vertex
		    {
                tri1Edge.Set(
                    vertexData[nlv*3   ] - vertexData[(nlv-p.segments+1)*3   ],
                    vertexData[nlv*3 +1] - vertexData[(nlv-p.segments+1)*3 +1],
                    vertexData[nlv*3 +2] - vertexData[(nlv-p.segments+1)*3 +2]
                    );

                tri2Edge.Set(
                    vertexData[nlv*3   ] - vertexData[(nlv-1)*3   ],
                    vertexData[nlv*3 +1] - vertexData[(nlv-1)*3 +1],
                    vertexData[nlv*3 +2] - vertexData[(nlv-1)*3 +2]
                    );

                triUpEdge.Set(
                    vertexData[nlv*3   ] - vertexData[(nlv+p.segments+1)*3   ],
                    vertexData[nlv*3 +1] - vertexData[(nlv+p.segments+1)*3 +1],
                    vertexData[nlv*3 +2] - vertexData[(nlv+p.segments+1)*3 +2]
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
                    vertexData[nlv*3   ] - vertexData[(nlv+p.segments+1)*3   ],
                    vertexData[nlv*3 +1] - vertexData[(nlv+p.segments+1)*3 +1],
                    vertexData[nlv*3 +2] - vertexData[(nlv+p.segments+1)*3 +2]
                    );

		        /*normalData[nlv*3 + 0] = vertexData[nlv*3 +0] * 0.015f;  //0.05f;
                // these other 2 are actually in the correct direction since its a cylinder
                normalData[nlv*3 + 1] = vertexData[nlv*3 + 1] * 0.015f;
                normalData[nlv*3 + 2] = vertexData[nlv*3 + 2] * 0.015f;
                continue;*/
		    }

		    MATHVECTOR <float, 3> faceNormal1 = triUpEdge.cross(tri1Edge);
            MATHVECTOR <float, 3> faceNormal2 = tri2Edge.cross(triUpEdge);

            MATHVECTOR <float, 3> vNormal = faceNormal1 + faceNormal2;
            vNormal = vNormal.Normalize();

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
                    vertexData[nlv*3   ] - vertexData[(nlv+p.segments)*3   ],
                    vertexData[nlv*3 +1] - vertexData[(nlv+p.segments)*3 +1],
                    vertexData[nlv*3 +2] - vertexData[(nlv+p.segments)*3 +2]
                    );

                triUpEdge.Set(
                    vertexData[nlv*3   ] - vertexData[(nlv-p.segments-1)*3   ],
                    vertexData[nlv*3 +1] - vertexData[(nlv-p.segments-1)*3 +1],
                    vertexData[nlv*3 +2] - vertexData[(nlv-p.segments-1)*3 +2]
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
                    vertexData[nlv*3   ] - vertexData[(nlv-p.segments)*3   ],
                    vertexData[nlv*3 +1] - vertexData[(nlv-p.segments)*3 +1],
                    vertexData[nlv*3 +2] - vertexData[(nlv-p.segments)*3 +2]
                    );

                triUpEdge.Set(
                    vertexData[nlv*3   ] - vertexData[(nlv-p.segments-1)*3   ],
                    vertexData[nlv*3 +1] - vertexData[(nlv-p.segments-1)*3 +1],
                    vertexData[nlv*3 +2] - vertexData[(nlv-p.segments-1)*3 +2]
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
                    vertexData[nlv*3   ] - vertexData[(nlv-p.segments+1)*3   ],
                    vertexData[nlv*3 +1] - vertexData[(nlv-p.segments+1)*3 +1],
                    vertexData[nlv*3 +2] - vertexData[(nlv-p.segments+1)*3 +2]
                    );

		        /*normalData[nlv*3 + 0] = vertexData[nlv*3 +0] * 0.015f;  //0.05f;
                // these other 2 are actually in the correct direction since its a cylinder
                normalData[nlv*3 + 1] = vertexData[nlv*3 + 1] * 0.015f;
                normalData[nlv*3 + 2] = vertexData[nlv*3 + 2] * 0.015f;
                continue;*/
		    }

            // all actual normal calculation takes place here
            MATHVECTOR <float, 3> faceNormal1 = tri1Edge.cross(triUpEdge);
            MATHVECTOR <float, 3> faceNormal2 = triUpEdge.cross(tri2Edge);

            MATHVECTOR <float, 3> vNormal = faceNormal1 + faceNormal2;
            vNormal = vNormal.Normalize();

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
                    vertexData[nlv*3   ] - vertexData[(nlv+p.segments-1)*3   ],
                    vertexData[nlv*3 +1] - vertexData[(nlv+p.segments-1)*3 +1],
                    vertexData[nlv*3 +2] - vertexData[(nlv+p.segments-1)*3 +2]
                    );

                triUpEdge.Set(
                    vertexData[nlv*3   ] - vertexData[(nlv+p.segments+1)*3   ],
                    vertexData[nlv*3 +1] - vertexData[(nlv+p.segments+1)*3 +1],
                    vertexData[nlv*3 +2] - vertexData[(nlv+p.segments+1)*3 +2]
                    );

                triDownEdge.Set(
                    vertexData[(nlv-p.segments-1)*3   ] - vertexData[nlv*3   ],
                    vertexData[(nlv-p.segments-1)*3 +1] - vertexData[nlv*3 +1],
                    vertexData[(nlv-p.segments-1)*3 +2] - vertexData[nlv*3 +2]
                    );

                /*normalData[nlv*3 + 0] = vertexData[nlv*3 +0] * 0.015f;  //0.05f;
                // these other 2 are actually in the correct direction since its a cylinder
                normalData[nlv*3 + 1] = vertexData[nlv*3 + 1] * 0.015f;
                normalData[nlv*3 + 2] = vertexData[nlv*3 + 2] * 0.015f;
                continue;*/
		    }
		    else if ((nlv % vertexesAround ) == p.segments )          // last vertex in ring
		    {
                tri1Edge.Set(
                    vertexData[nlv*3   ] - vertexData[(nlv-p.segments+1)*3   ],
                    vertexData[nlv*3 +1] - vertexData[(nlv-p.segments+1)*3 +1],
                    vertexData[nlv*3 +2] - vertexData[(nlv-p.segments+1)*3 +2]
                    );

                tri2Edge.Set(
                    vertexData[nlv*3   ] - vertexData[(nlv-1)*3   ],
                    vertexData[nlv*3 +1] - vertexData[(nlv-1)*3 +1],
                    vertexData[nlv*3 +2] - vertexData[(nlv-1)*3 +2]
                    );

                triUpEdge.Set(
                    vertexData[nlv*3   ] - vertexData[(nlv+p.segments+1)*3   ],
                    vertexData[nlv*3 +1] - vertexData[(nlv+p.segments+1)*3 +1],
                    vertexData[nlv*3 +2] - vertexData[(nlv+p.segments+1)*3 +2]
                    );

                triDownEdge.Set(
                    vertexData[(nlv-p.segments-1)*3   ] - vertexData[nlv*3   ],
                    vertexData[(nlv-p.segments-1)*3 +1] - vertexData[nlv*3 +1],
                    vertexData[(nlv-p.segments-1)*3 +2] - vertexData[nlv*3 +2]
                    );

                /*normalData[nlv*3 + 0] = vertexData[nlv*3 +0] * 0.015f;  //0.05f;
                // these other 2 are actually in the correct direction since its a cylinder
                normalData[nlv*3 + 1] = vertexData[nlv*3 + 1] * 0.015f;
                normalData[nlv*3 + 2] = vertexData[nlv*3 + 2] * 0.015f;
                continue;*/
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
                    vertexData[nlv*3   ] - vertexData[(nlv+p.segments+1)*3   ],
                    vertexData[nlv*3 +1] - vertexData[(nlv+p.segments+1)*3 +1],
                    vertexData[nlv*3 +2] - vertexData[(nlv+p.segments+1)*3 +2]
                    );
                triDownEdge.Set(
                    vertexData[(nlv-p.segments-1)*3   ] - vertexData[nlv*3   ],
                    vertexData[(nlv-p.segments-1)*3 +1] - vertexData[nlv*3 +1],
                    vertexData[(nlv-p.segments-1)*3 +2] - vertexData[nlv*3 +2]
                    );

		        /*normalData[nlv*3 + 0] = vertexData[nlv*3 +0] * 0.015f;  //0.05f;
                // these other 2 are actually in the correct direction since its a cylinder
                normalData[nlv*3 + 1] = vertexData[nlv*3 + 1] * 0.015f;
                normalData[nlv*3 + 2] = vertexData[nlv*3 + 2] * 0.015f;
                continue;*/
		    }

		    MATHVECTOR <float, 3> faceNormal1 = triUpEdge.cross(tri1Edge);
            MATHVECTOR <float, 3> faceNormal2 = tri2Edge.cross(triUpEdge);
            MATHVECTOR <float, 3> faceNormal3 = triDownEdge.cross(tri1Edge);
            MATHVECTOR <float, 3> faceNormal4 = tri2Edge.cross(triDownEdge);

            MATHVECTOR <float, 3> vNormal = faceNormal1 + faceNormal2 + faceNormal3 + faceNormal4;
            vNormal = vNormal.Normalize();

            normalData[nlv*3 + 0] = vNormal[0];
            normalData[nlv*3 + 1] = vNormal[1];
            normalData[nlv*3 + 2] = vNormal[2];
		}
		/*else
		{
            normalData[nlv*3 + 0] = vertexData[nlv*3 +0] * 0.015f;  //0.05f;
            // these other 2 are actually in the correct direction since its a cylinder
            normalData[nlv*3 + 1] = vertexData[nlv*3 + 1] * 0.015f;
            normalData[nlv*3 + 2] = vertexData[nlv*3 + 2] * 0.015f;
		}*/
	}

	tire.SetVertices(&vertexData.front(), vertexFloatCount);
	tire.SetFaces(&triData.front(), triVIndexCount);
	tire.SetTexCoordSets(1);
	tire.SetTexCoords(0, &texData.front(), texCoordFloats);
	tire.SetNormals(&normalData.front(), vertexFloatCount);
}

void CreateRim(VERTEXARRAY & rim, const RimSpec & p)
{
	// non-configurable parameters
	int vertexRings = 6;
	float angleIncrement = 360.0f / (float) p.segments;

	/////////////////////////////////////
	// vertices (temporary data)
	int vertexesAround = p.segments + 1;
	int vertexCount = vertexesAround * vertexRings;
	int vertexFloatCount = vertexCount * 3;	// * 3 cause there are 3 floats in a vertex
	std::vector<float> vertexData;
	vertexData.resize(vertexFloatCount);

	// Right-side bevel, outer lip
	for (int lv=0 ; lv<vertexesAround ; lv++)
	{
		vertexData[(lv+vertexesAround * 0) * 3 + 0] = 1.0f * (p.flangeOutsideWidth / 2.0f);
		vertexData[(lv+vertexesAround * 0) * 3 + 1] = p.flangeOuterRadius * cosD(angleIncrement * lv);
		vertexData[(lv+vertexesAround * 0) * 3 + 2] = p.flangeOuterRadius * sinD(angleIncrement * lv);
	}
	// Right-side bevel, inner lip
	for (int lv=0 ; lv<vertexesAround ; lv++)
	{
		vertexData[(lv+vertexesAround * 1) * 3 + 0] = 1.0f * (p.innerWidth / 2.0f);
		vertexData[(lv+vertexesAround * 1) * 3 + 1] = (p.innerRadius) * cosD(angleIncrement * lv);
		vertexData[(lv+vertexesAround * 1) * 3 + 2] = (p.innerRadius) * sinD(angleIncrement * lv);
	}

	// Right-side of main cylinder
	for (int lv=0 ; lv<vertexesAround ; lv++)
	{
		vertexData[(lv+vertexesAround * 2) * 3 + 0] = 1.0f * (p.innerWidth / 2.0f);
		vertexData[(lv+vertexesAround * 2) * 3 + 1] = (p.innerRadius) * cosD(angleIncrement * lv);
		vertexData[(lv+vertexesAround * 2) * 3 + 2] = (p.innerRadius) * sinD(angleIncrement * lv);
	}
	// Left-side of main cylinder,
	for (int lv=0 ; lv<vertexesAround ; lv++)
	{
		vertexData[(lv+vertexesAround * 3) * 3 + 0] = -1.0f * (p.innerWidth / 2.0f);
		vertexData[(lv+vertexesAround * 3) * 3 + 1] = (p.innerRadius) * cosD(angleIncrement * lv);
		vertexData[(lv+vertexesAround * 3) * 3 + 2] = (p.innerRadius) * sinD(angleIncrement * lv);
	}

	// Left-side bevel, inner lip
	for (int lv=0 ; lv<vertexesAround ; lv++)
	{
		vertexData[(lv+vertexesAround * 4) * 3 + 0] = -1.0f * (p.innerWidth / 2.0f);
		vertexData[(lv+vertexesAround * 4) * 3 + 1] = p.innerRadius * cosD(angleIncrement * lv);
		vertexData[(lv+vertexesAround * 4) * 3 + 2] = p.innerRadius * sinD(angleIncrement * lv);
	}
	// Left-side bevel, outer lip
	for (int lv=0 ; lv<vertexesAround ; lv++)
	{
		vertexData[(lv+vertexesAround * 5) * 3 + 0] = -1.0f * (p.flangeOutsideWidth / 2.0f);
		vertexData[(lv+vertexesAround * 5) * 3 + 1] = (p.flangeOuterRadius) * cosD(angleIncrement * lv);
		vertexData[(lv+vertexesAround * 5) * 3 + 2] = (p.flangeOuterRadius) * sinD(angleIncrement * lv);
	}

	/////////////////////////////////////
	//  build triangles
	//  different from the last one
    //  2 triangles * p.segments * 3 VertexIndexes * 3 completely separate tubes
	int triVIndexCount = 2 * p.segments * 3 * 3;
	std::vector<int> triData(triVIndexCount, 0);

	int triIndex = 0;
	for (int circleSegment=0 ; circleSegment<p.segments; circleSegment++)
	{
		// 1st triangle (Right-side - Inner to Sidewall)
		triData[(triIndex+0)*3 + 0 ] = circleSegment;
		triData[(triIndex+0)*3 + 1 ] = circleSegment+1;
		triData[(triIndex+0)*3 + 2 ] = circleSegment+vertexesAround;

		// 2nd triangle
		triData[(triIndex+1)*3 + 0 ] = circleSegment+vertexesAround;
		triData[(triIndex+1)*3 + 1 ] = circleSegment+1;
		triData[(triIndex+1)*3 + 2 ] = circleSegment+vertexesAround+1;

		triIndex +=2;
	}

	for (int circleSegment=0 ; circleSegment<p.segments; circleSegment++)
	{
		// 1st triangle (Right-side - Inner to Sidewall)
		triData[(triIndex+0)*3 + 0 ] = (2*vertexesAround)+circleSegment;
		triData[(triIndex+0)*3 + 1 ] = (2*vertexesAround)+circleSegment+1;
		triData[(triIndex+0)*3 + 2 ] = (2*vertexesAround)+circleSegment+vertexesAround;

		// 2nd triangle
		triData[(triIndex+1)*3 + 0 ] = (2*vertexesAround)+circleSegment+vertexesAround;
		triData[(triIndex+1)*3 + 1 ] = (2*vertexesAround)+circleSegment+1;
		triData[(triIndex+1)*3 + 2 ] = (2*vertexesAround)+circleSegment+vertexesAround+1;

		triIndex +=2;
	}

	for (int circleSegment=0 ; circleSegment<p.segments; circleSegment++)
	{
		// 1st triangle (Right-side - Inner to Sidewall)
		triData[(triIndex+0)*3 + 0 ] = (4*vertexesAround)+circleSegment+0;
		triData[(triIndex+0)*3 + 1 ] = (4*vertexesAround)+circleSegment+1;
		triData[(triIndex+0)*3 + 2 ] = (4*vertexesAround)+circleSegment+vertexesAround;

		// 2nd triangle
		triData[(triIndex+1)*3 + 0 ] = (4*vertexesAround)+circleSegment+vertexesAround;
		triData[(triIndex+1)*3 + 1 ] = (4*vertexesAround)+circleSegment+1;
		triData[(triIndex+1)*3 + 2 ] = (4*vertexesAround)+circleSegment+vertexesAround+1;

		triIndex +=2;
	}

    //////////////////////////////////////////////
    // Texture Coordinates
	int texCoordFloats = vertexCount * 2;
	std::vector<float> texData;
	texData.resize(texCoordFloats);

    // set them all to zero for the time being
    for ( int tlv=0 ; tlv< vertexCount ; tlv++ )
    {
        float u = tlv % vertexesAround;
		u = u / p.segments;
        texData[ tlv * 2 ] = u;

		float v = 0;
        if ( tlv < vertexesAround*1 )
        {
            v = 1.0 - 0.0f;
        }
        else if ( tlv < vertexesAround*2 )
        {
            v = 1.0 - 0.125f;
        }
        else if ( tlv < vertexesAround*3 )
        {
            v = 1.0 - 0.125f;
        }
        else if ( tlv < vertexesAround*4 )
        {
            v = 1.0 - 0.25f;
        }
        else if ( tlv < vertexesAround*5 )
        {
            v = 1.0 - 0.125f;
        }
        else            //if ( tlv < vertexesAround*6 )
        {
            v = 1.0 - 0.25f;
        }
        texData[ tlv * 2 + 1 ] = v;
    }

    ///////////////////////////////////////////////////////
    // time for normals
	std::vector<float> normalData;
	normalData.resize(vertexFloatCount);

    MATHVECTOR <float, 3> tri1Edge;        // one of the edges of a triangle that goes around the tire's circle
    MATHVECTOR <float, 3> tri2Edge;        // one of the edges of a triangle that goes around the tire's circle
    MATHVECTOR <float, 3> triUpEdge;

    for (int nlv=0 ; nlv<vertexCount ; nlv++)
    {
        normalData[nlv*3+0] = 0.1;
        normalData[nlv*3+1] = 0;
        normalData[nlv*3+2] = 0;
    }

    for (int nlv=0 ; nlv<vertexCount ; nlv++)
    {
        if ( nlv < vertexesAround*1 )       // first ring of vertexes
		{
		    if ((nlv % vertexesAround) == 0 )           // first vertex
		    {
                tri1Edge.Set(
                    vertexData[nlv*3   ] - vertexData[(nlv+1)*3   ],
                    vertexData[nlv*3 +1] - vertexData[(nlv+1)*3 +1],
                    vertexData[nlv*3 +2] - vertexData[(nlv+1)*3 +2]
                    );

                tri2Edge.Set(
                    vertexData[nlv*3   ] - vertexData[(nlv+p.segments-1)*3   ],
                    vertexData[nlv*3 +1] - vertexData[(nlv+p.segments-1)*3 +1],
                    vertexData[nlv*3 +2] - vertexData[(nlv+p.segments-1)*3 +2]
                    );

                triUpEdge.Set(
                    vertexData[nlv*3   ] - vertexData[(nlv+p.segments+1)*3   ],
                    vertexData[nlv*3 +1] - vertexData[(nlv+p.segments+1)*3 +1],
                    vertexData[nlv*3 +2] - vertexData[(nlv+p.segments+1)*3 +2]
                    );
		    }
		    else if ((nlv % vertexesAround) == p.segments )          // first ring, last vertex
		    {
                tri1Edge.Set(
                    vertexData[nlv*3   ] - vertexData[(nlv-p.segments+1)*3   ],
                    vertexData[nlv*3 +1] - vertexData[(nlv-p.segments+1)*3 +1],
                    vertexData[nlv*3 +2] - vertexData[(nlv-p.segments+1)*3 +2]
                    );

                tri2Edge.Set(
                    vertexData[nlv*3   ] - vertexData[(nlv-1)*3   ],
                    vertexData[nlv*3 +1] - vertexData[(nlv-1)*3 +1],
                    vertexData[nlv*3 +2] - vertexData[(nlv-1)*3 +2]
                    );

                triUpEdge.Set(
                    vertexData[nlv*3   ] - vertexData[(nlv+p.segments+1)*3   ],
                    vertexData[nlv*3 +1] - vertexData[(nlv+p.segments+1)*3 +1],
                    vertexData[nlv*3 +2] - vertexData[(nlv+p.segments+1)*3 +2]
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
                    vertexData[nlv*3   ] - vertexData[(nlv+p.segments+1)*3   ],
                    vertexData[nlv*3 +1] - vertexData[(nlv+p.segments+1)*3 +1],
                    vertexData[nlv*3 +2] - vertexData[(nlv+p.segments+1)*3 +2]
                    );
		    }

            MATHVECTOR <float, 3> faceNormal1 = tri1Edge.cross(triUpEdge);
            MATHVECTOR <float, 3> faceNormal2 = triUpEdge.cross(tri2Edge);

            MATHVECTOR <float, 3> vNormal = faceNormal1 + faceNormal2;
            vNormal = vNormal.Normalize();

            normalData[nlv*3 + 0] = vNormal[0];
            normalData[nlv*3 + 1] = vNormal[1];
            normalData[nlv*3 + 2] = vNormal[2];
		}
        else if ( nlv < vertexesAround *2 )
        {
            // since we want a hard edge here we can just copy the normals from the previous decision tree
            normalData[nlv*3 + 0] = normalData[(nlv-vertexesAround) *3 +0];
            normalData[nlv*3 + 1] = normalData[(nlv-vertexesAround) *3 +1];
            normalData[nlv*3 + 2] = normalData[(nlv-vertexesAround) *3 +2];
        }
        // now all the vertexes on the inside (both rings of them)
        else if ( nlv < vertexesAround *4 )
        {
            MATHVECTOR <float,3> iNormal;
            iNormal.Set( 0, vertexData[nlv*3 + 1] , vertexData[nlv*3 + 2]  );
            iNormal = iNormal * -1.0f;
            iNormal = iNormal.Normalize();

            normalData[nlv*3 + 0 ] = iNormal[0];
            normalData[nlv*3 + 1 ] = iNormal[1];
            normalData[nlv*3 + 2 ] = iNormal[2];
        }
		// first ring of vertexes
        else if ( nlv < vertexesAround*5 )
		{
		    if ((nlv % vertexesAround) == 0 )           // first vertex
		    {
                tri1Edge.Set(
                    vertexData[nlv*3   ] - vertexData[(nlv+1)*3   ],
                    vertexData[nlv*3 +1] - vertexData[(nlv+1)*3 +1],
                    vertexData[nlv*3 +2] - vertexData[(nlv+1)*3 +2]
                    );

                tri2Edge.Set(
                    vertexData[nlv*3   ] - vertexData[(nlv+p.segments-1)*3   ],
                    vertexData[nlv*3 +1] - vertexData[(nlv+p.segments-1)*3 +1],
                    vertexData[nlv*3 +2] - vertexData[(nlv+p.segments-1)*3 +2]
                    );

                triUpEdge.Set(
                    vertexData[nlv*3   ] - vertexData[(nlv+p.segments+1)*3   ],
                    vertexData[nlv*3 +1] - vertexData[(nlv+p.segments+1)*3 +1],
                    vertexData[nlv*3 +2] - vertexData[(nlv+p.segments+1)*3 +2]
                    );
		    }
		    else if ((nlv % vertexesAround) == p.segments )          // first ring, last vertex
		    {
                tri1Edge.Set(
                    vertexData[nlv*3   ] - vertexData[(nlv-p.segments+1)*3   ],
                    vertexData[nlv*3 +1] - vertexData[(nlv-p.segments+1)*3 +1],
                    vertexData[nlv*3 +2] - vertexData[(nlv-p.segments+1)*3 +2]
                    );

                tri2Edge.Set(
                    vertexData[nlv*3   ] - vertexData[(nlv-1)*3   ],
                    vertexData[nlv*3 +1] - vertexData[(nlv-1)*3 +1],
                    vertexData[nlv*3 +2] - vertexData[(nlv-1)*3 +2]
                    );

                triUpEdge.Set(
                    vertexData[nlv*3   ] - vertexData[(nlv+p.segments+1)*3   ],
                    vertexData[nlv*3 +1] - vertexData[(nlv+p.segments+1)*3 +1],
                    vertexData[nlv*3 +2] - vertexData[(nlv+p.segments+1)*3 +2]
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
                    vertexData[nlv*3   ] - vertexData[(nlv+p.segments+1)*3   ],
                    vertexData[nlv*3 +1] - vertexData[(nlv+p.segments+1)*3 +1],
                    vertexData[nlv*3 +2] - vertexData[(nlv+p.segments+1)*3 +2]
                    );
		    }

            MATHVECTOR <float, 3> faceNormal1 = tri1Edge.cross(triUpEdge);
            MATHVECTOR <float, 3> faceNormal2 = triUpEdge.cross(tri2Edge);

            MATHVECTOR <float, 3> vNormal = faceNormal1 + faceNormal2;
            vNormal = vNormal.Normalize();

            normalData[nlv*3 + 0] = vNormal[0];
            normalData[nlv*3 + 1] = vNormal[1];
            normalData[nlv*3 + 2] = vNormal[2];
		}
        else if ( nlv < vertexesAround *6 )
        {
            // since we want a hard edge here we can just copy the normals from the previous decision tree
            normalData[nlv*3 + 0] = normalData[(nlv-vertexesAround) *3 +0];
            normalData[nlv*3 + 1] = normalData[(nlv-vertexesAround) *3 +1];
            normalData[nlv*3 + 2] = normalData[(nlv-vertexesAround) *3 +2];
        }
		else
		{
		    normalData[nlv*3+0] = vertexData[nlv*3+0] * 0.05f;
		    normalData[nlv*3+1] = vertexData[nlv*3+1] * 0.05f;
		    normalData[nlv*3+2] = vertexData[nlv*3+2] * 0.05f;
		}
    }

	rim.SetVertices(&vertexData.front(), vertexFloatCount);
	rim.SetFaces(&triData.front(), triVIndexCount);
	rim.SetTexCoordSets(1);
	rim.SetTexCoords(0, &texData.front(), texCoordFloats);
	rim.SetNormals(&normalData.front(), vertexFloatCount);

	if (p.hub)
	{
		VERTEXARRAY hub = *p.hub;
		hub.Scale(p.sectionWidth, p.rimDiameter, p.rimDiameter);
		hub.Translate(-0.5 * p.innerWidth, 0, 0);
		rim = rim + hub;
	}
}

//////////////////////////////////////////////////////////////////////
// Brake Rotor
void CreateRotor(VERTEXARRAY & rotor, float radius, float thickness)
{
    // tweak-able
    unsigned int segments = 32;
    float normalLength = 1.00f;

    unsigned int vertexesAround = segments + 1;
    float angleIncrement = 360.0f / segments ;

    unsigned int vertexCount = vertexesAround*4;            // the two caps have 1 extra vertex in the center.  each ring of the sides has one extra dupe vertex for the texture map to wrap correctly
    unsigned int vertexFloatCount = vertexCount *3;

    std::vector<float> vertexData(vertexFloatCount);

    // first cap, first vertex
    vertexData[0+0] = -0.5 * thickness;
    vertexData[0+1] = 0.0f;
    vertexData[0+2] = 0.0f;

    // first cap, most vertexes
	for (unsigned int vlv=0 ; vlv<segments ; vlv++)
	{
		float *x = &vertexData[3 + (vlv + vertexesAround*0) * 3 + 0];
		float *y = &vertexData[3 + (vlv + vertexesAround*0) * 3 + 1];
		float *z = &vertexData[3 + (vlv + vertexesAround*0) * 3 + 2];

		*x = -0.5f * thickness;
		*y = radius * cosD( angleIncrement * vlv);
		*z = radius * sinD( angleIncrement * vlv);
	}

    // strip in the center, first ring
    for (unsigned int vlv=0 ; vlv<vertexesAround ; vlv++)
    {
		float *x = &vertexData[(vlv + vertexesAround*1) * 3 + 0];
		float *y = &vertexData[(vlv + vertexesAround*1) * 3 + 1];
		float *z = &vertexData[(vlv + vertexesAround*1) * 3 + 2];

		*x = -0.5f * thickness + 0.00f;
		*y = radius * cosD( angleIncrement * vlv);
		*z = radius * sinD( angleIncrement * vlv);
    }
    // strip in the center, second ring
    for (unsigned int vlv=0 ; vlv<vertexesAround ; vlv++)
    {
		float *x = &vertexData[(vlv + vertexesAround*2) * 3 + 0];
		float *y = &vertexData[(vlv + vertexesAround*2) * 3 + 1];
		float *z = &vertexData[(vlv + vertexesAround*2) * 3 + 2];

		*x = 0.5f * thickness + 0.00f;
		*y = radius * cosD( angleIncrement * vlv);
		*z = radius * sinD( angleIncrement * vlv);
    }

    // last cap, most vertexes
	for (unsigned int vlv=0 ; vlv<segments ; vlv++)
	{
		float *x = &vertexData[(vlv + vertexesAround*3) * 3 + 0];
		float *y = &vertexData[(vlv + vertexesAround*3) * 3 + 1];
		float *z = &vertexData[(vlv + vertexesAround*3) * 3 + 2];

		*x = 0.5f * thickness + 0.00f;
		*y = radius * cosD( angleIncrement * vlv);
		*z = radius * sinD( angleIncrement * vlv);
	}

    // last cap, last vertex
    vertexData[(vertexCount-1)*3 +0] = 0.5 * thickness + 0.0f;
    vertexData[(vertexCount-1)*3 +1] = 0.0f;
    vertexData[(vertexCount-1)*3 +2] = 0.0f;

    //////////////////////////////////////////////////
    // triangles
    unsigned int trianglesPerCap = segments;
    unsigned int trianglesPerStrip = segments * 2;

	unsigned int triVIndexCount = (2*trianglesPerCap + 2*trianglesPerStrip) * 3;
	std::vector<int> triData(triVIndexCount);

	int triIndex = 0;
	int *triVIndex0;
	int *triVIndex1;
	int *triVIndex2;

    // clear all data - good for when building the following loops
    for (unsigned int tlv=0 ; tlv<triVIndexCount ; tlv++)
    {
        triData[tlv] = 0;
    }

    // first cap
    for ( unsigned int tlv=0 ; tlv<segments ; tlv++)
    {
        triVIndex0 = &triData[ tlv * 3 + 0];
        triVIndex1 = &triData[ tlv * 3 + 1];
        triVIndex2 = &triData[ tlv * 3 + 2];

        if (tlv == segments-1)      // last triangle
        {
            *triVIndex0 = 0;
            *triVIndex1 = 1;
            *triVIndex2 = segments;
        }
        else
        {
            *triVIndex0 = 0;
            *triVIndex1 = tlv+2;
            *triVIndex2 = tlv+1;
        }
    }

    // strip in the center
    triIndex = segments;
    for ( unsigned int tlv=0 ; tlv<segments ; tlv++ )
    {
            // 1st tri
            triVIndex0 = &triData[ triIndex * 3 + 0];
            triVIndex1 = &triData[ triIndex * 3 + 1];
            triVIndex2 = &triData[ triIndex * 3 + 2];

            *triVIndex0 = vertexesAround+tlv+0;
            *triVIndex1 = vertexesAround+tlv+1;
            *triVIndex2 = vertexesAround+tlv+segments+1;

            triIndex++;

            // 2nd tri
            triVIndex0 = &triData[ triIndex * 3 + 0];
            triVIndex1 = &triData[ triIndex * 3 + 1];
            triVIndex2 = &triData[ triIndex * 3 + 2];

            *triVIndex0 = vertexesAround+tlv+0+1;
            *triVIndex2 = vertexesAround+tlv+segments+1;
            *triVIndex1 = vertexesAround+tlv+segments+1+1;

            triIndex++;
    }

    triIndex+=2;            // skip the last two vertexes in the strip, cause we had to make them duplicates to get the texcoords in different spots

    // 2nd cap
    for ( unsigned int tlv=0 ; tlv<segments ; tlv++)
    {
        triVIndex0 = &triData[(triIndex + tlv) * 3 + 0];
        triVIndex1 = &triData[(triIndex + tlv) * 3 + 1];
        triVIndex2 = &triData[(triIndex + tlv) * 3 + 2];

        if (tlv == segments-1)      // last triangle
        {
            *triVIndex1 = triIndex*0 + vertexCount-1;
            *triVIndex0 = triIndex + 1;
            *triVIndex2 = triIndex + segments;
        }
        else
        {
            *triVIndex1 = triIndex*0 + vertexCount-1;
            *triVIndex0 = triIndex + tlv+2;
            *triVIndex2 = triIndex + tlv+1;
        }
    }

    ///////////////////////////////////////////
    // texture coordinates
    unsigned int texCoordFloats = vertexCount * 2;
    std::vector<float> texData(texCoordFloats);

    // first cap, first texcoord
    texData[0+0] = 7.0f / 16.0f;
    texData[0+1] = 7.0f / 16.0f;

    // first cap, most texcoords
	for (unsigned int vlv=0 ; vlv<segments ; vlv++)
	{
		float *u = &texData[2 + (vlv + vertexesAround*0) * 2 + 0];
		float *v = &texData[2 + (vlv + vertexesAround*0) * 2 + 1];

		*u = 0.5f  * cosD( angleIncrement * vlv) + 0.5f;
		*v = 0.5f  * sinD( angleIncrement * vlv) + 0.5f;

		*u = *u * 14.0f / 16.0f;
		*v = *v * 14.0f / 16.0f;

		/**u = *u + 0.5f / 16.0f;
		*v = *v + 0.5f / 16.0f;*/
	}

    // strip in the center, first ring
    for (unsigned int vlv=0 ; vlv<vertexesAround ; vlv++)
    {
		float *u = &texData[(vlv + vertexesAround*1) * 2 + 0];
		float *v = &texData[(vlv + vertexesAround*1) * 2 + 1];

		*u = (float)vlv / (float)segments;
		*v = 1.0f - 1.0f/8.0f;
    }
    // strip in the center, second ring
    for (unsigned int vlv=0 ; vlv<vertexesAround ; vlv++)
    {
		float *u = &texData[(vlv + vertexesAround*2) * 2 + 0];
		float *v = &texData[(vlv + vertexesAround*2) * 2 + 1];

		*u = (float)vlv / (float)segments;
		*v = 1.0f;
    }

    // last cap, most texcoords
	for (unsigned int vlv=0 ; vlv<segments ; vlv++)
	{
		float *u = &texData[(vlv + vertexesAround*3) * 2 + 0];
		float *v = &texData[(vlv + vertexesAround*3) * 2 + 1];

		*u = 0.5f  * cosD( angleIncrement * vlv) + 0.5f;
		*v = 0.5f  * sinD( angleIncrement * vlv) + 0.5f;

		*u = *u * 14.0f / 16.0f;
		*v = *v * 14.0f / 16.0f;

		/**u = *u + 0.5f / 16.0f;
		*v = *v + 0.5f / 16.0f;*/
	}

    // last cap, last texcoord
    texData[(vertexCount-1)*2 +0] = 7.0f / 16.0f;
    texData[(vertexCount-1)*2 +1] = 7.0f / 16.0f;

    /////////////////////////////////////////
    // finally vertex normals
    std::vector<float> normalData(vertexFloatCount);
    for ( unsigned int nlv=0 ; nlv<vertexCount ; nlv++ )
    {
        if ( nlv < vertexesAround )
        {
            normalData[nlv*3+0] = -1.0f * normalLength;
            normalData[nlv*3+1] = 0.0f;
            normalData[nlv*3+2] = 0.0f;
        }
        else if ( nlv < vertexesAround* 3 )
        {
            MATHVECTOR <float, 3> normal;
            normal.Set(0.0f, vertexData[nlv*3+1], vertexData[nlv*3+2]);
            normal = normal.Normalize();
            normal = normal * normalLength;

            normalData[nlv*3+0] = normal[0];
            normalData[nlv*3+1] = normal[1];
            normalData[nlv*3+2] = normal[2];
        }
        else
        {
            normalData[nlv*3+0] = 1.0f * normalLength;
            normalData[nlv*3+1] = 0.0f;
            normalData[nlv*3+2] = 0.0f;
        }
    }

	// init vertexarray
    rotor.SetVertices(&vertexData.front(), vertexFloatCount);
	rotor.SetFaces(&triData.front(), triVIndexCount);
	rotor.SetTexCoordSets(1);
	rotor.SetTexCoords(0, &texData.front(), texCoordFloats);
	rotor.SetNormals(&normalData.front(), vertexFloatCount);
}

} //namespace
