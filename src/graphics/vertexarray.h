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

#ifndef _VERTEXARRAY_H
#define _VERTEXARRAY_H

#include "vertexformat.h"
#include "joeserialize.h"
#include "macros.h"

#include <vector>

class ModelObj;

class VertexArray
{
public:
	VertexArray();

	~VertexArray();

	void Clear();

	VertexArray operator+ (const VertexArray & v) const;

	void GetColors(const unsigned char * & output_array_pointer, int & output_array_num) const;

	void GetTexCoords(const float * & output_array_pointer, int & output_array_num) const;

	void GetNormals(const float * & output_array_pointer, int & output_array_num) const;

	void GetVertices(const float * & output_array_pointer, int & output_array_num) const;

	void GetFaces(const unsigned int * & output_array_pointer, int & output_array_num) const;

	unsigned int GetNumVertices() const { return vertices.size() / 3; }

	unsigned int GetNumIndices() const { return faces.size(); }

	VertexFormat::Enum GetVertexFormat() const { return format; }

	void Add(
		const unsigned int newfaces[], int newfacecount,
		const float newvert[], int newvertcount,
		const float newtco[] = 0, int newtcocount = 0,
		const float newnorm[] = 0, int newnormcount = 0,
		const unsigned char newcol[] = 0, int newcolcount = 0);

	/// helper functions

	void SetToBillboard(float x1, float y1, float x2, float y2);

	void SetTo2DButton(float x, float y, float w, float h, float sidewidth, bool flip=false);

	void SetTo2DBox(float x, float y, float w, float h, float marginwidth, float marginheight, float clipx=1.f);

	void SetTo2DQuad(float x1, float y1, float x2, float y2, float u1, float v1, float u2, float v2, float z=0.f);

	void SetVertexData2DQuad(float x1, float y1, float x2, float y2, float u1, float v1, float u2, float v2, float * vcorners, float * uvs, unsigned int * bfaces, unsigned int faceoffset=0) const;

	void SetToUnitCube();

	void SetTo2DRing(float r0, float r1, float a0, float a1, unsigned n);

	/// build the vertex array given the faces defined by the verts, normals, and texcoords passed in

	struct Float3
	{
		float x, y, z;

		Float3(float nx, float ny, float nz) : x(nx), y(ny), z(nz) {}

		Float3() : x(0), y(0), z(0) {}
	};

	struct Float2
	{
		float u,v;

		Float2(float nu, float nv) : u(nu), v(nv) {}

		Float2() : u(0), v(0) {}
	};

	struct VertexData
	{
		Float3 vertex;
		Float3 normal;
		Float2 texcoord;

		VertexData(Float3 nv, Float3 nn, Float2 nt) : vertex(nv), normal(nn), texcoord(nt) {}

		VertexData() {}

		bool operator<(const VertexData & other) const
		{
			if (vertex.x != other.vertex.x)
				return vertex.x < other.vertex.x;
			else if (vertex.y != other.vertex.y)
				return vertex.y < other.vertex.y;
			else if (vertex.z != other.vertex.z)
				return vertex.z < other.vertex.z;
			else if (normal.x != other.normal.x)
				return normal.x < other.normal.x;
			else if (normal.y != other.normal.y)
				return normal.y < other.normal.y;
			else if (normal.z != other.normal.z)
				return normal.z < other.normal.z;
			else if (texcoord.u != other.texcoord.u)
				return texcoord.u < other.texcoord.u;
			else if (texcoord.v != other.texcoord.v)
				return texcoord.v < other.texcoord.v;
			else
				return false; //they are equal
		}
	};

	struct Face
	{
		VertexData v[3];

		Face(VertexData v1, VertexData v2, VertexData v3) {
			v[0] = v1; v[1] = v2; v[2] = v3;
		}

		Face() {}
	};

	void BuildFromFaces(const std::vector <Face> & faces);

	void Translate(float x, float y, float z);

	void Rotate(float a, float x, float y, float z);

	void Scale(float x, float y, float z);

	// scale normals by -1
	void FlipNormals();

	// reverse face vertices winding order
	void FlipWindingOrder();

	// set winding order to match normal direction, used by scale
	void FixWindingOrder();

	bool Serialize(joeserialize::Serializer & s);

private:
	friend class joeserialize::Serializer;
	friend class ModelObj;
	std::vector <unsigned char> colors;
	std::vector <float> texcoords;
	std::vector <float> normals;
	std::vector <float> vertices;
	std::vector <unsigned int> faces;
	VertexFormat::Enum format;

	void SetColors(const unsigned char array[], size_t count, size_t offset = 0);

	void SetTexCoords(const float array[], size_t count, size_t offset = 0);

	void SetNormals(const float array[], size_t count, size_t offset = 0);

	void SetVertices(const float array[], size_t count, size_t offset = 0);

	void SetFaces(const unsigned int array[], size_t count, size_t offset = 0, size_t idoffset = 0);
};

#endif
