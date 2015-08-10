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

static const unsigned int max_buffer_size = 4 * 1024 * 1024;
static const unsigned int min_dynamic_vertex_buffer_size = 64 * 1024;
static const unsigned int min_dynamic_index_buffer_size = 4 * 1024;

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

// Assuming dynamic vertex data amount is small (~64 KB), write it directly
// into staging buffers, while deferring gpu upload to a separate pass.
struct VertexBuffer::BindDynamicVertexData
{
	VertexBuffer & ctx;

	BindDynamicVertexData(VertexBuffer & vb) :
		ctx(vb)
	{
		// ctor
	}

	void operator() (Drawable & drawable)
	{
		if (!drawable.GetDrawEnable())
			return;

		assert(drawable.GetVertArray());
		const VertexArray & va = *drawable.GetVertArray();
		const VertexFormat::Enum vf = va.GetVertexFormat();
		const unsigned int vsize = VertexFormat::Get(vf).stride / sizeof(float);
		const unsigned int vcount = va.GetNumVertices();
		const unsigned int icount = va.GetNumIndices();

		// FIXME: text drawables can contain empty vertex arrays,
		// they should be culled before getting here
		if (vcount == 0)
		{
			// reset segment as we might miss text drawable vcount change
			// happens with Tracks/Cars scroll onfocus tooltip update
			// need to investigate this
			if (drawable.GetVertexBufferSegment().age)
				drawable.SetVertexBufferSegment(Segment());
			return;
		}

		// get dynamic vertex data object (first object in the vector)
		const unsigned int obindex = 0;
		assert(!ctx.objects[vf].empty());
		Object & ob = ctx.objects[vf][obindex];

		// gen object buffers
		if (ob.vbuffer == 0)
		{
			glGenBuffers(1, &ob.ibuffer);
			glGenBuffers(1, &ob.vbuffer);
			if (ctx.use_vao)
			{
				glGenVertexArrays(1, &ob.varray);
				if (ob.varray == 0)
					ctx.use_vao = ctx.good_vao = false;
			}
			ob.vformat = vf;
		}

		// set segment
		Segment sg;
		sg.ioffset = ob.icount * sizeof(unsigned int);
		sg.icount = icount;
		sg.voffset = ob.vcount;
		sg.vcount = vcount;
		sg.vbuffer = ob.varray ? ob.varray : ob.vbuffer;
		sg.vformat = vf;
		sg.object = obindex;
		sg.age = ctx.age_dynamic;
		drawable.SetVertexBufferSegment(sg);

		// upload data into staging buffers
		std::vector<unsigned int> & index_buffer = ctx.staging_index_buffer[vf];
		std::vector<float> & vertex_buffer = ctx.staging_vertex_buffer[vf];
		const unsigned int ibn = (ob.icount + icount);
		const unsigned int vbn = (ob.vcount + vcount) * vsize;
		if (index_buffer.size() < ibn)
		{
			const unsigned int ibmin = min_dynamic_index_buffer_size / sizeof(unsigned int);
			index_buffer.resize(std::max(ibn, ibmin));
		}
		if (vertex_buffer.size() < vbn)
		{
			const unsigned int vbmin = min_dynamic_vertex_buffer_size / sizeof(float);
			vertex_buffer.resize(std::max(vbn, vbmin));
		}
		ob.icount = WriteIndices(va, ob.icount, ob.vcount, index_buffer);
		ob.vcount = WriteVertices(va, ob.vcount, vsize, vertex_buffer);
	}
};

// The idea is to initialize drawable vertex buffer segments and allocate
// buffer objects, while deferring vertex data upload to a later separate pass,
// to avoid staging buffers reallocations (up to 4 MB).
struct VertexBuffer::BindStaticVertexData
{
	VertexBuffer & ctx;
	std::vector<const VertexArray *> varrays[VertexFormat::LastFormat + 1];

	BindStaticVertexData(VertexBuffer & vb) :
		ctx(vb)
	{
		// ctor
	}

