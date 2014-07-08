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

#include "vertexbuffer.h"
#include "scenenode.h"
#include "model.h"

//#define VAO_BROKEN

static const unsigned int max_buffer_size = 4 * 1024 * 1024; // 4 MB buffer limit

template <typename Functor>
struct Wrapper
{
	Functor & functor;

	Wrapper(Functor & f) : functor(f) { }

	template <typename T>
	void operator() (T & object)
	{
		functor(object);
	}
};

struct VertexBuffer::BindVertexData
{
	VertexBuffer & ctx;
	std::vector<const VertexArray *> varrays[VertexFormat::LastFormat + 1];

	BindVertexData(VertexBuffer & vb) :
		ctx(vb)
	{
		// ctor
	}

	void operator() (Drawable & drawable)
	{
		Model * mo = drawable.GetModel();
		assert(mo);

		const VertexArray & va = mo->GetVertexArray();
		const VertexFormat::Enum vf = va.GetVertexFormat();
		const int vcount = va.GetNumVertices();
		const int icount = va.GetNumIndices();

		assert(vf < VertexFormat::LastFormat);

		// get object
		std::vector<Object> & obs = ctx.objects[vf];
		if (obs.empty() || (obs.back().vcount + vcount) * VertexFormat::Get(vf).stride > max_buffer_size)
		{
			obs.push_back(Object());
			assert(obs.size() <= 256);
		}
		Object & ob = obs.back();

		// set object buffers
		if (ob.vbuffer == 0)
		{
			glGenBuffers(1, &ob.ibuffer);
			glGenBuffers(1, &ob.vbuffer);
			if (ctx.use_vao)
				glGenVertexArrays(1, &ob.varray);
			ob.vformat = vf;
		}

		// check if model already in buffer
		Segment & sg = mo->GetVertexBufferSegment();
		if (sg.age != ctx.age)
		{
			// set model segment
			sg.ioffset = ob.icount * sizeof(int);
			sg.icount = icount;
			sg.voffset = ob.vcount;
			sg.vcount = vcount;
			sg.vbuffer = ctx.use_vao ? ob.varray : ob.vbuffer;
			sg.vformat = vf;
			sg.object = obs.size() - 1;
			sg.age = ctx.age;

			// store va for vertex data upload
			varrays[vf].push_back(&va);

			// update buffer counts
			ob.icount += icount;
			ob.vcount += vcount;
		}

		// set drawable segment
		drawable.SetVertexBufferSegment(sg);
	}
};

VertexBuffer::VertexBuffer() :
	age(1),
	use_vao(false)
{
	// ctor
}

VertexBuffer::Segment::Segment() :
	ioffset(0),
	icount(0),
	voffset(0),
	vcount(0),
	vbuffer(0),
	vformat(VertexFormat::LastFormat),
	object(0),
	age(0)
{
	// ctor
}

VertexBuffer::Object::Object() :
	icount(0),
	vcount(0),
	ibuffer(0),
	vbuffer(0),
	varray(0),
	vformat(VertexFormat::LastFormat)
{
	// ctor
}

VertexBuffer::~VertexBuffer()
{
	Clear();
}

void VertexBuffer::Clear()
{
	// reset buffer state
	if (use_vao)
		glBindVertexArray(0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	for (size_t n = 0; n <= VertexFormat::LastFormat; ++n)
	{
		for (size_t i = 0; i < objects[n].size(); ++i)
		{
			const Object & ob = objects[n][i];
			if (use_vao)
				glDeleteVertexArrays(1, &ob.varray);
			glDeleteBuffers(1, &ob.ibuffer);
			glDeleteBuffers(1, &ob.vbuffer);
		}
		objects[n].clear();
	}

	age += 2;
}

void VertexBuffer::Set(SceneNode * nodes[], unsigned int nodes_count)
{
	use_vao = glBindVertexArray ? true : false;

	Clear();

	BindVertexData data(*this);
	for (unsigned int i = 0; i < nodes_count; ++i)
	{
		assert(nodes[i]);
		nodes[i]->ApplyDrawableFunctor(Wrapper<BindVertexData>(data));
	}

	std::vector<float> vertex_buffer;
	std::vector<int> index_buffer;
	for (unsigned int i = 0; i <= VertexFormat::LastFormat; ++i)
	{
		UploadVertexData(objects[i], data.varrays[i], vertex_buffer, index_buffer, use_vao);
	}
}

void VertexBuffer::Draw(unsigned int & vobject, const Segment & s)
{
	assert(s.age == age);

	if (s.vbuffer != vobject)
	{
		// bind array / buffer
		if (use_vao)
		{
			glBindVertexArray(s.vbuffer);
			vobject = s.vbuffer;
			#ifdef VAO_BROKEN
				// broken vao implementations handling
				assert(s.vformat <= VertexAttrib::LastAttrib);
				assert(s.object < objects[s.vformat].size());
				const Object & ob = objects[s.vformat][s.object];
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ob.ibuffer);
			#endif
		}
		else
		{
			assert(s.vformat <= VertexAttrib::LastAttrib);
			assert(s.object < objects[s.vformat].size());
			const Object & ob = objects[s.vformat][s.object];

			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ob.ibuffer);
			glBindBuffer(GL_ARRAY_BUFFER, ob.vbuffer);
			SetVertexFormat(VertexFormat::Get(ob.vformat));
			vobject = ob.vbuffer;
		}
	}

	glDrawRangeElements(
		GL_TRIANGLES, s.voffset, s.voffset + s.vcount - 1,
		s.icount, GL_UNSIGNED_INT, (const void *)s.ioffset);
}

