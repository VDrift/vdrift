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

namespace MeshGen
{





/////////////////////////////////////////////////////////////////////////////////////////////
// mg_tire
void mg_tire(VertexArray & tire, float sectionWidth_mm, float aspectRatio, float rimDiameter_in)
{
	// configurable parameters - set to defaults
	unsigned int segmentsAround = 32;
	float innerRadius = 0.65f;
	float innerWidth = 0.60f;

	float sidewallRadius = 0.75f;
	float sidewallBulge = 0.05f;		// bulge from tread

	float shoulderRadius = 0.95f;
	float shoulderBulge = 0.00f;		// bulge from tread

	float treadRadius = 1.00f;
	float treadWidth = 0.60f;

	float vertexNormalLength = 0.025f;






	// use this to build a tire based on the tire code passed to this function
	// if the code under this if doesn't happen, then the parameters are meaningless
	if (true)
	{
		// use function parameters - comment out this section
		float sectionWidth_m = sectionWidth_mm / 1000.0f;
		float rimRadius_m = rimDiameter_in * 0.0254f * 0.5;


		innerRadius = rimRadius_m;

        // fudge number: based on 15 inch rims on my car and truck - this number has to be synchronized with the one in wheel_edge
		innerRadius *= 1.08f;


		treadWidth = sectionWidth_m - (sectionWidth_m / 4.0f);     // subtract 1/8th from section width, to get the tread width (not really sure, looks ok though)


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
		if (aspectRatio > 1)    // then he gave us a whole number
		{
		    aspectRatio = aspectRatio / 100.0f;
		}


		// if he gave us a large number (specifically over 200) we need to assume that he told us the diameter of the tire in mm
		if (aspectRatio > 2)		// he gave us a mm diameter for the entire tire (instead of a % of the section width)
		{
		    treadRadius = aspectRatio / 2000.0f;
		}
		else		// otherwise: he gave us the normal percent
		{
		    treadRadius = sectionWidth_m * aspectRatio + rimRadius_m;

		}


		// fudge some values by what looks ok.
		innerWidth = treadWidth;

		sidewallBulge = innerWidth * 0.050f;
		shoulderBulge = treadWidth * 0.050f;

		float totalSidewallHeight = sectionWidth_m * aspectRatio;
		sidewallRadius = totalSidewallHeight/2.0f + rimRadius_m;
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
	std::vector<float> vertexData;
	vertexData.resize(vertexFloatCount);

	// Right-side, Inner Ring
	for (unsigned int lv=0 ; lv<vertexesAround ; lv++)
	{
		vertexData[(lv+vertexesAround * 0) * 3 + 0] = 1.0f * (innerWidth / 2.0f);
		vertexData[(lv+vertexesAround * 0) * 3 + 1] = innerRadius * cosD(angleIncrement * lv);
		vertexData[(lv+vertexesAround * 0) * 3 + 2] = innerRadius * sinD(angleIncrement * lv);
	}
	// Right-side, Sidewall Ring
	for (unsigned int lv=0 ; lv<vertexesAround ; lv++)
	{
		vertexData[(lv+vertexesAround * 1) * 3 + 0] = 1.0f * (treadWidth / 2.0f + sidewallBulge);
		vertexData[(lv+vertexesAround * 1) * 3 + 1] = sidewallRadius * cosD(angleIncrement * lv);
		vertexData[(lv+vertexesAround * 1) * 3 + 2] = sidewallRadius * sinD(angleIncrement * lv);
	}
	// Right-side, Shoulder Ring
	for (unsigned int lv=0 ; lv<vertexesAround ; lv++)
	{
		vertexData[(lv+vertexesAround * 2) * 3 + 0] = 1.0f * (treadWidth / 2.0f + shoulderBulge);
		vertexData[(lv+vertexesAround * 2) * 3 + 1] = shoulderRadius * cosD(angleIncrement * lv);
		vertexData[(lv+vertexesAround * 2) * 3 + 2] = shoulderRadius * sinD(angleIncrement * lv);
	}
	// Right-side, Tread Ring
	for (unsigned int lv=0 ; lv<vertexesAround ; lv++)
	{
		vertexData[(lv+vertexesAround * 3) * 3 + 0] = 1.0f * (treadWidth / 2.0f);
		vertexData[(lv+vertexesAround * 3) * 3 + 1] = treadRadius * cosD(angleIncrement * lv);
		vertexData[(lv+vertexesAround * 3) * 3 + 2] = treadRadius * sinD(angleIncrement * lv);
	}


	// Left-side, Tread Ring
	for (unsigned int lv=0 ; lv<vertexesAround ; lv++)
	{
		vertexData[(lv+vertexesAround * 4) * 3 + 0] = -1.0f * (treadWidth / 2.0f);
		vertexData[(lv+vertexesAround * 4) * 3 + 1] = treadRadius * cosD(angleIncrement * lv);
		vertexData[(lv+vertexesAround * 4) * 3 + 2] = treadRadius * sinD(angleIncrement * lv);
	}
	// Left-side, Shoulder Ring
	for (unsigned int lv=0 ; lv<vertexesAround ; lv++)
	{
		vertexData[(lv+vertexesAround * 5) * 3 + 0] = -1.0f * (treadWidth / 2.0f + shoulderBulge);
		vertexData[(lv+vertexesAround * 5) * 3 + 1] = shoulderRadius * cosD(angleIncrement * lv);
		vertexData[(lv+vertexesAround * 5) * 3 + 2] = shoulderRadius * sinD(angleIncrement * lv);
	}
	// Left-side, Sidewall Ring
	for (unsigned int lv=0 ; lv<vertexesAround ; lv++)
	{
		vertexData[(lv+vertexesAround * 6) * 3 + 0] = -1.0f * (treadWidth / 2.0f + sidewallBulge);
		vertexData[(lv+vertexesAround * 6) * 3 + 1] = sidewallRadius * cosD(angleIncrement * lv);
		vertexData[(lv+vertexesAround * 6) * 3 + 2] = sidewallRadius * sinD(angleIncrement * lv);
	}
	// Left-side, Inner Ring
	for (unsigned int lv=0 ; lv<vertexesAround ; lv++)
	{
		vertexData[(lv+vertexesAround * 7) * 3 + 0] = -1.0f * (innerWidth / 2.0f);
		vertexData[(lv+vertexesAround * 7) * 3 + 1] = innerRadius * cosD(angleIncrement * lv);
		vertexData[(lv+vertexesAround * 7) * 3 + 2] = innerRadius * sinD(angleIncrement * lv);
	}





	/////////////////////////////////////
	//
	//  now let's build triangles
	//
	unsigned int triVIndexCount = 2 * segmentsAround * (vertexRings-1) * 3;	// 2 * triangles make a square,   * 3 indexes in a triangle
	std::vector<unsigned int> triData;
	triData.resize(triVIndexCount);

	unsigned int triIndex = 0;
	for (unsigned int circleSegment=0; circleSegment<segmentsAround; circleSegment++)
	{
		// 1st triangle (Right-side - Inner to Sidewall)
		triData[ (triIndex+0)*3 + 0 ] = circleSegment;
		triData[ (triIndex+0)*3 + 1 ] = circleSegment+vertexesAround;
		triData[ (triIndex+0)*3 + 2 ] = circleSegment+1;

		// 2nd triangle
		triData[ (triIndex+1)*3 + 0 ] = circleSegment+vertexesAround;
		triData[ (triIndex+1)*3 + 1 ] = circleSegment+vertexesAround+1;
		triData[ (triIndex+1)*3 + 2 ] = circleSegment+1;

		// 3rd triangle (Right-side - Sidewall to shoulder)
		triData[ (triIndex+2)*3 + 0 ] = circleSegment+vertexesAround*1;
		triData[ (triIndex+2)*3 + 1 ] = circleSegment+vertexesAround*2;
		triData[ (triIndex+2)*3 + 2 ] = circleSegment+vertexesAround*1+1;

		// 4th triangle
		triData[ (triIndex+3)*3 + 0 ] = circleSegment+vertexesAround*2;
		triData[ (triIndex+3)*3 + 1 ] = circleSegment+vertexesAround*2+1;
		triData[ (triIndex+3)*3 + 2 ] = circleSegment+vertexesAround*1+1;

		// 5th triangle (Right-side - Shoulder to Tread)
		triData[ (triIndex+4)*3 + 0 ] = circleSegment+vertexesAround*2;
		triData[ (triIndex+4)*3 + 1 ] = circleSegment+vertexesAround*3;
		triData[ (triIndex+4)*3 + 2 ] = circleSegment+vertexesAround*2+1;

		// 6th triangle
		triData[ (triIndex+5)*3 + 0 ] = circleSegment+vertexesAround*3;
		triData[ (triIndex+5)*3 + 1 ] = circleSegment+vertexesAround*3+1;
		triData[ (triIndex+5)*3 + 2 ] = circleSegment+vertexesAround*2+1;

		//////////////////////////////////////////////////////////////
		// 7th triangle (Right-side Tread to Left-side Tread)
		triData[ (triIndex+6)*3 + 0 ] = circleSegment+vertexesAround*3;
		triData[ (triIndex+6)*3 + 1 ] = circleSegment+vertexesAround*4;
		triData[ (triIndex+6)*3 + 2 ] = circleSegment+vertexesAround*3+1;

		// 8th triangle
		triData[ (triIndex+7)*3 + 0 ] = circleSegment+vertexesAround*4;
		triData[ (triIndex+7)*3 + 1 ] = circleSegment+vertexesAround*4+1;
		triData[ (triIndex+7)*3 + 2 ] = circleSegment+vertexesAround*3+1;


		/////////////////////////////////////////////////////////////
		// 9th triangle (Left-side - Tread to Shoulder)
		triData[ (triIndex+8)*3 + 0 ] = circleSegment+vertexesAround*4;
		triData[ (triIndex+8)*3 + 1 ] = circleSegment+vertexesAround*5;
		triData[ (triIndex+8)*3 + 2 ] = circleSegment+vertexesAround*4+1;

		// 10th triangle
		triData[ (triIndex+9)*3 + 0 ] = circleSegment+vertexesAround*5;
		triData[ (triIndex+9)*3 + 1 ] = circleSegment+vertexesAround*5+1;
		triData[ (triIndex+9)*3 + 2 ] = circleSegment+vertexesAround*4+1;

		// 11th triangle (Left-side - Shoulder to Sidewall)
		triData[ (triIndex+10)*3 + 0 ] = circleSegment+vertexesAround*5;
		triData[ (triIndex+10)*3 + 1 ] = circleSegment+vertexesAround*6;
		triData[ (triIndex+10)*3 + 2 ] = circleSegment+vertexesAround*5+1;

		// 12th triangle
		triData[ (triIndex+11)*3 + 0 ] = circleSegment+vertexesAround*6;
		triData[ (triIndex+11)*3 + 1 ] = circleSegment+vertexesAround*6+1;
		triData[ (triIndex+11)*3 + 2 ] = circleSegment+vertexesAround*5+1;

		// 13th triangle (Left-side - Sidewall to Inner)
		triData[ (triIndex+12)*3 + 0 ] = circleSegment+vertexesAround*6;
		triData[ (triIndex+12)*3 + 1 ] = circleSegment+vertexesAround*7;
		triData[ (triIndex+12)*3 + 2 ] = circleSegment+vertexesAround*6+1;

		// 14th triangle
		triData[ (triIndex+13)*3 + 0 ] = circleSegment+vertexesAround*7;
		triData[ (triIndex+13)*3 + 1 ] = circleSegment+vertexesAround*7+1;
		triData[ (triIndex+13)*3 + 2 ] = circleSegment+vertexesAround*6+1;

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
	unsigned int texCoordFloats = vertexCount * 2;
	std::vector<float> texData;
	texData.resize(texCoordFloats);


	for ( unsigned int uvl=0 ; uvl< vertexCount ; uvl++ )
	{
		// U coord
		float u = uvl % vertexesAround;
		u = u / segmentsAround;
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
		/*if ( (uvl % segmentsAround == 0) && (uvl > vertexRings * 8) )
		{
			*u = 1.0f;
		}*/

	}








	//////////////////////////////////////////////
	// build some vertex normals
	std::vector<float> normalData;
	normalData.resize(vertexFloatCount);


    Vec3 tri1Edge;        // one of the edges of a triangle that goes around the tire's circle
    Vec3 tri2Edge;        // one of the edges of a triangle that goes around the tire's circle
    Vec3 triUpEdge;       // one of the edges that wraps around the tire's tread which both faces share, not used on the last vertex ring
    Vec3 triDownEdge;     // the other edges that wraps around the tire's tread which both faces share, not used on the first vertex ring

	for (unsigned int nlv=0 ; nlv<vertexCount ; nlv++)
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

		        /*normalData[nlv*3 + 0] = vertexData[nlv*3 +0] * 0.015f;  //0.05f;
                // these other 2 are actually in the correct direction since its a cylinder
                normalData[nlv*3 + 1] = vertexData[nlv*3 + 1] * 0.015f;
                normalData[nlv*3 + 2] = vertexData[nlv*3 + 2] * 0.015f;
                continue;*/

		    }


		    Vec3 faceNormal1 = triUpEdge.cross(tri1Edge);
            Vec3 faceNormal2 = tri2Edge.cross(triUpEdge);

            Vec3 vNormal = faceNormal1 + faceNormal2;
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
                    vertexData[nlv*3   ] - vertexData[(nlv+segmentsAround)*3   ],
                    vertexData[nlv*3 +1] - vertexData[(nlv+segmentsAround)*3 +1],
                    vertexData[nlv*3 +2] - vertexData[(nlv+segmentsAround)*3 +2]
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
                    vertexData[nlv*3   ] - vertexData[(nlv-segmentsAround)*3   ],
                    vertexData[nlv*3 +1] - vertexData[(nlv-segmentsAround)*3 +1],
                    vertexData[nlv*3 +2] - vertexData[(nlv-segmentsAround)*3 +2]
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
                    vertexData[nlv*3   ] - vertexData[(nlv-segmentsAround+1)*3   ],
                    vertexData[nlv*3 +1] - vertexData[(nlv-segmentsAround+1)*3 +1],
                    vertexData[nlv*3 +2] - vertexData[(nlv-segmentsAround+1)*3 +2]
                    );

		        /*normalData[nlv*3 + 0] = vertexData[nlv*3 +0] * 0.015f;  //0.05f;
                // these other 2 are actually in the correct direction since its a cylinder
                normalData[nlv*3 + 1] = vertexData[nlv*3 + 1] * 0.015f;
                normalData[nlv*3 + 2] = vertexData[nlv*3 + 2] * 0.015f;
                continue;*/

		    }