	void operator() (Drawable & drawable)
	{
		Model * mo = drawable.GetModel();
		assert(mo);

		// early out if model already bound
		Segment & sg = mo->GetVertexBufferSegment();
		if (sg.age == ctx.age_static)
		{
			drawable.SetVertexBufferSegment(sg);
			return;
		}

		const VertexArray & va = mo->GetVertexArray();
		const VertexFormat::Enum vf = va.GetVertexFormat();
		const unsigned int vsize = VertexFormat::Get(vf).stride;
		const unsigned int vcount = va.GetNumVertices();
		const unsigned int icount = va.GetNumIndices();
		assert(vcount > 0);

		// get object (first object is reserved for dynamic vertex data)
		std::vector<Object> & obs = ctx.objects[vf];
		if (obs.size() < 2 || (obs.back().vcount + vcount) * vsize > max_buffer_size)
		{
			obs.push_back(Object());
			assert(obs.size() <= 256);
		}
		const unsigned int obindex = obs.size() - 1;
		Object & ob = obs[obindex];

		// gen object buffers
		if (ob.vbuffer == 0)
		{
			glGenBuffers(1, &ob.ibuffer);
			glGenBuffers(1, &ob.vbuffer);
			if (ctx.use_vao)
			{
				glGenVertexArrays(1, &ob.varray);
				if (ob.varray == 0)
					ctx.use_vao = ctx.good_vao = false;
			}
			ob.vformat = vf;
		}

		// set segment
		sg.ioffset = ob.icount * sizeof(unsigned int);
		sg.icount = icount;
		sg.voffset = ob.vcount;
		sg.vcount = vcount;
		sg.vbuffer = ob.varray ? ob.varray : ob.vbuffer;
		sg.vformat = vf;
		sg.object = obindex;
		sg.age = ctx.age_static;
		drawable.SetVertexBufferSegment(sg);

		// store va for vertex data upload and update buffer counts
		varrays[vf].push_back(&va);
		ob.icount += icount;
		ob.vcount += vcount;
	}
};

VertexBuffer::VertexBuffer() :
	age_dynamic(1),
	age_static(1),
	use_vao(false),
	good_vao(true),
	bind_ibo(false)
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
	icapacity(0),
	vcapacity(0),
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

void VertexBuffer::BindElementBufferExplicitly()
{
	bind_ibo = true;
}

void VertexBuffer::Clear()
{
	// reset buffer state
	if (GLC_ARB_vertex_array_object)
		glBindVertexArray(0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	for (unsigned int n = 0; n <= VertexFormat::LastFormat; ++n)
	{
		for (unsigned int i = 0; i < objects[n].size(); ++i)
		{
			const Object & ob = objects[n][i];
			if (ob.varray)
				glDeleteVertexArrays(1, &ob.varray);
			glDeleteBuffers(1, &ob.ibuffer);
			glDeleteBuffers(1, &ob.vbuffer);
		}
		objects[n].clear();
	}
}

void VertexBuffer::SetDynamicVertexData(SceneNode * nodes[], unsigned int count)
{
	use_vao = GLC_ARB_vertex_array_object && good_vao;

	age_dynamic += 2;

	InitDynamicBufferObjects();

	BindDynamicVertexData bind_data(*this);
	for (unsigned int i = 0; i < count; ++i)
	{
		assert(nodes[i]);
		nodes[i]->ApplyDrawableFunctor(Wrapper<BindDynamicVertexData>(bind_data));
	}

	for (unsigned int i = 0; i <= VertexFormat::LastFormat; ++i)
	{
		UploadDynamicVertexData(objects[i], staging_index_buffer[i], staging_vertex_buffer[i]);
	}
}

void VertexBuffer::SetStaticVertexData(SceneNode * nodes[], unsigned int count)
{
	use_vao = GLC_ARB_vertex_array_object && good_vao;

	age_static += 2;

	Clear();

	// make sure dynamic buffer objects are allocated
	InitDynamicBufferObjects();

	BindStaticVertexData bind_data(*this);
	for (unsigned int i = 0; i < count; ++i)
	{
		assert(nodes[i]);
		nodes[i]->ApplyDrawableFunctor(Wrapper<BindStaticVertexData>(bind_data));
	}

	std::vector<unsigned int> index_buffer;
	std::vector<float> vertex_buffer;
	for (unsigned int i = 0; i <= VertexFormat::LastFormat; ++i)
	{
		UploadStaticVertexData(objects[i], bind_data.varrays[i], index_buffer, vertex_buffer);
	}
}

void VertexBuffer::Draw(unsigned int & vbuffer, const Segment & s) const
{
	// FIXME: text drawables can contain empty vertex arrays,
	// they should be culled before getting here
	if (s.vcount == 0)
		return;

	const unsigned short age = (s.object != 0) ? age_static : age_dynamic;
	if (s.age != age)
	{
		assert(0);
		return;
	}

	if (vbuffer != s.vbuffer)
		BindSegmentBuffer(vbuffer, s);

	// don't attempt to draw with no buffer bound
	if (vbuffer == 0)
	{
		assert(0);
		return;
	}

	if (s.icount != 0)
	{
		glDrawRangeElements(
			GL_TRIANGLES, s.voffset, s.voffset + s.vcount - 1, s.icount,
			GL_UNSIGNED_INT, (const void *)(size_t)s.ioffset);
	}
	else
	{
		glDrawArrays(GL_LINES, s.voffset, s.vcount);
	}
}

void VertexBuffer::BindSegmentBuffer(unsigned int & vbuffer, const Segment & s) const
{
	if (use_vao)
	{
		glBindVertexArray(s.vbuffer);
		#ifdef _WIN32
		if (bind_ibo)
		{
			assert(s.vformat <= VertexAttrib::LastAttrib);
			assert(s.object < objects[s.vformat].size());
			const Object & ob = objects[s.vformat][s.object];
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ob.ibuffer);
		}
		#endif
		vbuffer = s.vbuffer;
	}
	else
	{
		assert(s.vformat <= VertexAttrib::LastAttrib);
		assert(s.object < objects[s.vformat].size());
		const Object & ob = objects[s.vformat][s.object];

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ob.ibuffer);
		glBindBuffer(GL_ARRAY_BUFFER, ob.vbuffer);
		SetVertexFormat(VertexFormat::Get(ob.vformat));
		vbuffer = ob.vbuffer;
	}
}

