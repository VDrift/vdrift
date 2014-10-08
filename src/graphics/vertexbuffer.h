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

	/// \brief Intel vao implementation doesn't store ibo binding workaround
	void BindElementBufferExplicitly();

	/// \brief Clear all buffers and objects, will reset gl vbo, vao state
	void Clear();

	/// \brief Bind dynamic scene node drawables vertex data and upload it to gpu
	/// \param nodes is the node pointer array to be processed
	/// \param count is the size of the node array
	void SetDynamicVertexData(SceneNode * nodes[], unsigned int count);

	/// \brief Bind static scene node drawables vertex data and upload it to gpu
	/// \param nodes is the node pointer array to be processed
	/// \param count is the size of the node array
	void SetStaticVertexData(SceneNode * nodes[], unsigned int count);

	/// \brief A vertex buffer segment
	struct Segment
	{
		unsigned int ioffset;		///< index start offset in bytes
		unsigned int icount;		///< index count
		unsigned int voffset;		///< vertex start element index
		unsigned int vcount;		///< vertex count
		unsigned int vbuffer;		///< vertex buffer / array object
		unsigned char vformat;		///< vertex format
		unsigned char object;		///< object this segment belongs to
		unsigned short age;			///< segment age
		Segment();
	};

	/// \brief Draw vertex buffer segment
	/// \param vbuffer is the currently bound vertex buffer / array object
	/// \param segment is the segment to be drawn
	void Draw(unsigned int & vbuffer, const Segment & segment) const;

private:
	/// \brief Buffer objects store gpu buffer state
	struct Object
	{
		unsigned int icapacity;		///< index buffer size
		unsigned int vcapacity;		///< vertex buffer size
		unsigned int icount;		///< index count
		unsigned int vcount;		///< vertex count
		unsigned int ibuffer;		///< index buffer object
		unsigned int vbuffer;		///< vertex buffer object
		unsigned int varray;		///< vertex array object
		VertexFormat::Enum vformat;	///< vertex format
		Object();
	};
	std::vector<Object> objects[VertexFormat::LastFormat + 1];

	/// Staging buffers for dynamic vertex data updates
	std::vector<unsigned int> staging_index_buffer[VertexFormat::LastFormat + 1];
	std::vector<float> staging_vertex_buffer[VertexFormat::LastFormat + 1];

	/// Buffer age counters used for debugging
	unsigned short age_dynamic;
	unsigned short age_static;

	bool use_vao;
	bool good_vao; ///< handle implementations returning vao 0
	bool bind_ibo; ///< workaround for broken vao implementation

	/// \brief Scene node visitors
	struct BindStaticVertexData;
	struct BindDynamicVertexData;

	/// \brief Bind vao/vbo+ibo of segment and update vbuffer value
	void BindSegmentBuffer(unsigned int & vbuffer, const Segment & segment) const;

	/// \brief Init dynamic vertex data objects
	void InitDynamicBufferObjects();

	/// \brief Upload dynamic vertex data to gpu
	static void UploadDynamicVertexData(
		std::vector<Object> & objects,
		const std::vector<unsigned int> & index_buffer,
		const std::vector<float> & vertex_buffer);

	/// \brief Upload static vertex data to gpu
	static void UploadStaticVertexData(
		std::vector<Object> & objects,
		const std::vector<const VertexArray *> & varrays,
		std::vector<unsigned int> & index_buffer,
		std::vector<float> & vertex_buffer);

	/// \brief Write vertex array indices into staging buffer
	static unsigned int WriteIndices(
		const VertexArray & va,
		const unsigned int icount,
		const unsigned int vcount,
		std::vector<unsigned int> & index_buffer);

	/// \brief Write vertex array vertices into staging buffer
	static unsigned int WriteVertices(
		const VertexArray & va,
		const unsigned int vcount,
		const unsigned int vertex_size,
		std::vector<float> & vertex_buffer);

	/// \brief Upload staging data into object vbo/ibo
	static void UploadBuffers(
		Object & object,
		const std::vector<unsigned int> & index_buffer,
		const std::vector<float> & vertex_buffer);

	/// \brief Set vertex format of currently bound vertex array
	static void SetVertexFormat(const VertexFormat & vf);
};

#endif // _VERTEX_BUFFER_H
