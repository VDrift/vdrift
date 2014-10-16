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

#include <iosfwd>
#include <string>

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

	virtual bool Load(const std::string & strFileName, std::ostream & error_output);

	bool Load(const VertexArray & nvarray, std::ostream & error_output);

	bool Serialize(joeserialize::Serializer & s);

	bool WriteToFile(const std::string & filepath);

	bool ReadFromFile(const std::string & filepath, std::ostream & error_output);

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
	VertexArray varray;			///< to be filled by the derived classes

private:
	VertexBuffer::Segment vbs;	///< vertex buffer segment

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
