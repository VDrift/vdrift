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
#include "vertexattrib.h"
#include "glutil.h"
#include <limits>

using namespace VertexAttrib;

#define ERROR_CHECK CheckForOpenGLErrors(std::string(__PRETTY_FUNCTION__)+":"+__FILE__+":"+Utils::tostr(__LINE__), error_output)

static const std::string file_magic = "OGLVARRAYV01";

Model::Model() :
	vao(0),
	element_count(0),
	listid(0),
	radius(0),
	generatedmetrics(false)
{
	// Constructor.
}

Model::Model(const std::string & filepath, std::ostream & error_output) :
	vao(0),
	element_count(0),
	listid(0),
	radius(0),
	generatedmetrics(false)
{
	if (filepath.size() > 4 && filepath.substr(filepath.size()-4) == ".ova")
		ReadFromFile(filepath, error_output, false);
	else
		Load(filepath, error_output, false);
}

Model::~Model()
{
	Clear();
}

bool Model::CanSave() const
{
	return false;
}

bool Model::Save(const std::string & strFileName, std::ostream & error_output) const
{
	return false;
}

bool Model::Load(const std::string & strFileName, std::ostream & error_output, bool genlist)
{
	return false;
}

bool Model::Load(const VertexArray & varray, std::ostream & error_output, bool genlist)
{
	Clear();

	SetVertexArray(varray);
	GenMeshMetrics();

	if (genlist)
		GenDrawList(error_output);
	else
		GenVertexArrayObject(error_output);

	return true;
}

bool Model::Serialize(joeserialize::Serializer & s)
{
	_SERIALIZE_(s, m_mesh);
	return true;
}

bool Model::WriteToFile(const std::string & filepath)
{
	std::ofstream fileout(filepath.c_str());
	if (!fileout)
		return false;

	fileout.write(file_magic.c_str(), file_magic.size());
	joeserialize::BinaryOutputSerializer s(fileout);
	return Serialize(s);
}

bool Model::ReadFromFile(const std::string & filepath, std::ostream & error_output, bool genlist)
{
	std::ifstream filein(filepath.c_str(), std::ios_base::binary);
	if (!filein)
	{
		error_output << "Can't find file: " << filepath << std::endl;
		return false;
	}

	std::vector<char> fmagic(file_magic.size() + 1, 0);
	filein.read(&fmagic[0], file_magic.size());
	if (!filein)
	{
		error_output << "File magic read error: " << filepath << std::endl;
		return false;
	}

	if (!file_magic.compare(&fmagic[0]))
	{
		error_output << "File magic is incorrect: \"" << file_magic << "\" != \"" << &fmagic[0] << "\" in " << filepath << std::endl;
		return false;
	}

	joeserialize::BinaryInputSerializer s(filein);
	if (!Serialize(s))
	{
		error_output << "Serialization error: " << filepath << std::endl;
		Clear();
		return false;
	}

	ClearDrawList();
	ClearMetrics();
	GenMeshMetrics();

	if (genlist)
		GenDrawList(error_output);

	return true;
}

void Model::GenDrawList(std::ostream & error_output)
{
	ClearDrawList();

	listid = glGenLists(1);

	const int * faces;
	const float * verts;
	const float * norms;
	const float * tcos;
	int fcount;
	int vcount;
	int ncount;
	int tcount;

	m_mesh.GetFaces(faces, fcount);
	m_mesh.GetVertices(verts, vcount);
	m_mesh.GetNormals(norms, ncount);
	m_mesh.GetTexCoords(tcos, tcount);

	assert(fcount > 0);
	assert(vcount > 0);

	glEnableVertexAttribArray(VertexPosition);
	glVertexAttribPointer(VertexPosition, 3, GL_FLOAT, GL_FALSE, 0, verts);
	if (ncount)
	{
		glEnableVertexAttribArray(VertexNormal);
		glVertexAttribPointer(VertexNormal, 3, GL_FLOAT, GL_FALSE, 0, norms);
	}
	if (tcount)
	{
		glEnableVertexAttribArray(VertexTexCoord);
		glVertexAttribPointer(VertexTexCoord, 2, GL_FLOAT, GL_FALSE, 0, tcos);
	}

	glNewList(listid, GL_COMPILE);
	glDrawElements(GL_TRIANGLES, fcount, GL_UNSIGNED_INT, faces);
	glEndList();

	glDisableVertexAttribArray(VertexPosition);
	if (ncount)
		glDisableVertexAttribArray(VertexNormal);
	if (tcount)
		glDisableVertexAttribArray(VertexTexCoord);

	CheckForOpenGLErrors("model list ID generation", error_output);
}

template <typename T>
GLuint GenerateBuffer(
	std::ostream & error_output,
	unsigned attrib_id,
	const T * data,
	unsigned vertex_count,
	unsigned elems_per_vertex,
	GLenum type = GL_FLOAT,
	bool normalized = false)
{
	GLuint vbo = 0;
	glGenBuffers(1, &vbo);ERROR_CHECK;
	glBindBuffer(GL_ARRAY_BUFFER, vbo);ERROR_CHECK;
	glBufferData(GL_ARRAY_BUFFER, vertex_count * elems_per_vertex * sizeof(T), data, GL_STATIC_DRAW);ERROR_CHECK;
	glVertexAttribPointer(attrib_id, elems_per_vertex, type, normalized, 0, 0);ERROR_CHECK;
	glEnableVertexAttribArray(attrib_id);ERROR_CHECK;
	return vbo;
}

