#include "rendermodelext_drawable.h"

#include "vertexarray.h"

enum VERTEX_ATTRIBS
{
	VERTEX_POSITION,
	VERTEX_NORMAL,
	VERTEX_TANGENT,
	VERTEX_BITANGENT,
	VERTEX_COLOR,
	VERTEX_UV0,
	VERTEX_UV1,
	VERTEX_UV2
};

void RenderModelExternalDrawable::draw(GLWrapper & gl) const
{
	if (vert_array)
	{
		const float * verts;
		int vertcount;
		vert_array->GetVertices(verts, vertcount);
		gl.VertexAttribPointer(VERTEX_POSITION, 3, GL_FLOAT, GL_FALSE, 0, verts);
		gl.EnableVertexAttribArray(VERTEX_POSITION);

		const float * norms;
		int normcount;
		vert_array->GetNormals(norms, normcount);
		gl.VertexAttribPointer(VERTEX_NORMAL, 3, GL_FLOAT, GL_FALSE, 0, norms);
		gl.EnableVertexAttribArray(VERTEX_NORMAL);
		
		gl.DisableVertexAttribArray(VERTEX_TANGENT);
		gl.DisableVertexAttribArray(VERTEX_BITANGENT);
		gl.DisableVertexAttribArray(VERTEX_COLOR);

		const float * tc[1];
		int tccount[1];
		if (vert_array->GetTexCoordSets() > 0)
		{
			vert_array->GetTexCoords(0, tc[0], tccount[0]);
			gl.VertexAttribPointer(VERTEX_UV0, 2, GL_FLOAT, GL_FALSE, 0, tc[0]);
			gl.EnableVertexAttribArray(VERTEX_UV0);
		}
		else
			gl.DisableVertexAttribArray(VERTEX_UV0);
		
		gl.DisableVertexAttribArray(VERTEX_UV1);
		gl.DisableVertexAttribArray(VERTEX_UV2);

		const int * faces;
		int facecount;
		vert_array->GetFaces(faces, facecount);
		gl.DrawElements(GL_TRIANGLES, facecount, GL_UNSIGNED_INT, faces);
	}
}
