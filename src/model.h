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

#ifndef _MODEL_H
#define _MODEL_H

#include "opengl_utility.h"
#include "vertexarray.h"
#include "mathvector.h"
#include "aabb.h"

/// Loading data into the mesh vertexarray is implemented by derived classes.
class MODEL
{
friend class joeserialize::Serializer;
public:
	MODEL();

	MODEL(const std::string & filepath, std::ostream & error_output);

	virtual ~MODEL();

	/// Returns true if the model format is capable of saving to a file.
	virtual bool CanSave() const;

	/// Optional capability.
	virtual bool Save(const std::string & strFileName, std::ostream & error_output) const;

	virtual bool Load(const std::string & strFileName, std::ostream & error_output, bool genlist);

	bool Load(const VERTEXARRAY & varray, std::ostream & error_output, bool genlist);

	bool Serialize(joeserialize::Serializer & s);

	bool WriteToFile(const std::string & filepath);

	bool ReadFromFile(const std::string & filepath, std::ostream & error_output, bool generatelistid=true);

	void GenerateListID(std::ostream & error_output);

	void GenerateVertexArrayObject(std::ostream & error_output);
	bool HaveVertexArrayObject() const;
	void ClearVertexArrayObject();

	/// Returns true if we have a vertex array object and stores the VAO handle and element count in the provided arguments.
	/// Returns false if we have no vertex array object.
	bool GetVertexArrayObject(GLuint & vao_out, unsigned int & elementCount_out) const;

	void GenerateMeshMetrics();

	void ClearMeshData();

	int GetListID() const;

	float GetRadius() const;

	float GetRadiusXZ() const;

	MATHVECTOR <float, 3> GetCenter();

	bool HaveMeshData() const;

	bool HaveMeshMetrics() const;

	bool HaveListID() const;

	void Clear();

	const VERTEXARRAY & GetVertexArray() const;

	void SetVertexArray(const VERTEXARRAY & newmesh);

	void BuildFromVertexArray(const VERTEXARRAY & newmesh);

	bool Loaded();

	void Translate(float x, float y, float z);

	void Rotate(float a, float x, float y, float z);

	void Scale(float x, float y, float z);

	AABB <float> GetAABB() const;

protected:
	/// To be filled by the derived classes.
	VERTEXARRAY mesh;

private:
	bool generatedlistid;
	bool generatedmetrics;
	int listid;

	bool generatedvao;
	/// VAO means vertex array object.
	GLuint vao;
	std::vector <GLuint> vbos;
	GLuint elementVbo;
	unsigned int elementCount;

	// Metrics.
	float radius;
	float radiusxz;
	MATHVECTOR <float, 3> bboxmin;
	MATHVECTOR <float, 3> bboxmax;

	void RequireMetrics() const;

	void RequireListID() const;

	void ClearListID();

	void ClearMetrics();
};

#endif
