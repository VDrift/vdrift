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

#include "rendermodelext_drawable.h"

#include "vertexarray.h"
#include "vertexattrib.h"

using namespace VertexAttrib;

void RenderModelExtDrawable::draw(GLWrapper & gl) const
{
	if (vao != 0)
	{
		RenderModelExt::draw(gl);
	}
	else if (vert_array)
	{
		gl.unbindVertexArray();

		const float * verts = 0;
		int vertcount = 0;
		vert_array->GetVertices(verts, vertcount);
		if (verts)
		{
			gl.VertexAttribPointer(VertexPosition, 3, GL_FLOAT, GL_FALSE, 0, verts);
			gl.EnableVertexAttribArray(VertexPosition);

			const float * norms = 0;
			int normcount = 0;
			vert_array->GetNormals(norms, normcount);
			if (norms)
			{
				gl.VertexAttribPointer(VertexNormal, 3, GL_FLOAT, GL_FALSE, 0, norms);
				gl.EnableVertexAttribArray(VertexNormal);
			}
			else
				gl.DisableVertexAttribArray(VertexNormal);

			gl.DisableVertexAttribArray(VertexTangent);
			gl.DisableVertexAttribArray(VertexBitangent);

			const unsigned char * cols = 0;
			int colcount = 0;
			vert_array->GetColors(cols, colcount);
			if (cols)
			{
				gl.VertexAttribPointer(VertexColor, 4, GL_UNSIGNED_BYTE, GL_TRUE, 0, cols);
				gl.EnableVertexAttribArray(VertexColor);
			}
			else
				gl.DisableVertexAttribArray(VertexColor);

			const float * tcos = 0;
			int tccount = 0;
			vert_array->GetTexCoords(tcos, tccount);
			if (tcos)
			{

				gl.VertexAttribPointer(VertexTexCoord, 2, GL_FLOAT, GL_FALSE, 0, tcos);
				gl.EnableVertexAttribArray(VertexTexCoord);
			}
			else
				gl.DisableVertexAttribArray(VertexTexCoord);

			gl.DisableVertexAttribArray(VertexBlendIndices);
			gl.DisableVertexAttribArray(VertexBlendWeights);
			gl.DisableVertexAttribArray(VertexColor);

			const unsigned int * faces = 0;
			int facecount = 0;
			vert_array->GetFaces(faces, facecount);
			if (faces)
			{
				gl.DrawElements(GL_TRIANGLES, facecount, GL_UNSIGNED_INT, faces);
			}
			else
			{
				gl.DrawArrays(GL_LINES, 0, vertcount/3);
			}
		}
	}
}
