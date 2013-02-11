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

#include "joeserialize.h"
#include "macros.h"

#include <vector>
#include <cassert>

class MODEL_OBJ;

class VERTEXARRAY
{
public:
	VERTEXARRAY();

	~VERTEXARRAY();

	VERTEXARRAY operator+ (const VERTEXARRAY & v) const;

	void Clear();

	void SetColors(const unsigned char array[], size_t count, size_t offset = 0);

	void SetNormals(const float array[], size_t count, size_t offset = 0);

	void SetVertices(const float array[], size_t count, size_t offset = 0);

	void SetFaces(const int newarray[], size_t newarraycount);

	void SetTexCoordSets(int newtcsets);

	/// set is zero indexed
	void SetTexCoords(size_t set, const float newarray[], size_t newarraycount);

	/// assumes there is 1 tex coord set
	void Add(
		const unsigned char newcol[], int newcolcount,
		const float newnorm[], int newnormcount,
		const float newvert[], int newvertcount,
		const int newfaces[], int newfacecount,
		const float newtc[], int newtccount);

	/// C style interface functions

	void GetColors(const unsigned char * & output_array_pointer, int & output_array_num) const;

	void GetNormals(const float * & output_array_pointer, int & output_array_num) const;

	void GetVertices(const float * & output_array_pointer, int & output_array_num) const;

	void GetFaces(const int * & output_array_pointer, int & output_array_num) const;

	inline int GetTexCoordSets() const { return texcoords.size(); }

	void GetTexCoords(size_t set, const float * & output_array_pointer, int & output_array_num) const;

	int GetNumFaces() const { return faces.size(); }

	/// helper functions

	void SetToBillboard(float x1, float y1, float x2, float y2);

	void SetTo2DButton(float x, float y, float w, float h, float sidewidth, bool flip=false);

	void SetTo2DBox(float x, float y, float w, float h, float marginwidth, float marginheight, float clipx=1.f);

	void SetTo2DQuad(float x1, float y1, float x2, float y2, float u1, float v1, float u2, float v2, float z=0.f);

	void SetVertexData2DQuad(float x1, float y1, float x2, float y2, float u1, float v1, float u2, float v2, float * vcorners, float * uvs, int * bfaces, int faceoffset=0) const;

	void SetToUnitCube();

	/// build the vertex array given the faces defined by the verts, normals, and texcoords passed in

	struct TRIFLOAT
	{
		float x, y, z;

		TRIFLOAT(float nx, float ny, float nz) : x(nx), y(ny), z(nz) {}

		TRIFLOAT() : x(0), y(0), z(0) {}
	};

	struct TWOFLOAT
	{
		float u,v;

		TWOFLOAT(float nu, float nv) : u(nu), v(nv) {}

		TWOFLOAT() : u(0), v(0) {}
	};

	struct VERTEXDATA
	{
		TRIFLOAT vertex;
		TRIFLOAT normal;
		TWOFLOAT texcoord;

		VERTEXDATA(TRIFLOAT nv, TRIFLOAT nn, TWOFLOAT nt) : vertex(nv), normal(nn), texcoord(nt) {}

		VERTEXDATA() {}

		bool operator<(const VERTEXDATA & other) const
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

	struct FACE
	{
		VERTEXDATA v[3];

		FACE(VERTEXDATA v1, VERTEXDATA v2, VERTEXDATA v3) {
			v[0] = v1; v[1] = v2; v[3] = v3;
		}

		FACE() {}
	};

	void BuildFromFaces(const std::vector <FACE> & faces);

	/// set color for all vertices
	void SetColor(float r, float g, float b, float a);

	void Translate(float x, float y, float z);

	void Rotate(float a, float x, float y, float z);

	void Scale(float x, float y, float z);

	void FlipNormals();

	bool Serialize(joeserialize::Serializer & s);

private:
	friend class joeserialize::Serializer;
	friend class MODEL_OBJ;
	std::vector < std::vector <float> > texcoords;
	std::vector <unsigned char> colors;
	std::vector <float> normals;
	std::vector <float> vertices;
	std::vector <int> faces;
};

#endif
