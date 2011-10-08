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

#include "model.h"
#include "utils.h"
#include "vertexattribs.h"
using namespace VERTEX_ATTRIBS;

#define ERROR_CHECK OPENGL_UTILITY::CheckForOpenGLErrors(std::string(__PRETTY_FUNCTION__)+":"+__FILE__+":"+UTILS::tostr(__LINE__), error_output)

static const char file_magic[] = "OGLVARRAYV01";

MODEL::MODEL() : generatedlistid(false), generatedmetrics(false), generatedvao(false), vao(0), elementVbo(0), elementCount(0), radius(0), radiusxz(0)
{
	// Constructor.
}

MODEL::MODEL(const std::string & filepath, std::ostream & error_output) : generatedlistid(false), generatedmetrics(false), generatedvao(false), vao(0), elementVbo(0), elementCount(0), radius(0), radiusxz(0)
{
	if (filepath.size() > 4 && filepath.substr(filepath.size()-4) == ".ova")
		ReadFromFile(filepath, error_output, false);
	else
		Load(filepath, error_output, false);
}

MODEL::~MODEL()
{
	Clear();
}

bool MODEL::CanSave() const
{
	return false;
}

bool MODEL::Save(const std::string & strFileName, std::ostream & error_output) const
{
	return false;
}

bool MODEL::Load(const std::string & strFileName, std::ostream & error_output, bool genlist)
{
	return false;
}

bool MODEL::Load(const VERTEXARRAY & varray, std::ostream & error_output, bool genlist)
{
	BuildFromVertexArray(varray);
	if (genlist)
		GenerateListID(error_output);
	else
		GenerateVertexArrayObject(error_output);
	return true;
}

bool MODEL::Serialize(joeserialize::Serializer & s)
{
	_SERIALIZE_(s,mesh);
	return true;
}

bool MODEL::WriteToFile(const std::string & filepath)
{
	std::ofstream fileout(filepath.c_str());
	if (!fileout)
		return false;

	fileout.write(file_magic, sizeof(file_magic));
	joeserialize::BinaryOutputSerializer s(fileout);
	return Serialize(s);
}

bool MODEL::ReadFromFile(const std::string & filepath, std::ostream & error_output, bool generatelistid)
{
	std::ifstream filein(filepath.c_str(), std::ios_base::binary);
	if (!filein)
	{
		error_output << "Can't find file: " << filepath << std::endl;
		return false;
	}

	char fmagic[sizeof(file_magic)+1];
	filein.read(fmagic, sizeof(file_magic));
	if (!filein)
	{
		error_output << "File magic read error: " << filepath << std::endl;
		return false;
	}

	fmagic[sizeof(file_magic)] = '\0';
	if (file_magic != fmagic)
	{
		error_output << "File magic is incorrect: \"" << file_magic << "\" != \"" << fmagic << "\" in " << filepath << std::endl;
		return false;
	}

	joeserialize::BinaryInputSerializer s(filein);
	if (!Serialize(s))
	{
		error_output << "Serialization error: " << filepath << std::endl;
		Clear();
		return false;
	}

	ClearListID();
	ClearMetrics();
	GenerateMeshMetrics();

	if (generatelistid)
		GenerateListID(error_output);

	return true;
}

void MODEL::GenerateListID(std::ostream & error_output)
{
	if (HaveListID())
		return;

	ClearListID();

	listid = glGenLists(1);
	glNewList (listid, GL_COMPILE);

	glBegin(GL_TRIANGLES);

	int true_faces = mesh.GetNumFaces() / 3;

	// Iterate through all of the faces (polygons).
	for (int j = 0; j < true_faces; j++)
		// Iterate though each vertex in the face.
		for (int whichVertex = 0; whichVertex < 3; whichVertex++)
		{
			// Get the 3D location for this vertex.
			/// Vert array bounds are not checked but it is assumed to be of size 3.
			std::vector <float> vert = mesh.GetVertex(j, whichVertex);

			// Get the 3D normal for this vertex.
			/// Norm array bounds are not checked but it is assumed to be of size 3.
			std::vector <float> norm = mesh.GetNormal(j, whichVertex);

			assert(mesh.GetTexCoordSets() > 0);

			if (mesh.GetTexCoordSets() > 0)
			{
				// Get the 2D texture coordinates for this vertex.
				///Tex array bounds are not checked but it is assumed to be of size 2.
				std::vector <float> tex = mesh.GetTextureCoordinate(j, whichVertex, 0);

				glMultiTexCoord2f(GL_TEXTURE0, tex[0], tex[1]);
			}

			glNormal3fv(&norm[0]);
			glVertex3fv(&vert[0]);
		}

	glEnd();

	glEndList ();

	generatedlistid = true;

	OPENGL_UTILITY::CheckForOpenGLErrors("model list ID generation", error_output);
}

