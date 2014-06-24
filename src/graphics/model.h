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

#include "vertexarray.h"
#include "vertexbuffer.h"
#include "mathvector.h"

/// Loading data into the mesh vertexarray is implemented by derived classes.
class Model
{
friend class joeserialize::Serializer;
public:
	Model();

	Model(const std::string & filepath, std::ostream & error_output);

	virtual ~Model();

	/// Returns true if the model format is capable of saving to a file.
	virtual bool CanSave() const;

	/// Optional capability.
	virtual bool Save(const std::string & strFileName, std::ostream & error_output) const;

	virtual bool Load(const std::string & strFileName, std::ostream & error_output, bool genlist);

	bool Load(const VertexArray & varray, std::ostream & error_output, bool genlist);

	bool Serialize(joeserialize::Serializer & s);

	bool WriteToFile(const std::string & filepath);

	bool ReadFromFile(const std::string & filepath, std::ostream & error_output, bool genlist);

	void GenVertexArrayObject(std::ostream & error_output);

	bool HaveVertexArrayObject() const;

	void ClearVertexArrayObject();

	/// Returns true if we have a vertex array object and stores the VAO handle and element count in the provided arguments.
	/// Returns false if we have no vertex array object.
	bool GetVertexArrayObject(unsigned & vao_out, unsigned & element_count_out) const;

	/// vertex buffer interface
	VertexBuffer::Segment & GetVertexBufferSegment() { return vbs; };

	/// Recalculate mesh bounding box and radius
	void GenMeshMetrics();

	/// Get aabb size.
	Vec3 GetSize() const;

	/// Get aabb center.
	Vec3 GetCenter() const;

	/// Get bounding radius relative to center.
	float GetRadius() const;

	void Clear();

	const VertexArray & GetVertexArray() const;

	bool Loaded() const;

protected:
	VertexArray m_mesh;			///< to be filled by the derived classes

private:
	VertexBuffer::Segment vbs;	///< vertex buffer segment
	std::vector<unsigned> vbos;	///< vertex buffer objects
	unsigned element_count;		///< number of indices
	unsigned vao;				///< vertex array object, 0 is reserved as invalid

	/// Metrics
	Vec3 min;
	Vec3 max;
	float radius;
	bool generatedmetrics;

	void RequireMetrics() const;

	void ClearMetrics();

	void ClearMeshData();
};

#endif