            // all actual normal calculation takes place here
            Vec3 faceNormal1 = tri1Edge.cross(triUpEdge);
            Vec3 faceNormal2 = triUpEdge.cross(tri2Edge);

            Vec3 vNormal = faceNormal1 + faceNormal2;
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


                /*normalData[nlv*3 + 0] = vertexData[nlv*3 +0] * 0.015f;  //0.05f;
                // these other 2 are actually in the correct direction since its a cylinder
                normalData[nlv*3 + 1] = vertexData[nlv*3 + 1] * 0.015f;
                normalData[nlv*3 + 2] = vertexData[nlv*3 + 2] * 0.015f;
                continue;*/

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
                    vertexData[nlv*3   ] - vertexData[(nlv+segmentsAround+1)*3   ],
                    vertexData[nlv*3 +1] - vertexData[(nlv+segmentsAround+1)*3 +1],
                    vertexData[nlv*3 +2] - vertexData[(nlv+segmentsAround+1)*3 +2]
                    );
                triDownEdge.Set(
                    vertexData[(nlv-segmentsAround-1)*3   ] - vertexData[nlv*3   ],
                    vertexData[(nlv-segmentsAround-1)*3 +1] - vertexData[nlv*3 +1],
                    vertexData[(nlv-segmentsAround-1)*3 +2] - vertexData[nlv*3 +2]
                    );

		        /*normalData[nlv*3 + 0] = vertexData[nlv*3 +0] * 0.015f;  //0.05f;
                // these other 2 are actually in the correct direction since its a cylinder
                normalData[nlv*3 + 1] = vertexData[nlv*3 + 1] * 0.015f;
                normalData[nlv*3 + 2] = vertexData[nlv*3 + 2] * 0.015f;
                continue;*/

		    }


		    Vec3 faceNormal1 = triUpEdge.cross(tri1Edge);
            Vec3 faceNormal2 = tri2Edge.cross(triUpEdge);
            Vec3 faceNormal3 = triDownEdge.cross(tri1Edge);
            Vec3 faceNormal4 = tri2Edge.cross(triDownEdge);


            Vec3 vNormal = faceNormal1 + faceNormal2 + faceNormal3 + faceNormal4;
            vNormal = vNormal.Normalize();
            vNormal = vNormal * vertexNormalLength;

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



	//////////////////////////////////////////////
	// VERTEXARRAY will copy this data
	tire.Add(
		&triData.front(), triVIndexCount,
		&vertexData.front(), vertexFloatCount,
		&texData.front(), texCoordFloats,
		&normalData.front(), vertexFloatCount);

	//printf("tire created: v=%u, tri=%u\n", vertexCount, (triVIndexCount/3));
}


















