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

#include "vertexformat.h"
#include "glcore.h"

const VertexFormat & VertexFormat::Get(Enum e)
{
	using namespace VertexAttrib;
	static const VertexFormat fmts[LastFormat + 1] =
	{
		{
			// PNT332
			{
				{VertexPosition,     3, GL_FLOAT, 0, false},
				{VertexNormal,       3, GL_FLOAT, 3 * sizeof(float), false},
				{VertexTexCoord,     2, GL_FLOAT, 6 * sizeof(float), false},
				{VertexTangent,      0, GL_FLOAT, 0, false},
				{VertexBlendIndices, 0, GL_UNSIGNED_BYTE, 0, false},
				{VertexBlendWeights, 0, GL_UNSIGNED_BYTE, 0, true},
				{VertexColor,        0, GL_UNSIGNED_BYTE, 0, true},
			},
			3,
			8 * sizeof(float)
		},

		{
			// PTC324
			{
				{VertexPosition,     3, GL_FLOAT, 0, false},
				{VertexTexCoord,     2, GL_FLOAT, 3 * sizeof(float), false},
				{VertexColor,        4, GL_UNSIGNED_BYTE, 5 * sizeof(float), true},
				{VertexNormal,       0, GL_FLOAT, 0, false},
				{VertexTangent,      0, GL_FLOAT, 0, false},
				{VertexBlendIndices, 0, GL_UNSIGNED_BYTE, 0, false},
				{VertexBlendWeights, 0, GL_UNSIGNED_BYTE, 0, true},
			},
			3,
			5 * sizeof(float) + 4
		},

		{
			// PT32
			{
				{VertexPosition,     3, GL_FLOAT, 0, false},
				{VertexTexCoord,     2, GL_FLOAT, 3 * sizeof(float), false},
				{VertexNormal,       0, GL_FLOAT, 0, false},
				{VertexTangent,      0, GL_FLOAT, 0, false},
				{VertexBlendIndices, 0, GL_UNSIGNED_BYTE, 0, false},
				{VertexBlendWeights, 0, GL_UNSIGNED_BYTE, 0, true},
				{VertexColor,        0, GL_UNSIGNED_BYTE, 0, true},
			},
			2,
			5 * sizeof(float)
		},

		{
			// P3
			{
				{VertexPosition,     3, GL_FLOAT, 0, false},
				{VertexNormal,       0, GL_FLOAT, 0, false},
				{VertexTangent,      0, GL_FLOAT, 0, false},
				{VertexTexCoord,     0, GL_FLOAT, 0, false},
				{VertexBlendIndices, 0, GL_UNSIGNED_BYTE, 0, false},
				{VertexBlendWeights, 0, GL_UNSIGNED_BYTE, 0, true},
				{VertexColor,        0, GL_UNSIGNED_BYTE, 0, true},
			},
			1,
			3 * sizeof(float)
		}
	};
	return fmts[e];
}
