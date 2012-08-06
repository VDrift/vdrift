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

#ifndef _MESHGEN_H
#define _MESHGEN_H

class VERTEXARRAY;

// all units are metric
namespace MESHGEN
{

struct TireSpec
{
	TireSpec();

	// will recalculate optional parameters
	void set(float sectionwidth, float aspectratio, float rimdiameter);

	float sectionWidth;			// 0.185
	float aspectRatio;			// 0.45
	float rimDiameter;			// 17 * 0.0254 = 0.43
	float innerRadius;			// rimDiameter * 0.5
	float innerWidth;			// sectionWidth * 0.75
	float sidewallRadius;		// innerRadius + sectionWidth * aspectRatio * 0.5
	float shoulderRadius;		// innerRadius + sectionWidth * aspectRatio * 0.9
	float treadRadius;			// innerRadius + sectionWidth * aspectRatio * 1.0
	float sidewallBulge;		// innerWidth * 0.05
	float shoulderBulge;		// treadWidth * 0.05
	float treadWidth;			// sectionWidth * 0.75
	int segments;				// 32
};

struct RimSpec
{
	RimSpec();

	// will recalculate optional parameters
	void set(float sectionwidth, float aspectratio, float rimdiameter);
	
	const VERTEXARRAY * hub;	// mesh containing hub section of the rim
	float sectionWidth;			// 0.185
	float aspectRatio;			// 0.45
	float rimDiameter;			// 17 * 0.0254 = 0.43
	float innerRadius;			// rimDiameter * 0.5
	float innerWidth;			// sectionWidth * 0.75
	float flangeOuterRadius;	// innerRadius * 1.08
	float flangeInnerRadius;	// innerRadius - 0.01
	float flangeOutsideWidth;	// sectionWidth * 0.75
	float flangeInnerWidth;		// flangeOutsideWidth - 0.02
	int segments;				// 32
};

void CreateTire(VERTEXARRAY & tire, const TireSpec & spec);

void CreateRim(VERTEXARRAY & rim, const RimSpec & spec);

void CreateRotor(VERTEXARRAY & rotor, float radius, float thickness);

}

#endif
