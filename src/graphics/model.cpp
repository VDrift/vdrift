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
#include "vertexattrib.h"
#include "glutil.h"
#include "glew.h"
#include "utils.h"
#include <limits>

#define ERROR_CHECK CheckForOpenGLErrors(std::string(__PRETTY_FUNCTION__)+":"+__FILE__+":"+Utils::tostr(__LINE__), error_output)

static const std::string file_magic = "OGLVARRAYV01";

Model::Model() :
	element_count(0),
	vao(0),
	radius(0),
	generatedmetrics(false)
{
	// Constructor.
}

Model::Model(const std::string & filepath, std::ostream & error_output) :
	element_count(0),
	vao(0),
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

bool Model::Load(const VertexArray & nvarray, std::ostream & error_output, bool genlist)
{
	Clear();

	varray = nvarray;

	GenMeshMetrics();

	if (!genlist)
		GenVertexArrayObject(error_output);

	return true;
}

bool Model::Serialize(joeserialize::Serializer & s)
{
	_SERIALIZE_(s, varray);
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

	GenMeshMetrics();

	if (!genlist)
		GenVertexArrayObject(error_output);

	return true;
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
	using namespace VertexAttrib;

	if (HaveVertexArrayObject())
		return;

	// Generate vertex array object.
	glGenVertexArrays(1, &vao);ERROR_CHECK;
	glBindVertexArray(vao);ERROR_CHECK;

	// Buffer object for faces.
	const unsigned int * faces;
	int fcount;
	varray.GetFaces(faces, fcount);
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
	varray.GetVertices(verts, vcount);
	assert(verts && vcount > 0);
	const int vertex_count = vcount / 3;
	vbos.push_back(GenerateBuffer(error_output, VertexPosition, verts, vertex_count, 3));

	// Generate buffer object for normals.
	const float * norms;
	int ncount;
	varray.GetNormals(norms, ncount);
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
	varray.GetTexCoords(tcos, tcount);
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
	varray.GetColors(cols, ccount);
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

bool Model::GetVertexArrayObject(unsigned & vao_out, unsigned int & element_count_out) const
{
	if (!HaveVertexArrayObject())
		return false;

	vao_out = vao;
	element_count_out = element_count;

	return true;
}

void Model::GenMeshMetrics()
{
	const float fmax = std::numeric_limits<float>::max();
	float maxv[3] = {-fmax, -fmax, -fmax};
	float minv[3] = {+fmax, +fmax, +fmax};

	const float * verts;
	int vnum3;
	varray.GetVertices(verts, vnum3);
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

void Model::Clear()
{
	ClearMeshData();
	ClearVertexArrayObject();
	ClearMetrics();
}

const VertexArray & Model::GetVertexArray() const
{
	return varray;
}

bool Model::Loaded() const
{
	return (varray.GetNumIndices() > 0);
}

void Model::RequireMetrics() const
{
	// Mesh metrics need to be generated before they can be queried.
	assert(generatedmetrics);
}

void Model::ClearMetrics()
{
	generatedmetrics = false;
}

void Model::ClearMeshData()
{
	varray.Clear();
}
