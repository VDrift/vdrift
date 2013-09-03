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

class VertexArray;

namespace MeshGen
{

void mg_tire(VertexArray & tire, float sectionWidth_mm, float aspectRatio, float rimDiameter_in);

void mg_rim(VertexArray & rim, float sectionWidth_mm, float aspectRatio, float rimDiameter_in, float flangeDisplacement_mm);

void mg_brake_rotor(VertexArray & rotor, float diameter_mm, float thickness_mm);

}

#endif