void VertexBuffer::UploadVertexData(
	const std::vector<Object> & objects,
	const std::vector<const VertexArray *> & varrays,
	std::vector<float> & vertex_buffer,
	std::vector<int> & index_buffer,
	const bool use_vao)
{
	size_t varray_index = 0;
	for (size_t i = 0; i < objects.size(); ++i)
	{
		const Object & ob = objects[i];
		const VertexFormat & vf = VertexFormat::Get(ob.vformat);
		const unsigned int vertex_size = vf.stride / sizeof(float);

		// fill staging buffers
		index_buffer.resize(ob.icount);
		vertex_buffer.resize(ob.vcount * vertex_size);
		unsigned int icount = 0;
		unsigned int vcount = 0;
		while (vcount < ob.vcount)
		{
			assert(varray_index < varrays.size());
			const VertexArray & va = *varrays[varray_index];
			icount = WriteIndices(va, icount, vcount, index_buffer);
			vcount = WriteVertices(va, vcount, vertex_size, vertex_buffer);
			varray_index++;
		}
		assert(icount == ob.icount);
		assert(vcount == ob.vcount);

		// upload buffers
		if (use_vao)
			glBindVertexArray(ob.varray);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ob.ibuffer);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, ob.icount * sizeof(int), &index_buffer[0], GL_STATIC_DRAW);

		glBindBuffer(GL_ARRAY_BUFFER, ob.vbuffer);
		glBufferData(GL_ARRAY_BUFFER, ob.vcount * vf.stride, &vertex_buffer[0], GL_STATIC_DRAW);

		SetVertexFormat(vf);

		// reset buffer state
		if (use_vao)
			glBindVertexArray(0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}
}

unsigned int VertexBuffer::WriteIndices(
	const VertexArray & va,
	const unsigned int icount,
	const unsigned int vcount,
	std::vector<int> & index_buffer)
{
	const int * faces = 0;
	int fn;
	va.GetFaces(faces, fn);

	assert(icount + fn <= index_buffer.size());
	int * ib = &index_buffer[icount];
	for (int j = 0; j < fn; ++j)
	{
		ib[j] = faces[j] + vcount;
	}

	return icount + fn;
}

unsigned int VertexBuffer::WriteVertices(
	const VertexArray & va,
	const unsigned int vcount,
	const unsigned int vertex_size,
	std::vector<float> & vertex_buffer)
{
	// get vertices (fixme: use VertexFormat info here)
	const int vat_count = 4;
	const void * vat_ptrs[4] = {0, 0, 0, 0};
	int vn, nn, tn, cn;
	va.GetVertices((const float *&)vat_ptrs[0], vn);
	va.GetNormals((const float *&)vat_ptrs[1], nn);
	va.GetTexCoords((const float *&)vat_ptrs[2], tn);
	va.GetColors((const unsigned char *&)vat_ptrs[3], cn);

	// calculate vertex element sizes and offsets in sizeof(float)
	int vat_sizes[4] = {3, 3, 2, 1};
	int vat_offsets[4] = {0, 3, 6, 8};
	for (int j = 0; j < vat_count; ++j)
	{
		if (vat_ptrs[j] == 0)
			vat_sizes[j] = 0;
	}
	for (int j = 1; j < vat_count; ++j)
	{
		vat_offsets[j] = vat_offsets[j - 1] + vat_sizes[j - 1];
	}

	// fill vertices
	assert((vcount + vn / 3) * vertex_size <= vertex_buffer.size());
	float * vb = &vertex_buffer[vcount * vertex_size];
	for (int j = 0; j < vn / 3 ; ++j)
	{
		float * v = vb + j * vertex_size;
		for (int k = 0; k < vat_count; ++k)
		{
			const float * vat = (const float *)vat_ptrs[k] + j * vat_sizes[k];
			for (int m = 0; m < vat_sizes[k]; ++m)
			{
				v[vat_offsets[k] + m] = vat[m];
			}
		}
	}

	return vcount + vn / 3;
}

void VertexBuffer::SetVertexFormat(const VertexFormat & vf)
{
	for (unsigned int n = 0; n < vf.attribs_count; ++n)
	{
		const VertexAttrib::Format & af = vf.attribs[n];
		glEnableVertexAttribArray(af.index);
		glVertexAttribPointer(
			af.index, af.size, af.type, af.norm,
			vf.stride, (const void *)af.offset);
	}
	// TODO: Cache enabled / disabled attributes?
	for (unsigned int n = vf.attribs_count; n <= VertexAttrib::LastAttrib; ++n)
	{
		glDisableVertexAttribArray(vf.attribs[n].index);
	}
}