void Model::GenVertexArrayObject(std::ostream & error_output)
{
	if (HaveVertexArrayObject())
		return;

	// Generate vertex array object.
	glGenVertexArrays(1, &vao);ERROR_CHECK;
	glBindVertexArray(vao);ERROR_CHECK;

	// Buffer object for faces.
	const int * faces;
	int fcount;
	m_mesh.GetFaces(faces, fcount);
	assert(faces && fcount > 0);
	element_count = fcount;
	GLuint evbo = 0;
	glGenBuffers(1, &evbo);ERROR_CHECK;
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, evbo);ERROR_CHECK;
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, fcount * sizeof(GLuint), faces, GL_STATIC_DRAW);ERROR_CHECK;
	vbos.push_back(evbo);

	// Generate buffer object for vertex positions.
	const float * verts;
	int vcount;
	m_mesh.GetVertices(verts, vcount);
	assert(verts && vcount > 0);
	const int vertex_count = vcount / 3;
	vbos.push_back(GenerateBuffer(error_output, VertexPosition, verts, vertex_count, 3));

	// Generate buffer object for normals.
	const float * norms;
	int ncount;
	m_mesh.GetNormals(norms, ncount);
	if (!norms || ncount <= 0)
		glDisableVertexAttribArray(VertexNormal);
	else
	{
		assert(ncount == vertex_count * 3);
		vbos.push_back(GenerateBuffer(error_output, VertexNormal, norms, vertex_count, 3));
	}

	// TODO: Generate tangent and bitangent.
	glDisableVertexAttribArray(VertexTangent);
	glDisableVertexAttribArray(VertexBitangent);

	// Generate buffer object for texture coordinates.
	const float * tcos;
	int tcount;
	m_mesh.GetTexCoords(tcos, tcount);
	if (tcos && tcount)
	{
		assert(tcount == vertex_count * 2);
		vbos.push_back(GenerateBuffer(error_output, VertexTexCoord, tcos, vertex_count, 2));
	}
	else
		glDisableVertexAttribArray(VertexTexCoord);

	glDisableVertexAttribArray(VertexBlendIndices);
	glDisableVertexAttribArray(VertexBlendWeights);

	// Generate buffer object for colors.
	const unsigned char * cols = 0;
	int ccount = 0;
	m_mesh.GetColors(cols, ccount);
	if (cols && ccount)
	{
		assert(ccount == vertex_count * 4);
		vbos.push_back(GenerateBuffer(error_output, VertexColor, cols, vertex_count, 4, GL_UNSIGNED_BYTE, true));
	}
	else
		glDisableVertexAttribArray(VertexColor);

	// Don't leave anything bound.
	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

bool Model::HaveVertexArrayObject() const
{
	return vao;
}

void Model::ClearVertexArrayObject()
{
	if (!HaveVertexArrayObject())
		return;

	if (!vbos.empty())
	{
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
		glDeleteBuffers(vbos.size(), &vbos[0]);
		vbos.clear();
	}

	if (vao)
	{
		glBindVertexArray(0);
		glDeleteVertexArrays(1, &vao);
		vao = 0;
	}
}

bool Model::GetVertexArrayObject(GLuint & vao_out, unsigned int & element_count_out) const
{
	if (!HaveVertexArrayObject())
		return false;

	vao_out = vao;
	element_count_out = element_count;

	return true;
}

void Model::GenMeshMetrics()
{
	const float flt_max = std::numeric_limits<float>::max();
	const float flt_min = std::numeric_limits<float>::min();
	float maxv[3] = {flt_min, flt_min, flt_min};
	float minv[3] = {flt_max, flt_max, flt_max};

	const float * verts;
	int vnum3;
	m_mesh.GetVertices(verts, vnum3);
	assert(vnum3);

	for (int n = 0; n < vnum3; n += 3)
	{
		const float * v = verts + n;
		if (v[0] > maxv[0]) maxv[0] = v[0];
		if (v[1] > maxv[1]) maxv[1] = v[1];
		if (v[2] > maxv[2]) maxv[2] = v[2];
		if (v[0] < minv[0]) minv[0] = v[0];
		if (v[1] < minv[1]) minv[1] = v[1];
		if (v[2] < minv[2]) minv[2] = v[2];
	}

	min.Set(minv[0], minv[1], minv[2]);
	max.Set(maxv[0], maxv[1], maxv[2]);
	radius = GetSize().Magnitude() * 0.5f + 0.001f;	// 0.001 margin

	generatedmetrics = true;
}

void Model::ClearMeshData()
{
	m_mesh.Clear();
}

unsigned Model::GetDrawList() const
{
	RequireDrawList();
	return listid;
}

Vec3 Model::GetSize() const
{
	return max - min;
}

Vec3 Model::GetCenter() const
{
	return (max + min) * 0.5f;
}

float Model::GetRadius() const
{
	RequireMetrics();
	return radius;
}

bool Model::HaveMeshData() const
{
	return (m_mesh.GetNumFaces() > 0);
}

bool Model::HaveMeshMetrics() const
{
	return generatedmetrics;
}

void Model::Clear()
{
	ClearMeshData();
	ClearDrawList();
	ClearVertexArrayObject();
	ClearMetrics();
}

const VertexArray & Model::GetVertexArray() const
{
	return m_mesh;
}

void Model::SetVertexArray(const VertexArray & newmesh)
{
	m_mesh = newmesh;
}

bool Model::Loaded()
{
	return (m_mesh.GetNumFaces() > 0);
}

void Model::RequireMetrics() const
{
	// Mesh metrics need to be generated before they can be queried.
	assert(generatedmetrics);
}

void Model::RequireDrawList() const
{
	// Mesh id needs to be generated.
	assert(listid);
}

void Model::ClearDrawList()
{
	if (listid)
		glDeleteLists(listid, 1);
	listid = 0;
}

void Model::ClearMetrics()
{
	generatedmetrics = false;
}
