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

#ifndef _VERTEX_BUFFER_H
#define _VERTEX_BUFFER_H

#include "vertexformat.h"
#include <vector>

class SceneNode;
class VertexArray;

/// \class VertexBuffer
/// \brief This class is responsible for vertex data batching, upload and drawing
class VertexBuffer
{
public:
	VertexBuffer();

	~VertexBuffer();

	/// \brief Clear all buffers and objects, will reset gl vbo, vao state
	void Clear();

	/// \brief Bind scene node drawables vertex data and upload it to gpu
	/// \param nodes is the node pointer array to be processed
	/// \param nodes_count is the size of the node array
	void Set(SceneNode * nodes[], unsigned int nodes_count);

	/// \brief A vertex buffer segment
	struct Segment
	{
		Segment();

		unsigned int ioffset;		///< index start byte offset
		unsigned int icount;		///< index count
		unsigned int voffset;		///< vertex start element index
		unsigned int vcount;		///< vertex count
		unsigned int vbuffer;		///< vertex buffer / array object
		unsigned short object;		///< object this segment belongs to
		unsigned short age;			///< segment age
	};

	/// \brief Draw vertex buffer segment
	/// \param vbuffer is the currently bound vertex buffer / array object
	/// \param segment is the segment to be drawn
	void Draw(unsigned int & vbuffer, const Segment & segment);

private:
	struct Object
	{
		Object();

		unsigned int icount;		///< index count
		unsigned int vcount;		///< vertex count
		unsigned int ibuffer;		///< index buffer object
		unsigned int vbuffer;		///< vertex buffer object
		unsigned int varray;		///< vertex array object
		VertexFormat::Enum vformat;	///< vertex format
	};
	std::vector<Object> objects[VertexFormat::LastFormat + 1];
	unsigned short age;
	bool use_vao;

	/// \brief Scene node visitor
	struct BindVertexData;

	/// \brief Upload vertex data to gpu
	static void UploadVertexData(
		const std::vector<Object> & objects,
		const std::vector<const VertexArray *> & varrays,
		std::vector<float> & vertex_buffer,
		std::vector<int> & index_buffer,
		const bool use_vao);

	/// \brief Set vertex format of currently bound vertex array
	static void SetVertexFormat(const VertexFormat & vf);
};

#endif // _VERTEX_BUFFER_H
