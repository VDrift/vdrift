#ifndef _VERTEXARRAY_H
#define _VERTEXARRAY_H

#include "joeserialize.h"
#include "macros.h"

#include <vector>
#include <cassert>
#include <cstring>

class MODEL_OBJ;

class VERTEXARRAY
{
friend class joeserialize::Serializer;
friend class MODEL_OBJ;
private:
	std::vector < std::vector <float> > texcoords;
	std::vector <float> normals;
	std::vector <float> vertices;
	std::vector <int> faces;

public:
	VERTEXARRAY() {}
	~VERTEXARRAY() {Clear();}

	VERTEXARRAY operator+ (const VERTEXARRAY & v) const;

	void Clear() {texcoords.clear();normals.clear();vertices.clear();faces.clear();}

	void SetNormals(float * array, size_t count, size_t offset = 0);
	void SetVertices(float * array, size_t count, size_t offset = 0);
	void SetFaces(int * newarray, size_t newarraycount);
	void SetTexCoordSets(int newtcsets);
	void SetTexCoords(size_t set, float * newarray, size_t newarraycount); ///<set is zero indexed

	void Add(float * newnorm, int newnormcount, float * newvert, int newvertcount, int * newfaces, int newfacecount,
		float * newtc, int newtccount); ///< assumes there is 1 tex coord set

	//C style interface functions
	void GetNormals(const float * & output_array_pointer, int & output_array_num) const;
	void GetVertices(const float * & output_array_pointer, int & output_array_num) const;
	void GetFaces(const int * & output_array_pointer, int & output_array_num) const;
	inline int GetTexCoordSets() const {return texcoords.size();}
	void GetTexCoords(size_t set, const float * & output_array_pointer, int & output_array_num) const;

	//C++ style interface functions
	inline int GetNumFaces() const {return faces.size();}
	///array bounds are not checked
	inline std::vector <float> GetVertex(int face_number, int vertex_number) const
	{
		std::vector <float> v3(3);

		for (int i = 0; i < 3; i++)
			v3[i] = vertices[faces[face_number*3+vertex_number]*3+i];

		return v3;
	}
	///array bounds are not checked
	inline std::vector <float> GetNormal(int face_number, int vertex_number) const
	{
		std::vector <float> n3(3);

		for (int i = 0; i < 3; i++)
			n3[i] = normals[faces[face_number*3+vertex_number]*3+i];

		return n3;
	}
	///array bounds are not checked
	inline std::vector <float> GetTextureCoordinate(int face_number, int vertex_number, int tc_set) const
	{
		std::vector <float> t2(2);

		for (int i = 0; i < 2; i++)
			t2[i] = texcoords[tc_set][faces[face_number*3+vertex_number]*2+i];

		return t2;
	}

	void SetToBillboard(float x1, float y1, float x2, float y2);

	void SetTo2DButton(float x, float y, float w, float h, float sidewidth, bool flip=false);

	void SetTo2DBox(float x, float y, float w, float h, float marginwidth, float marginheight, float clipx=1.f);

	void SetTo2DQuad(float x1, float y1, float x2, float y2, float u1, float v1, float u2, float v2, float z);

	void SetVertexData2DQuad(float x1, float y1, float x2, float y2, float u1, float v1, float u2, float v2, float * vcorners, float * uvs, int * bfaces, int faceoffset=0) const;

	void SetToUnitCube();

	bool Serialize(joeserialize::Serializer & s)
	{
		_SERIALIZE_(s,vertices);
		_SERIALIZE_(s,normals);
		_SERIALIZE_(s,texcoords);
		_SERIALIZE_(s,faces);
		return true;
	}

	///build the vertex array given the faces defined by the verts, normals, and texcoords passed in
	class TRIFLOAT
	{
		public:
			TRIFLOAT(float nx, float ny, float nz) : x(nx), y(ny), z(nz) {}
			float x,y,z;
			TRIFLOAT() : x(0), y(0), z(0) {}
	};
	class TWOFLOAT
	{
		public:
			TWOFLOAT(float nu, float nv) : u(nu), v(nv) {}
			float u,v;
			TWOFLOAT() : u(0), v(0) {}
	};
	class VERTEXDATA
	{
		public:
			VERTEXDATA() {}
			VERTEXDATA(TRIFLOAT nv, TRIFLOAT nn, TWOFLOAT nt) : vertex(nv), normal(nn), texcoord(nt) {}

			TRIFLOAT vertex;
			TRIFLOAT normal;
			TWOFLOAT texcoord;

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
	class FACE
	{
		public:
			FACE(VERTEXDATA nv1, VERTEXDATA nv2, VERTEXDATA nv3) : v1(nv1),v2(nv2),v3(nv3) {}
			VERTEXDATA v1, v2, v3;
			const VERTEXDATA & GetVertexData(int index) const
			{
				assert(index == 0 || index == 1 || index == 2);
				switch (index)
				{
					case 0:
						return v1;
						break;
					case 1:
						return v2;
						break;
					case 2:
						return v3;
						break;
					default:
						assert(0);
						return v1;	//never reached
						break;
				}
			}
	};
	void BuildFromFaces(const std::vector <FACE> & faces);
	void Translate(float x, float y, float z);
	void Rotate(float a, float x, float y, float z);
	void Scale(float x, float y, float z);
	void FlipNormals();
};

#endif
