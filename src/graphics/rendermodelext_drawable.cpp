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
#include "vertexattribs.h"
using namespace VERTEX_ATTRIBS;

void RenderModelExternalDrawable::draw(GLWrapper & gl) const
{
	if (vao != 0)
	{
		RenderModelExternal::draw(gl);
	}
	else if (vert_array)
	{
		gl.unbindVertexArray();

		const float * verts = 0;
		int vertcount = 0;
		vert_array->GetVertices(verts, vertcount);
		if (verts)
		{
			gl.VertexAttribPointer(VERTEX_POSITION, 3, GL_FLOAT, GL_FALSE, 0, verts);
			gl.EnableVertexAttribArray(VERTEX_POSITION);

			const float * norms = 0;
			int normcount = 0;
			vert_array->GetNormals(norms, normcount);
			if (norms)
			{
				gl.VertexAttribPointer(VERTEX_NORMAL, 3, GL_FLOAT, GL_FALSE, 0, norms);
				gl.EnableVertexAttribArray(VERTEX_NORMAL);
			}
			else
				gl.DisableVertexAttribArray(VERTEX_NORMAL);

			gl.DisableVertexAttribArray(VERTEX_TANGENT);
			gl.DisableVertexAttribArray(VERTEX_BITANGENT);

			const unsigned char * cols = 0;
			int colcount = 0;
			vert_array->GetColors(cols, colcount);
			if (cols)
			{
				gl.VertexAttribPointer(VERTEX_COLOR, 4, GL_UNSIGNED_BYTE, GL_FALSE, 0, cols);
				gl.EnableVertexAttribArray(VERTEX_COLOR);
			}
			else
				gl.DisableVertexAttribArray(VERTEX_COLOR);

			const float * tc[1] = {0};
			int tccount[1] = {0};
			if (vert_array->GetTexCoordSets() > 0)
			{
				// TODO: make this work for UV1 and UV2
				vert_array->GetTexCoords(0, tc[0], tccount[0]);
				assert(tc[0]);
				gl.VertexAttribPointer(VERTEX_UV0, 2, GL_FLOAT, GL_FALSE, 0, tc[0]);
				gl.EnableVertexAttribArray(VERTEX_UV0);
			}
			else
				gl.DisableVertexAttribArray(VERTEX_UV0);

			gl.DisableVertexAttribArray(VERTEX_UV1);
			gl.DisableVertexAttribArray(VERTEX_UV2);

			const int * faces = 0;
			int facecount = 0;
			vert_array->GetFaces(faces, facecount);
            if (faces)
            {
                gl.DrawElements(GL_TRIANGLES, facecount, GL_UNSIGNED_INT, faces);
            }
            else
            {
                gl.LineWidth(linesize);
                gl.DrawArrays(GL_LINES, 0, vertcount/3);
            }
		}
	}
}