void VertexBuffer::InitDynamicBufferObjects()
{
	for (unsigned int i = 0; i <= VertexFormat::LastFormat; ++i)
	{
		std::vector<Object> & obs = objects[i];
		if (obs.empty())
		{
			Object ob;
			ob.icapacity = min_dynamic_index_buffer_size;
			ob.vcapacity = min_dynamic_vertex_buffer_size;
			obs.push_back(ob);
		}
		obs[0].icount = 0;
		obs[0].vcount = 0;
	}
}

void VertexBuffer::UploadDynamicVertexData(
	std::vector<Object> & objects,
	const std::vector<unsigned int> & index_buffer,
	const std::vector<float> & vertex_buffer)
{
	assert(!objects.empty());
	Object & ob = objects[0];
	if (ob.vcount)
	{
		UploadBuffers(ob, index_buffer, vertex_buffer);
	}
}

void VertexBuffer::UploadStaticVertexData(
	std::vector<Object> & objects,
	const std::vector<const VertexArray *> & varrays,
	std::vector<unsigned int> & index_buffer,
	std::vector<float> & vertex_buffer)
{
	unsigned int varray_index = 0;
	for (unsigned int i = 1; i < objects.size(); ++i)
	{
		Object & ob = objects[i];
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

		UploadBuffers(ob, index_buffer, vertex_buffer);
	}
}

unsigned int VertexBuffer::WriteIndices(
	const VertexArray & va,
	const unsigned int icount,
	const unsigned int vcount,
	std::vector<unsigned int> & index_buffer)
{
	const unsigned int * faces = 0;
	int fn;
	va.GetFaces(faces, fn);

	assert(icount + fn <= index_buffer.size());
	unsigned int * ib = &index_buffer[icount];
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

void VertexBuffer::UploadBuffers(
	Object & object,
	const std::vector<unsigned int> & index_buffer,
	const std::vector<float> & vertex_buffer)
{
	const VertexFormat & vformat = VertexFormat::Get(object.vformat);
	const unsigned int icapacity = object.icount * sizeof(unsigned int);
	const unsigned int vcapacity = object.vcount * vformat.stride;

	if (object.varray)
		glBindVertexArray(object.varray);

	if (object.ibuffer)
	{
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, object.ibuffer);
		if (object.icapacity > icapacity)
		{
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, object.icapacity, NULL, GL_STATIC_DRAW);
			glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, icapacity, &index_buffer[0]);
		}
		else
		{
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, icapacity, &index_buffer[0], GL_STATIC_DRAW);
			object.icapacity = icapacity;
		}
	}

	assert(object.vbuffer);
	glBindBuffer(GL_ARRAY_BUFFER, object.vbuffer);
	if (object.vcapacity > vcapacity)
	{
		glBufferData(GL_ARRAY_BUFFER, object.vcapacity, NULL, GL_STATIC_DRAW);
		glBufferSubData(GL_ARRAY_BUFFER, 0, vcapacity, &vertex_buffer[0]);
	}
	else
	{
		glBufferData(GL_ARRAY_BUFFER, vcapacity, &vertex_buffer[0], GL_STATIC_DRAW);
		object.vcapacity = vcapacity;
	}

	SetVertexFormat(vformat);

	// reset buffer state
	if (object.varray)
		glBindVertexArray(0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void VertexBuffer::SetVertexFormat(const VertexFormat & vf)
{
	for (unsigned int n = 0; n < vf.attribs_count; ++n)
	{
		const VertexAttrib::Format & af = vf.attribs[n];
		glEnableVertexAttribArray(af.index);
		glVertexAttribPointer(
			af.index, af.size, af.type, af.norm,
			vf.stride, (const void *)(size_t)af.offset);
	}
	// TODO: Cache enabled / disabled attributes?
	for (unsigned int n = vf.attribs_count; n <= VertexAttrib::LastAttrib; ++n)
	{
		glDisableVertexAttribArray(vf.attribs[n].index);
	}
}
