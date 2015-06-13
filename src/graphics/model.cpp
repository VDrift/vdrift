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
#include <fstream>
#include <string>
#include <limits>

static const std::string file_magic = "OGLVARRAYV01";

Model::Model() :
	radius(0),
	generatedmetrics(false)
{
	// Constructor.
}

Model::Model(const std::string & filepath, std::ostream & error_output) :
	radius(0),
	generatedmetrics(false)
{
	if (filepath.size() > 4 && filepath.substr(filepath.size()-4) == ".ova")
		ReadFromFile(filepath, error_output);
	else
		Load(filepath, error_output);
}

Model::~Model()
{
	Clear();
}

bool Model::CanSave() const
{
	return false;
}

bool Model::Save(const std::string & /*strFileName*/, std::ostream & /*error_output*/) const
{
	return false;
}

bool Model::Load(const std::string & /*strFileName*/, std::ostream & /*error_output*/)
{
	return false;
}

bool Model::Load(const VertexArray & nvarray, std::ostream & /*error_output*/)
{
	Clear();

	varray = nvarray;

	GenMeshMetrics();

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

bool Model::ReadFromFile(const std::string & filepath, std::ostream & error_output)
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