/////////////////////////////////////////////////////////////////////////////////////////////
// mg_wheelEdge
void mg_rim(VertexArray & rim, float sectionWidth_mm, float /*aspectRatio*/, float rimDiameter_in, float flangeDisplacement_mm)
{
    unsigned int segmentsAround = 32;

    float vertexNormalLength = 0.025f;


    // some defaults that all get reset in the next decision
	float innerRadius = 0.65f;
    float flangeOuterRadius = 0.22f;

    float flangeOutsideWidth = 0.17f;
    float innerWidth = 0.16f;        // the main part of the wheel



    if (true)
    {
        innerRadius = rimDiameter_in * 0.0254f * 0.5f;      // convert to a meter radius.


        // fudge number: based on 15 inch rims on my car and truck - must be synchronized with the one in mg_tire
        flangeOuterRadius = innerRadius * 1.08f;


        innerRadius -= 0.010f;      // knock off 10mm for the thickness of the sheet metal.


        // calculate some messumenets of the tire
        float sectionWidth_m = sectionWidth_mm / 1000.0f;
        flangeOutsideWidth = sectionWidth_m - (sectionWidth_m / 4.0f);     // subtract 1/8th from section width, to get the tread width (not really sure, looks ok though)


        //flangeDisplacement_mm = abs(flangeDisplacement_mm);         // make it so it can't spike outward
        innerWidth = flangeOutsideWidth - (flangeDisplacement_mm/1000.0f * 2.0f);


    }



	// non-configurable parameters
	unsigned int vertexRings = 6;
	float angleIncrement = 360.0f / (float) segmentsAround;

	/////////////////////////////////////
	//
	// vertices (temporary data)
	//
	unsigned int vertexesAround = segmentsAround + 1;
	unsigned int vertexCount = vertexesAround * vertexRings;
	unsigned int vertexFloatCount = vertexCount * 3;	// * 3 cause there are 3 floats in a vertex
	std::vector<float> vertexData;
	vertexData.resize(vertexFloatCount);

	// Right-side bevel, outer lip
	for (unsigned int lv=0 ; lv<vertexesAround ; lv++)
	{
		vertexData[(lv+vertexesAround * 0) * 3 + 0] = 1.0f * (flangeOutsideWidth / 2.0f);
		vertexData[(lv+vertexesAround * 0) * 3 + 1] = flangeOuterRadius * cosD(angleIncrement * lv);
		vertexData[(lv+vertexesAround * 0) * 3 + 2] = flangeOuterRadius * sinD(angleIncrement * lv);
	}
	// Right-side bevel, inner lip
	for (unsigned int lv=0 ; lv<vertexesAround ; lv++)
	{
		vertexData[(lv+vertexesAround * 1) * 3 + 0] = 1.0f * (innerWidth / 2.0f);
		vertexData[(lv+vertexesAround * 1) * 3 + 1] = (innerRadius) * cosD(angleIncrement * lv);
		vertexData[(lv+vertexesAround * 1) * 3 + 2] = (innerRadius) * sinD(angleIncrement * lv);
	}


	// Right-side of main cylinder
	for (unsigned int lv=0 ; lv<vertexesAround ; lv++)
	{
		vertexData[(lv+vertexesAround * 2) * 3 + 0] = 1.0f * (innerWidth / 2.0f);
		vertexData[(lv+vertexesAround * 2) * 3 + 1] = (innerRadius) * cosD(angleIncrement * lv);
		vertexData[(lv+vertexesAround * 2) * 3 + 2] = (innerRadius) * sinD(angleIncrement * lv);
	}
	// Left-side of main cylinder,
	for (unsigned int lv=0 ; lv<vertexesAround ; lv++)
	{
		vertexData[(lv+vertexesAround * 3) * 3 + 0] = -1.0f * (innerWidth / 2.0f);
		vertexData[(lv+vertexesAround * 3) * 3 + 1] = (innerRadius) * cosD(angleIncrement * lv);
		vertexData[(lv+vertexesAround * 3) * 3 + 2] = (innerRadius) * sinD(angleIncrement * lv);
	}

	// Left-side bevel, inner lip
	for (unsigned int lv=0 ; lv<vertexesAround ; lv++)
	{
		vertexData[(lv+vertexesAround * 4) * 3 + 0] = -1.0f * (innerWidth / 2.0f);
		vertexData[(lv+vertexesAround * 4) * 3 + 1] = innerRadius * cosD(angleIncrement * lv);
		vertexData[(lv+vertexesAround * 4) * 3 + 2] = innerRadius * sinD(angleIncrement * lv);
	}
	// Left-side bevel, outer lip
	for (unsigned int lv=0 ; lv<vertexesAround ; lv++)
	{
		vertexData[(lv+vertexesAround * 5) * 3 + 0] = -1.0f * (flangeOutsideWidth / 2.0f);
		vertexData[(lv+vertexesAround * 5) * 3 + 1] = (flangeOuterRadius) * cosD(angleIncrement * lv);
		vertexData[(lv+vertexesAround * 5) * 3 + 2] = (flangeOuterRadius) * sinD(angleIncrement * lv);
	}






	/////////////////////////////////////
	//
	//  build triangles
	//  different from the last one
    //                            2 triangles * segmentsAround * 3 VertexIndexes * 3 completely separate tubes
	unsigned int triVIndexCount = 2 * segmentsAround * 3 * 3;
	std::vector<unsigned int> triData(triVIndexCount, 0);
	//triData.resize(triVIndexCount);

	unsigned int triIndex = 0;
	for (unsigned int circleSegment=0 ; circleSegment<segmentsAround; circleSegment++)
	{
		// 1st triangle (Right-side - Inner to Sidewall)
		triData[ (triIndex+0)*3 + 0 ] = circleSegment;
		triData[ (triIndex+0)*3 + 1 ] = circleSegment+1;
		triData[ (triIndex+0)*3 + 2 ] = circleSegment+vertexesAround;

		// 2nd triangle
		triData[ (triIndex+1)*3 + 0 ] = circleSegment+vertexesAround;
		triData[ (triIndex+1)*3 + 1 ] = circleSegment+1;
		triData[ (triIndex+1)*3 + 2 ] = circleSegment+vertexesAround+1;

		triIndex +=2;
	}

	for (unsigned int circleSegment=0 ; circleSegment<segmentsAround; circleSegment++)
	{
		// 1st triangle (Right-side - Inner to Sidewall)
		triData[ (triIndex+0)*3 + 0 ] = (2*vertexesAround)+circleSegment;
		triData[ (triIndex+0)*3 + 1 ] = (2*vertexesAround)+circleSegment+1;
		triData[ (triIndex+0)*3 + 2 ] = (2*vertexesAround)+circleSegment+vertexesAround;

		// 2nd triangle
		triData[ (triIndex+1)*3 + 0 ] = (2*vertexesAround)+circleSegment+vertexesAround;
		triData[ (triIndex+1)*3 + 1 ] = (2*vertexesAround)+circleSegment+1;
		triData[ (triIndex+1)*3 + 2 ] = (2*vertexesAround)+circleSegment+vertexesAround+1;

		triIndex +=2;
	}

	for (unsigned int circleSegment=0 ; circleSegment<segmentsAround; circleSegment++)
	{
		// 1st triangle (Right-side - Inner to Sidewall)
		triData[ (triIndex+0)*3 + 0 ] = (4*vertexesAround)+circleSegment+0;
		triData[ (triIndex+0)*3 + 1 ] = (4*vertexesAround)+circleSegment+1;
		triData[ (triIndex+0)*3 + 2 ] = (4*vertexesAround)+circleSegment+vertexesAround;

		// 2nd triangle
		triData[ (triIndex+1)*3 + 0 ] = (4*vertexesAround)+circleSegment+vertexesAround;
		triData[ (triIndex+1)*3 + 1 ] = (4*vertexesAround)+circleSegment+1;
		triData[ (triIndex+1)*3 + 2 ] = (4*vertexesAround)+circleSegment+vertexesAround+1;

		triIndex +=2;
	}








    //////////////////////////////////////////////
    // Texture Coordinates
	unsigned int texCoordFloats = vertexCount * 2;
	std::vector<float> texData;
	texData.resize(texCoordFloats);

    // set them all to zero for the time being
    for ( unsigned int tlv=0 ; tlv< vertexCount ; tlv++ )
    {
        float u = tlv % vertexesAround;
		u = u / segmentsAround;
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

    Vec3 tri1Edge;        // one of the edges of a triangle that goes around the tire's circle
    Vec3 tri2Edge;        // one of the edges of a triangle that goes around the tire's circle
    Vec3 triUpEdge;

    for (unsigned int nlv=0 ; nlv<vertexCount ; nlv++)
    {
        normalData[nlv*3+0] = 0.1;
        normalData[nlv*3+1] = 0;
        normalData[nlv*3+2] = 0;
    }


    for (unsigned int nlv=0 ; nlv<vertexCount ; nlv++)
    {
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

            Vec3 faceNormal1 = tri1Edge.cross(triUpEdge);
            Vec3 faceNormal2 = triUpEdge.cross(tri2Edge);

            Vec3 vNormal = faceNormal1 + faceNormal2;
            vNormal = vNormal.Normalize();
            vNormal = vNormal * vertexNormalLength;

            normalData[nlv*3 + 0] = vNormal[0];
            normalData[nlv*3 + 1] = vNormal[1];
            normalData[nlv*3 + 2] = vNormal[2];
		}
        else if ( nlv < vertexesAround *2 )
        {
            // since we want a hard edge here we can just copy the normals from the previous decision tree
            normalData[nlv*3 + 0] = normalData[ (nlv-vertexesAround) *3 +0];
            normalData[nlv*3 + 1] = normalData[ (nlv-vertexesAround) *3 +1];
            normalData[nlv*3 + 2] = normalData[ (nlv-vertexesAround) *3 +2];
        }


        // now all the vertexes on the inside (both rings of them)
        else if ( nlv < vertexesAround *4 )
        {

            Vec3 iNormal;
            iNormal.Set( 0, vertexData[nlv*3 + 1] , vertexData[nlv*3 + 2]  );
            iNormal = iNormal * -1.0f;
            iNormal = iNormal.Normalize();
            iNormal = iNormal * vertexNormalLength;

            normalData[nlv*3 + 0 ] = iNormal[0];
            normalData[nlv*3 + 1 ] = iNormal[1];
            normalData[nlv*3 + 2 ] = iNormal[2];
        }



        else if ( nlv < vertexesAround*5 )       // first ring of vertexes
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

            Vec3 faceNormal1 = tri1Edge.cross(triUpEdge);
            Vec3 faceNormal2 = triUpEdge.cross(tri2Edge);

            Vec3 vNormal = faceNormal1 + faceNormal2;
            vNormal = vNormal.Normalize();
            vNormal = vNormal * vertexNormalLength;

            normalData[nlv*3 + 0] = vNormal[0];
            normalData[nlv*3 + 1] = vNormal[1];
            normalData[nlv*3 + 2] = vNormal[2];
		}
        else if ( nlv < vertexesAround *6 )
        {
            // since we want a hard edge here we can just copy the normals from the previous decision tree
            normalData[nlv*3 + 0] = normalData[ (nlv-vertexesAround) *3 +0];
            normalData[nlv*3 + 1] = normalData[ (nlv-vertexesAround) *3 +1];
            normalData[nlv*3 + 2] = normalData[ (nlv-vertexesAround) *3 +2];
        }






		else
		{
		    normalData[nlv*3+0] = vertexData[nlv*3+0] * 0.05f;
		    normalData[nlv*3+1] = vertexData[nlv*3+1] * 0.05f;
		    normalData[nlv*3+2] = vertexData[nlv*3+2] * 0.05f;
		}
    }




	//////////////////////////////////////////////
	// VERTEXARRAY will copy this data
	rim.Add(
		&triData.front(), triVIndexCount,
		&vertexData.front(), vertexFloatCount,
		&texData.front(), texCoordFloats,
		&normalData.front(), vertexFloatCount);

	//	printf("wheel_edge created: v=%u, tri=%u\n", vertexCount, (triVIndexCount/3) );

}