template <typename T>
GLuint GenerateBufferObject(T * data, unsigned int i, unsigned int vertexCount, unsigned int elementsPerVertex, std::ostream & error_output)
{
	GLuint vboHandle;
	glGenBuffers(1, &vboHandle);ERROR_CHECK;
	glBindBuffer(GL_ARRAY_BUFFER, vboHandle);ERROR_CHECK;
	glBufferData(GL_ARRAY_BUFFER, vertexCount*elementsPerVertex*sizeof(T), data, GL_STATIC_DRAW);ERROR_CHECK;
	glVertexAttribPointer(i, elementsPerVertex, GL_FLOAT, GL_FALSE, 0, 0);ERROR_CHECK;
	glEnableVertexAttribArray(i);ERROR_CHECK;

	return vboHandle;
}

void MODEL::GenerateVertexArrayObject(std::ostream & error_output)
{
	if (generatedvao)
		return;

	// Generate vertex array object.
	glGenVertexArrays(1, &vao);ERROR_CHECK;
	glBindVertexArray(vao);ERROR_CHECK;

	// Buffer object for faces.
	const int * faces;
	int facecount;
	mesh.GetFaces(faces, facecount);
	assert(faces && facecount > 0);
	glGenBuffers(1, &elementVbo);ERROR_CHECK;
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementVbo);ERROR_CHECK;
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, facecount*sizeof(GLuint), faces, GL_STATIC_DRAW);ERROR_CHECK;
	elementCount = facecount;

	// Calculate the number of vertices (vertcount is the size of the verts array).
	const float * verts;
	int vertcount;
	mesh.GetVertices(verts, vertcount);
	assert(verts && vertcount > 0);
	unsigned int vertexCount = vertcount/3;

	// Generate buffer object for vertex positions.
	vbos.push_back(GenerateBufferObject(verts, VERTEX_POSITION, vertexCount, 3, error_output));

	// Generate buffer object for normals.
	const float * norms;
	int normcount;
	mesh.GetNormals(norms, normcount);
	if (!norms || normcount <= 0)
		glDisableVertexAttribArray(VERTEX_NORMAL);
	else
	{
		assert((unsigned int)normcount == vertexCount*3);
		vbos.push_back(GenerateBufferObject(norms, VERTEX_NORMAL, vertexCount, 3, error_output));
	}

	// TODO: Generate tangent and bitangent.
	glDisableVertexAttribArray(VERTEX_TANGENT);
	glDisableVertexAttribArray(VERTEX_BITANGENT);

	glDisableVertexAttribArray(VERTEX_COLOR);

	// Generate buffer object for texture coordinates.
	const float * tc[1];
	int tccount[1];
	if (mesh.GetTexCoordSets() > 0)
	{
		// TODO: Make this work for UV1 and UV2.
		mesh.GetTexCoords(0, tc[0], tccount[0]);
		assert((unsigned int)tccount[0] == vertexCount*2);
		vbos.push_back(GenerateBufferObject(tc[0], VERTEX_UV0, vertexCount, 2, error_output));
	}
	else
		glDisableVertexAttribArray(VERTEX_UV0);

	glDisableVertexAttribArray(VERTEX_UV1);
	glDisableVertexAttribArray(VERTEX_UV2);

	// Don't leave anything bound.
	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER,0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,0);

	generatedvao = true;
}

bool MODEL::HaveVertexArrayObject() const
{
	return generatedvao;
}

void MODEL::ClearVertexArrayObject()
{
	if (generatedvao)
	{
		glBindBuffer(GL_ARRAY_BUFFER,0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,0);
		if (!vbos.empty())
			glDeleteBuffers(vbos.size(), &vbos[0]);

		if (elementVbo != 0)
		{
			glDeleteBuffers(1, &elementVbo);
			elementVbo = 0;
		}
		if (vao != 0)
		{
			glBindVertexArray(0);
			glDeleteVertexArrays(1,&vao);
			vao = 0;
		}
	}
	generatedlistid = false;
}

