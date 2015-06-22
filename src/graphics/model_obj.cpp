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

#include "model_obj.h"
#include "unittest.h"
#include "vertexarray.h"

#include <algorithm>

#include <fstream>
using std::ifstream;

#include <string>
using std::string;

#include <sstream>
using std::istringstream;

#include <iostream>
using std::ostream;
using std::endl;

#include <vector>
using std::vector;

///extract a formatted string from the output, ignoring comments
string ReadFromStream(std::istream & s)
{
	if (!s)
		return "";
	std::streampos curpos = s.tellg();
	assert(curpos >= 0);
	string str;
	s >> str;
	if (!s || str.empty())
		return "";
	if (str[0] == '#')
	{
		s.seekg(curpos);
		std::getline(s,str);
		return ReadFromStream(s);
	}
	return str;
}

///extract a repeating value, return true on success.
template <typename T>
bool ExtractRepeating(std::vector <T> & output_vector, unsigned int repeats, std::istream & s)
{
	if (!s)
		return false;
	for (unsigned int i = 0; i < repeats; i++)
	{
		string strformat = ReadFromStream(s);
		if (strformat.empty())
			return false;
		istringstream reformat(strformat);
		T reformatted;
		reformat >> reformatted;
		output_vector.push_back(reformatted);
	}

	if (output_vector.size() != repeats)
		return false;

	return true;
}

bool ExtractTriFloat(vector <VertexArray::Float3> & output_vector, const std::string & section, std::istream & s,  std::ostream & error_log, const std::string & filepath)
{
	vector <float> coords;
	if (!ExtractRepeating(coords, 3, s))
	{
		error_log << "Error reading " << section << " in " << filepath << endl;
		return false;
	}
	output_vector.push_back(VertexArray::Float3(coords[0],coords[1],coords[2]));
	return true;
}

bool ExtractTwoFloat(vector <VertexArray::Float2> & output_vector, const std::string & section, std::istream & s,  std::ostream & error_log, const std::string & filepath)
{
	vector <float> coords;
	if (!ExtractRepeating(coords, 2, s))
	{
		error_log << "Error reading " << section << " in " << filepath << endl;
		return false;
	}
	output_vector.push_back(VertexArray::Float2(coords[0],coords[1]));
	return true;
}

bool BuildVertex(VertexArray::VertexData & outputvert, vector <VertexArray::Float3> & verts, vector <VertexArray::Float3> & normals, vector <VertexArray::Float2> & texcoords, const string & facestr)
{
	if (std::count(facestr.begin(), facestr.end(), '/') != 2)
		return false;

	if (facestr.find("//") != string::npos)
		return false;

	string facestr2 = facestr;
	std::replace(facestr2.begin(), facestr2.end(), '/', ' ');
	istringstream s(facestr2);
	int v(-1),t(-1),n(-1);
	s >> v >> t >> n;
	if (v <= 0 || t <= 0 || n <= 0)
		return false;

	outputvert.vertex = verts[v-1];
	outputvert.normal = normals[n-1];
	outputvert.texcoord = texcoords[t-1];

	return true;
}

bool ModelObj::Load(const std::string & filepath, std::ostream & error_log)
{
	ifstream f(filepath.c_str());
	if (!f)
	{
		error_log << "Couldn't open object file: " << filepath << endl;
		return false;
	}

	vector <VertexArray::Float3> verts;
	vector <VertexArray::Float3> normals;
	vector <VertexArray::Float2> texcoords;
	vector <VertexArray::Face> faces;

	while (f)
	{
		string id = ReadFromStream(f);

		if (id == "v")
		{
			if (!ExtractTriFloat(verts, "vertices", f, error_log, filepath)) return false;
		}
		else if (id == "vn")
		{
			if (!ExtractTriFloat(normals, "normals", f, error_log, filepath)) return false;
		}
		else if (id == "vt")
		{
			if (!ExtractTwoFloat(texcoords, "texcoords", f, error_log, filepath)) return false;
			texcoords.back().v = 1.0-texcoords.back().v;
		}
		else if (id == "f")
		{
			vector <string> faceverts;
			if (!ExtractRepeating(faceverts, 3, f))
			{
				error_log << "Error reading faces in " << filepath << endl;
				return false;
			}
			VertexArray::VertexData newverts[3];
			for (int i = 0; i < 3; i++)
			{
				if (!BuildVertex(newverts[i], verts, normals, texcoords, faceverts[i]))
				{
					error_log << "Error: obj file has faces without texture and normal data: " << faceverts[i] << " in " << filepath << endl;
					return false;
				}
			}
			VertexArray::Face newface(newverts[0], newverts[1], newverts[2]);
			faces.push_back(newface);
		}
	}

	varray.BuildFromFaces(faces);
	GenMeshMetrics();

	return true;
}

void WriteRange(std::ostream & s, const vector <float> & v, int startidx, int endidx)
{
	assert(startidx >= 0 && startidx < (int) v.size() && startidx < endidx && startidx != endidx);
	assert(endidx <= (int) v.size());
	for (int i = startidx; i < endidx; i++)
	{
		if (i != startidx)
			s << " ";
		s << v[i];
	}
}

void WriteVectorGroupings(std::ostream & s, const vector <float> & v, const std::string & id, int groupsize)
{
	assert(groupsize > 0);
	for (unsigned int i = 0; i < v.size() / groupsize; i++)
	{
		s << id << " ";
		WriteRange(s, v, i*groupsize, i*groupsize+groupsize);
		s << endl;
	}
}

void WriteFace(std::ostream & s, int index)
{
	s << index << "/" << index << "/" << index;
}

bool ModelObj::Save(const std::string & strFileName, std::ostream & error_output) const
{
	std::ofstream f(strFileName.c_str());
	if (!f)
	{
		error_output << "Error opening file for writing: " << strFileName << endl;
		return false;
	}

	f << "# Model conversion utility by Joe Venzon" << endl << endl;

	WriteVectorGroupings(f, varray.vertices, "v", 3);
	f << endl;
	WriteVectorGroupings(f, varray.texcoords, "vt", 2);
	f << endl;
	WriteVectorGroupings(f, varray.normals, "vn", 3);
	f << endl;

	for (unsigned int i = 0; i < varray.faces.size() / 3; i++)
	{
		f << "f ";
		for (int v = 0; v < 3; v++)
		{
			WriteFace(f, varray.faces[i*3+v]+1);
			f << " ";
		}
		f << endl;
	}

	return true;
}