//////////////////////////////////////////////////////////////////////
// Brake Rotor
void mg_brake_rotor(VertexArray & rotor, float diameter_mm, float thickness_mm)
{
    // tweak-able
    unsigned int segmentsAround = 32;
    float normalLength = 1.00f;



    // non-tweakable
    float radius_m = diameter_mm;
    radius_m /= 2;      // now a radius
    radius_m /= 1000;   // now in meters

    float thickness_m = thickness_mm / 1000.0f;

    unsigned int vertexesAround = segmentsAround + 1;
    float angleIncrement = 360.0f / segmentsAround ;



    unsigned int vertexCount = vertexesAround*4;            // the two caps have 1 extra vertex in the center.  each ring of the sides has one extra dupe vertex for the texture map to wrap correctly
    unsigned int vertexFloatCount = vertexCount *3;

    std::vector<float> vertexData;
    vertexData.resize(vertexFloatCount);

    // first cap, first vertex
    vertexData[0+0] = -1.0f * thickness_m / 2.0f;
    vertexData[0+1] = 0.0f;
    vertexData[0+2] = 0.0f;

    // first cap, most vertexes
	for (unsigned int vlv=0 ; vlv<segmentsAround ; vlv++)
	{
		float *x = &vertexData[3 + (vlv + vertexesAround*0) * 3 + 0];
		float *y = &vertexData[3 + (vlv + vertexesAround*0) * 3 + 1];
		float *z = &vertexData[3 + (vlv + vertexesAround*0) * 3 + 2];

		*x = -1.0f * (thickness_m / 2.0f);
		*y = radius_m * cosD( angleIncrement * vlv);
		*z = radius_m * sinD( angleIncrement * vlv);
	}

    // strip in the center, first ring
    for (unsigned int vlv=0 ; vlv<vertexesAround ; vlv++)
    {
		float *x = &vertexData[(vlv + vertexesAround*1) * 3 + 0];
		float *y = &vertexData[(vlv + vertexesAround*1) * 3 + 1];
		float *z = &vertexData[(vlv + vertexesAround*1) * 3 + 2];

		*x = -1.0f * (thickness_m / 2.0f) + 0.00f;
		*y = radius_m * cosD( angleIncrement * vlv);
		*z = radius_m * sinD( angleIncrement * vlv);

    }
    // strip in the center, second ring
    for (unsigned int vlv=0 ; vlv<vertexesAround ; vlv++)
    {
		float *x = &vertexData[(vlv + vertexesAround*2) * 3 + 0];
		float *y = &vertexData[(vlv + vertexesAround*2) * 3 + 1];
		float *z = &vertexData[(vlv + vertexesAround*2) * 3 + 2];

		*x = 1.0f * (thickness_m / 2.0f) + 0.00f;
		*y = radius_m * cosD( angleIncrement * vlv);
		*z = radius_m * sinD( angleIncrement * vlv);

    }

    // last cap, most vertexes
	for (unsigned int vlv=0 ; vlv<segmentsAround ; vlv++)
	{
		float *x = &vertexData[(vlv + vertexesAround*3) * 3 + 0];
		float *y = &vertexData[(vlv + vertexesAround*3) * 3 + 1];
		float *z = &vertexData[(vlv + vertexesAround*3) * 3 + 2];

		*x = 1.0f * (thickness_m / 2.0f) + 0.00f;
		*y = radius_m * cosD( angleIncrement * vlv);
		*z = radius_m * sinD( angleIncrement * vlv);
	}

    // last cap, last vertex
    vertexData[ (vertexCount-1)*3 +0] = thickness_m / 2.0f + 0.0f;
    vertexData[ (vertexCount-1)*3 +1] = 0.0f;
    vertexData[ (vertexCount-1)*3 +2] = 0.0f;



    //////////////////////////////////////////////////
    // triangles
    unsigned int trianglesPerCap = segmentsAround;
    unsigned int trianglesPerStrip = segmentsAround * 2;

	unsigned int triVIndexCount = (2*trianglesPerCap + 2*trianglesPerStrip) * 3;

	std::vector<unsigned int> triData;
	triData.resize(triVIndexCount);

	unsigned int triIndex = 0;

	unsigned int *triVIndex0;
	unsigned int *triVIndex1;
	unsigned int *triVIndex2;



    // clear all data - good for when building the following loops
    for (unsigned int tlv=0 ; tlv<triVIndexCount ; tlv++)
    {
        triData[tlv] = 0;
    }

    // first cap
    for ( unsigned int tlv=0 ; tlv<segmentsAround ; tlv++)
    {
        triVIndex0 = &triData[ tlv * 3 + 0];
        triVIndex1 = &triData[ tlv * 3 + 1];
        triVIndex2 = &triData[ tlv * 3 + 2];

        if (tlv == segmentsAround-1)      // last triangle
        {
            *triVIndex0 = 0;
            *triVIndex1 = 1;
            *triVIndex2 = segmentsAround;
        }
        else
        {
            *triVIndex0 = 0;
            *triVIndex1 = tlv+2;
            *triVIndex2 = tlv+1;
        }
    }

    // strip in the center
    triIndex = segmentsAround;
    for ( unsigned int tlv=0 ; tlv<segmentsAround ; tlv++ )
    {
            // 1st tri
            triVIndex0 = &triData[ triIndex * 3 + 0];
            triVIndex1 = &triData[ triIndex * 3 + 1];
            triVIndex2 = &triData[ triIndex * 3 + 2];

            *triVIndex0 = vertexesAround+tlv+0;
            *triVIndex1 = vertexesAround+tlv+1;
            *triVIndex2 = vertexesAround+tlv+segmentsAround+1;

            triIndex++;

            // 2nd tri
            triVIndex0 = &triData[ triIndex * 3 + 0];
            triVIndex1 = &triData[ triIndex * 3 + 1];
            triVIndex2 = &triData[ triIndex * 3 + 2];

            *triVIndex0 = vertexesAround+tlv+0+1;
            *triVIndex2 = vertexesAround+tlv+segmentsAround+1;
            *triVIndex1 = vertexesAround+tlv+segmentsAround+1+1;

            triIndex++;
    }


    triIndex+=2;            // skip the last two vertexes in the strip, cause we had to make them duplicates to get the texcoords in different spots

    // 2nd cap
    for ( unsigned int tlv=0 ; tlv<segmentsAround ; tlv++)
    {
        triVIndex0 = &triData[ (triIndex + tlv) * 3 + 0];
        triVIndex1 = &triData[ (triIndex + tlv) * 3 + 1];
        triVIndex2 = &triData[ (triIndex + tlv) * 3 + 2];

        if (tlv == segmentsAround-1)      // last triangle
        {
            *triVIndex1 = triIndex*0 + vertexCount-1;
            *triVIndex0 = triIndex + 1;
            *triVIndex2 = triIndex + segmentsAround;
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
    std::vector<float> texData;
	texData.resize(texCoordFloats);


    // first cap, first texcoord
    texData[0+0] = 7.0f / 16.0f;
    texData[0+1] = 7.0f / 16.0f;

    // first cap, most texcoords
	for (unsigned int vlv=0 ; vlv<segmentsAround ; vlv++)
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

		*u = (float)vlv / (float)segmentsAround;
		*v = 1.0f - 1.0f/8.0f;

    }
    // strip in the center, second ring
    for (unsigned int vlv=0 ; vlv<vertexesAround ; vlv++)
    {
		float *u = &texData[(vlv + vertexesAround*2) * 2 + 0];
		float *v = &texData[(vlv + vertexesAround*2) * 2 + 1];

		*u = (float)vlv / (float)segmentsAround;
		*v = 1.0f;

    }

    // last cap, most texcoords
	for (unsigned int vlv=0 ; vlv<segmentsAround ; vlv++)
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
    texData[ (vertexCount-1)*2 +0] = 7.0f / 16.0f;
    texData[ (vertexCount-1)*2 +1] = 7.0f / 16.0f;





    /////////////////////////////////////////
    // finally vertex normals
    std::vector<float> normalData;
    normalData.resize(vertexFloatCount);


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
            Vec3 normal;

            normal.Set(   0.0f,
                vertexData[nlv*3+1],
                vertexData[nlv*3+2]     );

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


	rotor.Add(
		&triData.front(), triVIndexCount,
		&vertexData.front(), vertexFloatCount,
		&texData.front(), texCoordFloats,
		&normalData.front(), vertexFloatCount);
}











} //namespace