bool MODEL::GetVertexArrayObject(GLuint & vao_out, unsigned int & elementCount_out) const
{
	if (!generatedvao)
		return false;

	vao_out = vao;
	elementCount_out = elementCount;

	return true;
}

void MODEL::GenerateMeshMetrics()
{
	float maxv[3] = {0, 0, 0};
	float minv[3] = {0, 0, 0};
	bool havevals[6];
	for (int n = 0; n < 6; n++)
		havevals[n] = false;

	const float * verts;
	int vnum;
	mesh.GetVertices(verts, vnum);
	vnum /= 3;
	for (int v = 0; v < vnum; v++)
	{
		MATHVECTOR <float, 3> temp;

		temp.Set(verts + v*3);

		// Cache for bbox stuff.
		for (int n = 0; n < 3; n++)
		{
			if (!havevals[n])
			{
				maxv[n] = temp[n];
				havevals[n] = true;
			}
			else if (temp[n] > maxv[n])
				maxv[n] = temp[n];

			if (!havevals[n+3])
			{
				minv[n] = temp[n];
				havevals[n+3] = true;
			}
			else if (temp[n] < minv[n])
				minv[n] = temp[n];
		}

		float r = temp.Magnitude();
		MATHVECTOR <float, 2> tempxz;
		tempxz.Set(temp[0], temp[2]);
		float rxz = tempxz.Magnitude();
		if (r > radius)
			radius = r;
		if (rxz > radiusxz)
			radiusxz = rxz;
	}

	bboxmin.Set(minv[0], minv[1], minv[2]);
	bboxmax.Set(maxv[0], maxv[1], maxv[2]);

	MATHVECTOR <float, 3> center;
	center = (bboxmin + bboxmax)*0.5;
	radius = (bboxmin - center).Magnitude();

	MATHVECTOR <float, 3> minv_noy = bboxmin;
	minv_noy[1] = 0;
	center[1] = 0;
	radiusxz = (minv_noy - center).Magnitude();

	generatedmetrics = true;
}

void MODEL::ClearMeshData()
{
	mesh.Clear();
}

int MODEL::GetListID() const
{
	RequireListID();
	return listid;
}

float MODEL::GetRadius() const
{
	RequireMetrics();
	return radius + 0.5f;
}

float MODEL::GetRadiusXZ() const
{
	RequireMetrics();
	return radiusxz;
}

MATHVECTOR <float, 3> MODEL::GetCenter()
{
	return (bboxmax + bboxmin) * 0.5;
}

bool MODEL::HaveMeshData() const
{
	return (mesh.GetNumFaces() > 0);
}

bool MODEL::HaveMeshMetrics() const
{
	return generatedmetrics;
}

bool MODEL::HaveListID() const
{
	return generatedlistid;
}

void MODEL::Clear()
{
	ClearMeshData();
	ClearListID();
	ClearVertexArrayObject();
	ClearMetrics();
}

const VERTEXARRAY & MODEL::GetVertexArray() const
{
	return mesh;
}

void MODEL::SetVertexArray(const VERTEXARRAY & newmesh)
{
	Clear();
	mesh = newmesh;
}

void MODEL::BuildFromVertexArray(const VERTEXARRAY & newmesh)
{
	SetVertexArray(newmesh);

	// Generate metrics such as bounding box, etc.
	GenerateMeshMetrics();
}

bool MODEL::Loaded()
{
	return (mesh.GetNumFaces() > 0);
}

void MODEL::Translate(float x, float y, float z)
{
	mesh.Translate(x, y, z);
}

void MODEL::Rotate(float a, float x, float y, float z)
{
	mesh.Rotate(a, x, y, z);
}

void MODEL::Scale(float x, float y, float z)
{
	mesh.Scale(x,y,z);
}

AABB <float> MODEL::GetAABB() const
{
	AABB <float> output;
	output.SetFromCorners(bboxmin, bboxmax);
	return output;
}

void MODEL::RequireMetrics() const
{
	// Mesh metrics need to be generated before they can be queried.
	assert(generatedmetrics);
}

void MODEL::RequireListID() const
{
	// Mesh id needs to be generated.
	assert(generatedlistid);
}

void MODEL::ClearListID()
{
	if (generatedlistid)
		glDeleteLists(listid, 1);
	generatedlistid = false;
}

void MODEL::ClearMetrics()
{
	generatedmetrics = false;
}
