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

#include "vertexarray.h"
#include "quaternion.h"
#include "unittest.h"

VertexArray::VertexArray() :
	format(VertexFormat::P3)
{
	// ctor
}

VertexArray::~VertexArray()
{
	Clear();
}

void VertexArray::Clear()
{
	colors.clear();
	texcoords.clear();
	normals.clear();
	vertices.clear();
	faces.clear();
}

#define COMBINEVECTORS(vname) {out.vname.reserve(vname.size() + v.vname.size());out.vname.insert(out.vname.end(), vname.begin(), vname.end());out.vname.insert(out.vname.end(), v.vname.begin(), v.vname.end());}

VertexArray VertexArray::operator+ (const VertexArray & v) const
{
	VertexArray out;

	COMBINEVECTORS(colors);

	COMBINEVECTORS(texcoords);

	COMBINEVECTORS(normals);

	COMBINEVECTORS(vertices);

	const int offset = vertices.size() / 3;
	out.faces.reserve(faces.size() + v.faces.size());
	out.faces.insert(out.faces.end(), faces.begin(), faces.end());
	for (size_t i = 0; i < v.faces.size(); i++)
	{
		out.faces.push_back(v.faces[i] + offset);
	}

	assert(format == v.format);
	out.format = format;

	return out;
}

void VertexArray::GetColors(const unsigned char * & output_array_pointer, int & output_array_num) const
{
	output_array_num = colors.size();
	output_array_pointer = colors.empty() ? NULL : &colors[0];
}

void VertexArray::GetTexCoords(const float * & output_array_pointer, int & output_array_num) const
{
	output_array_num = texcoords.size();
	output_array_pointer = texcoords.empty() ? NULL : &texcoords[0];
}

void VertexArray::GetNormals(const float * & output_array_pointer, int & output_array_num) const
{
	output_array_num = normals.size();
	output_array_pointer = normals.empty() ? NULL : &normals[0];
}

void VertexArray::GetVertices(const float * & output_array_pointer, int & output_array_num) const
{
	output_array_num = vertices.size();
	output_array_pointer = vertices.empty() ? NULL : &vertices[0];
}

void VertexArray::GetFaces(const unsigned int * & output_array_pointer, int & output_array_num) const
{
	output_array_num = faces.size();
	output_array_pointer = faces.empty() ? NULL : &faces[0];
}

void VertexArray::SetColors(const unsigned char array[], size_t count, size_t offset)
{
	size_t size = offset + count;

	// Tried to assign values that aren't in sets of 4
	assert(size % 4 == 0);

	if (size != colors.size())
		colors.resize(size);

	unsigned char * myarray = &(colors[offset]);
	for (size_t i = 0; i < count; ++i)
	{
		myarray[i] = array[i];
	}
}

void VertexArray::SetTexCoords(const float array[], size_t count, size_t offset)
{
	// Tried to assign values that aren't in sets of 2
	assert(count % 2 == 0);

	size_t size = offset + count;

	if (size != texcoords.size())
		texcoords.resize(size);

	float * myarray = &(texcoords[offset]);
	for (size_t i = 0; i < count; ++i)
	{
		myarray[i] = array[i];
	}
}

void VertexArray::SetNormals(const float array[], size_t count, size_t offset)
{
	size_t size = offset + count;

	// Tried to assign values that aren't in sets of 3
	assert(size % 3 == 0);

	if (size != normals.size())
		normals.resize(size);

	float * myarray = &(normals[offset]);
	for (size_t i = 0; i < count; ++i)
	{
		myarray[i] = array[i];
	}
}

void VertexArray::SetVertices(const float array[], size_t count, size_t offset)
{
	size_t size = offset + count;

	// Tried to assign values that aren't in sets of 3
	assert(size % 3 == 0);

	if (size != vertices.size())
		vertices.resize(size);

	float * myarray = &(vertices[offset]);
	for (size_t i = 0; i < count; ++i)
	{
		myarray[i] = array[i];
	}
}

void VertexArray::SetFaces(const unsigned int array[], size_t count, size_t offset, size_t idoffset)
{
	// Tried to assign values that aren't in sets of 3
	assert (count % 3 == 0);

	size_t size = offset + count;

	if (size != faces.size())
		faces.resize(size);

	unsigned int * myarray = &(faces[offset]);
	for (size_t i = 0; i < count; ++i)
	{
		myarray[i] = array[i] + idoffset;
	}
}

void VertexArray::Add(
	const unsigned int newfaces[], int newfacecount,
	const float newvert[], int newvertcount,
	const float newtco[], int newtcocount,
	const float newnorm[], int newnormcount ,
	const unsigned char newcol[], int newcolcount)
{
	SetFaces(newfaces, newfacecount, faces.size(), vertices.size() / 3);
	SetVertices(newvert, newvertcount, vertices.size());
	SetNormals(newnorm, newnormcount, normals.size());
	SetTexCoords(newtco, newtcocount, texcoords.size());
	SetColors(newcol, newcolcount, colors.size());

	assert(!vertices.empty());
	format = VertexFormat::P3;
	if (!texcoords.empty())
	{
		if (!normals.empty())
			format = VertexFormat::PNT332;
		else
			format = colors.empty() ? VertexFormat::PT32 : VertexFormat::PTC324;
	}
}

void VertexArray::SetToBillboard(float x1, float y1, float x2, float y2)
{
	unsigned int bfaces[6];
	bfaces[0] = 0;
	bfaces[1] = 1;
	bfaces[2] = 2;
	bfaces[3] = 0;
	bfaces[4] = 2;
	bfaces[5] = 3;
	SetFaces(bfaces, 6);
/*
	float normals[12];
	for (unsigned int i = 0; i < 12; i+=3)
	{
		normals[i] = 0;
		normals[i+1] = 0;
		normals[i+2] = 1;
	}
	SetNormals(normals, 12);
*/
	//build this:
	//x1, y1, 0
	//x2, y1, 0
	//x2, y2, 0
	//x1, y2, 0
	float verts[12];
	verts[2] = verts[5] = verts[8] = verts[11] = 0.0;
	verts[0] = verts[9] = x1;
	verts[3] = verts[6] = x2;
	verts[1] = verts[4] = y1;
	verts[7] = verts[10] = y2;
	SetVertices(verts, 12);

	float tc[8];
	tc[0] = tc[1] = tc[3] = tc[6] = 0.0;
	tc[2] = tc[4] = tc[5] = tc[7] = 1.0;
	SetTexCoords(tc, 8);

	format = VertexFormat::PT32;
}

void VertexArray::SetTo2DQuad(float x1, float y1, float x2, float y2, float u1, float v1, float u2, float v2, float z)
{
	float vcorners[12];
	float uvs[8];
	unsigned int bfaces[6];
	SetVertexData2DQuad(x1,y1,x2,y2,u1,v1,u2,v2, vcorners, uvs, bfaces);
	for (unsigned int i = 2; i < 12; i += 3)
		vcorners[i] = z;

	SetFaces(bfaces, 6);
	SetVertices(vcorners, 12);
	SetTexCoords(uvs, 8);

	format = VertexFormat::PT32;
}

void VertexArray::SetVertexData2DQuad(float x1, float y1, float x2, float y2, float u1, float v1, float u2, float v2, float * vcorners, float * uvs, unsigned int * bfaces, unsigned int faceoffset) const
{
	vcorners[0] = x1;
	vcorners[1] = y1;
	vcorners[2] = 0;
	vcorners[3] = x2;
	vcorners[4] = y1;
	vcorners[5] = 0;
	vcorners[6] = x2;
	vcorners[7] = y2;
	vcorners[8] = 0;
	vcorners[9] = x1;
	vcorners[10] = y2;
	vcorners[11] = 0;

	uvs[0] = u1;
	uvs[1] = v1;
	uvs[2] = u2;
	uvs[3] = v1;
	uvs[4] = u2;
	uvs[5] = v2;
	uvs[6] = u1;
	uvs[7] = v2;

	bfaces[0] = faceoffset+0;
	bfaces[1] = faceoffset+2;
	bfaces[2] = faceoffset+1;
	bfaces[3] = faceoffset+0;
	bfaces[4] = faceoffset+3;
	bfaces[5] = faceoffset+2;
}

void VertexArray::SetTo2DButton(float x, float y, float w, float h, float sidewidth, bool flip)
{
	float vcorners[12*3];
	float uvs[8*3];
	unsigned int bfaces[6*3];

	//y1 = 1.0 - y1;
	//y2 = 1.0 - y2;

	Vec2 corner1;
	Vec2 corner2;
	Vec2 dim;
	dim.Set(w,h);
	Vec2 center;
	center.Set(x,y);
	corner1 = center - dim*0.5;
	corner2 = center + dim*0.5;

	float x1 = corner1[0];
	float y1 = corner1[1];
	float x2 = corner2[0];
	float y2 = corner2[1];

	if (flip)
	{
		float y3 = y1;
		y1 = y2;
		y2 = y3;
	}

	//left
	SetVertexData2DQuad(x1-sidewidth,y1,x1,y2, 0,0,0.5,1, vcorners, uvs, bfaces);

	//center
	SetVertexData2DQuad(x1,y1,x2,y2, 0.5,0,0.5,1, &(vcorners[12]), &(uvs[8]), &(bfaces[6]), 4);

	//right
	SetVertexData2DQuad(x2,y1,x2+sidewidth,y2, 0.5,0,1,1, &(vcorners[12*2]), &(uvs[8*2]), &(bfaces[6*2]), 8);

	SetFaces(bfaces, 6*3);
	SetVertices(vcorners, 12*3);
	SetTexCoords(uvs, 8*3);

	format = VertexFormat::PT32;
}

void VertexArray::SetTo2DBox(float x, float y, float w, float h, float marginwidth, float marginheight, float clipx)
{
	const unsigned int quads = 9;
	float vcorners[12*quads];
	float uvs[8*quads];
	unsigned int bfaces[6*quads];

	//y1 = 1.0 - y1;
	//y2 = 1.0 - y2;

	Vec2 corner1;
	Vec2 corner2;
	Vec2 dim;
	dim.Set(w,h);
	Vec2 center;
	center.Set(x,y);
	corner1 = center - dim*0.5;
	corner2 = center + dim*0.5;
	Vec2 margin;
	margin.Set(marginwidth, marginheight);

	float lxmax = std::max((corner1-margin)[0],std::min(clipx,corner1[0]));
	float cxmax = std::max(corner1[0],std::min(clipx,corner2[0]));
	float rxmax = std::max(corner2[0],std::min(clipx,(corner2+margin)[0]));
	float lumax = (lxmax-(corner1-margin)[0])/(corner1[0]-(corner1-margin)[0])*0.5;
	float rumax = (rxmax-corner2[0])/((corner2+margin)[0]-corner2[0])*0.5+0.5;

	//upper left
	SetVertexData2DQuad((corner1-margin)[0],(corner1-margin)[1],lxmax,corner1[1],
			    0,0,lumax,0.5, vcorners,uvs,bfaces);

	//upper center
	SetVertexData2DQuad(corner1[0],(corner1-margin)[1],cxmax,corner1[1],
			     0.5,0,0.5,0.5,
			     &(vcorners[12*1]),&(uvs[8*1]),&(bfaces[6*1]),4*1);

	//upper right
	SetVertexData2DQuad(corner2[0],(corner1-margin)[1],rxmax,corner1[1],
			    0.5,0,rumax,0.5,
			    &(vcorners[12*2]),&(uvs[8*2]),&(bfaces[6*2]),4*2);

	//center left
	SetVertexData2DQuad((corner1-margin)[0],corner1[1],lxmax,corner2[1],
			    0,0.5,lumax,0.5,
			    &(vcorners[12*3]),&(uvs[8*3]),&(bfaces[6*3]),4*3);

	//center center
	SetVertexData2DQuad(corner1[0],corner1[1],cxmax,corner2[1],
			    0.5,0.5,0.5,0.5,
			    &(vcorners[12*4]),&(uvs[8*4]),&(bfaces[6*4]),4*4);

	//center right
	SetVertexData2DQuad(corner2[0],corner1[1],rxmax,corner2[1],
			    0.5,0.5,rumax,0.5,
			    &(vcorners[12*5]),&(uvs[8*5]),&(bfaces[6*5]),4*5);

	//lower left
	SetVertexData2DQuad((corner1-margin)[0],corner2[1],lxmax,(corner2+margin)[1],
			    0,0.5,lumax,1,
			    &(vcorners[12*6]),&(uvs[8*6]),&(bfaces[6*6]),4*6);

	//lower center
	SetVertexData2DQuad(corner1[0],corner2[1],cxmax,(corner2+margin)[1],
			    0.5,0.5,0.5,1,
			    &(vcorners[12*7]),&(uvs[8*7]),&(bfaces[6*7]),4*7);

	//lower right
	SetVertexData2DQuad(corner2[0],corner2[1],rxmax,(corner2+margin)[1],
			    0.5,0.5,rumax,1,
			    &(vcorners[12*8]),&(uvs[8*8]),&(bfaces[6*8]),4*8);

	SetFaces(bfaces, 6*quads);
	SetVertices(vcorners, 12*quads);
	SetTexCoords(uvs, 8*quads);

	format = VertexFormat::PT32;
}

void VertexArray::SetToUnitCube()
{
	std::vector <VertexArray::Float3> verts;
	verts.push_back(VertexArray::Float3(0,0,0));
	verts.push_back(VertexArray::Float3(0.5,-0.5,-0.5)); //1
	verts.push_back(VertexArray::Float3(0.5,-0.5,0.5));
	verts.push_back(VertexArray::Float3(-0.5,-0.5,0.5));
	verts.push_back(VertexArray::Float3(-0.5,-0.5,-0.5)); //4
	verts.push_back(VertexArray::Float3(0.5,0.5,-0.5)); //5
	verts.push_back(VertexArray::Float3(0.5,0.5,0.5));
	verts.push_back(VertexArray::Float3(-0.5,0.5,0.5));
	verts.push_back(VertexArray::Float3(-0.5,0.5,-0.5)); //8

	std::vector <VertexArray::Float3> norms;
	norms.push_back(VertexArray::Float3(0,0,0));
	norms.push_back(VertexArray::Float3(0.0,0.0,-1.0));
	norms.push_back(VertexArray::Float3(-1.0,-0.0,-0.0));
	norms.push_back(VertexArray::Float3(-0.0,-0.0,1.0));
	norms.push_back(VertexArray::Float3(-0.0,0.0,1.0));
	norms.push_back(VertexArray::Float3(1.0,-0.0,0.0));
	norms.push_back(VertexArray::Float3(1.0,0.0,0.0));
	norms.push_back(VertexArray::Float3(0.0,1.0,-0.0));
	norms.push_back(VertexArray::Float3(-0.0,-1.0,0.0));

	std::vector <VertexArray::Float2> texcoords(1);
	texcoords[0].u = 0;
	texcoords[0].v = 0;

	std::vector <VertexArray::Face> cubesides;
	cubesides.push_back(VertexArray::Face(VertexArray::VertexData(verts[5],norms[1],texcoords[0]),VertexArray::VertexData(verts[1],norms[1],texcoords[0]),VertexArray::VertexData(verts[8],norms[1],texcoords[0])));
	cubesides.push_back(VertexArray::Face(VertexArray::VertexData(verts[1],norms[1],texcoords[0]),VertexArray::VertexData(verts[4],norms[1],texcoords[0]),VertexArray::VertexData(verts[8],norms[1],texcoords[0])));
	cubesides.push_back(VertexArray::Face(VertexArray::VertexData(verts[3],norms[2],texcoords[0]),VertexArray::VertexData(verts[7],norms[2],texcoords[0]),VertexArray::VertexData(verts[8],norms[2],texcoords[0])));
	cubesides.push_back(VertexArray::Face(VertexArray::VertexData(verts[3],norms[2],texcoords[0]),VertexArray::VertexData(verts[8],norms[2],texcoords[0]),VertexArray::VertexData(verts[4],norms[2],texcoords[0])));
	cubesides.push_back(VertexArray::Face(VertexArray::VertexData(verts[2],norms[3],texcoords[0]),VertexArray::VertexData(verts[6],norms[3],texcoords[0]),VertexArray::VertexData(verts[3],norms[3],texcoords[0])));
	cubesides.push_back(VertexArray::Face(VertexArray::VertexData(verts[6],norms[4],texcoords[0]),VertexArray::VertexData(verts[7],norms[4],texcoords[0]),VertexArray::VertexData(verts[3],norms[4],texcoords[0])));
	cubesides.push_back(VertexArray::Face(VertexArray::VertexData(verts[1],norms[5],texcoords[0]),VertexArray::VertexData(verts[5],norms[5],texcoords[0]),VertexArray::VertexData(verts[2],norms[5],texcoords[0])));
	cubesides.push_back(VertexArray::Face(VertexArray::VertexData(verts[5],norms[6],texcoords[0]),VertexArray::VertexData(verts[6],norms[6],texcoords[0]),VertexArray::VertexData(verts[2],norms[6],texcoords[0])));
	cubesides.push_back(VertexArray::Face(VertexArray::VertexData(verts[5],norms[7],texcoords[0]),VertexArray::VertexData(verts[8],norms[7],texcoords[0]),VertexArray::VertexData(verts[7],norms[7],texcoords[0])));
	cubesides.push_back(VertexArray::Face(VertexArray::VertexData(verts[5],norms[7],texcoords[0]),VertexArray::VertexData(verts[7],norms[7],texcoords[0]),VertexArray::VertexData(verts[6],norms[7],texcoords[0])));
	cubesides.push_back(VertexArray::Face(VertexArray::VertexData(verts[1],norms[8],texcoords[0]),VertexArray::VertexData(verts[2],norms[8],texcoords[0]),VertexArray::VertexData(verts[3],norms[8],texcoords[0])));
	cubesides.push_back(VertexArray::Face(VertexArray::VertexData(verts[1],norms[8],texcoords[0]),VertexArray::VertexData(verts[3],norms[8],texcoords[0]),VertexArray::VertexData(verts[4],norms[8],texcoords[0])));

	BuildFromFaces(cubesides);
}

void VertexArray::SetTo2DRing(float r0, float r1, float a0, float a1, unsigned n)
{
	assert(n > 0);

	format = VertexFormat::PT32;
	texcoords.resize((n + 1) * 4);
	vertices.resize((n + 1) * 6);
	faces.resize(n * 6);

	float du = 1.0f / n;
	float da = (a1 - a0) * du;

	float u = 0.0f;
	float a = a0;
	for (unsigned i = 0, t = 0; i <= n * 6; i += 6, t += 4)
	{
		float cosa = cosf(a);
		float sina = sinf(a);

		vertices[i + 0] = cosa * r0;
		vertices[i + 1] = sina * r0;
		vertices[i + 2] = 0;
		vertices[i + 3] = cosa * r1;
		vertices[i + 4] = sina * r1;
		vertices[i + 5] = 0;

		texcoords[t + 0] = u;
		texcoords[t + 1] = 0;
		texcoords[t + 2] = u;
		texcoords[t + 3] = 1;

		a += da;
		u += du;
	}

	for (unsigned i = 0, f = 0; i < n * 6; i += 6, f += 2)
	{
		faces[i + 0] = f + 0;
		faces[i + 1] = f + 1;
		faces[i + 2] = f + 2;
		faces[i + 3] = f + 1;
		faces[i + 4] = f + 3;
		faces[i + 5] = f + 2;
	}
}

void VertexArray::BuildFromFaces(const std::vector <Face> & newfaces)
{
	Clear();

	std::map <VertexData, unsigned int> indexmap;
	for (std::vector <Face>::const_iterator i = newfaces.begin(); i != newfaces.end(); ++i) //loop through input triangles
	{
		for (int n = 0; n < 3; n++) //loop through vertices in triangle
		{
			const VertexData & curvertdata = i->v[n]; //grab vertex
			std::map <VertexData, unsigned int>::iterator result = indexmap.find(curvertdata);
			if (result == indexmap.end()) //new vertex
			{
				unsigned int newidx = indexmap.size();
				indexmap[curvertdata] = newidx;

				vertices.push_back(curvertdata.vertex.x);
				vertices.push_back(curvertdata.vertex.y);
				vertices.push_back(curvertdata.vertex.z);

				normals.push_back(curvertdata.normal.x);
				normals.push_back(curvertdata.normal.y);
				normals.push_back(curvertdata.normal.z);

				texcoords.push_back(curvertdata.texcoord.u);
				texcoords.push_back(curvertdata.texcoord.v);

				faces.push_back(newidx);
			}
			else //non-unique vertex
				faces.push_back(result->second);
		}
	}

	format = VertexFormat::PNT332;

	assert(faces.size()/3 == newfaces.size());
	assert(vertices.size()/3 == normals.size()/3 && normals.size()/3 == texcoords.size()/2);
	assert(vertices.size()/3 <= faces.size());
}

void VertexArray::Translate(float x, float y, float z)
{
	assert(vertices.size() % 3 == 0);
	for (std::vector <float>::iterator i = vertices.begin(); i != vertices.end(); i += 3)
	{
		float * vert = &*i;
		vert[0] += x;
		vert[1] += y;
		vert[2] += z;
	}
}

void VertexArray::Rotate(float a, float x, float y, float z)
{
	Quat q;
	q.SetAxisAngle(a, x, y, z);

	assert(vertices.size() % 3 == 0);
	for (std::vector <float>::iterator i = vertices.begin(); i != vertices.end(); i += 3)
	{
		float * vert = &*i;
		q.RotateVector(vert);
	}

	assert(normals.size() % 3 == 0);
	for (std::vector <float>::iterator i = normals.begin(); i != normals.end(); i += 3)
	{
		float * n = &*i;
		q.RotateVector(n);
	}
}

void VertexArray::Scale(float x, float y, float z)
{
	assert(vertices.size() % 3 == 0);
	for (std::vector <float>::iterator i = vertices.begin(), e = vertices.end(); i != e; i += 3)
	{
		float * vert = &*i;
		vert[0] *= x;
		vert[1] *= y;
		vert[2] *= z;
	}

	assert(normals.size() % 3 == 0);
	for (std::vector <float>::iterator i = normals.begin(), e = normals.end(); i != e; i += 3)
	{
		float * n = &*i;
		n[0] *= x;
		n[1] *= y;
		n[2] *= z;
		float len2 = n[0] * n[0] + n[1] * n[1] + n[2] * n[2];
		if (len2 > 0.0)
		{
			float len = 1 / sqrtf(len2);
			n[0] *= len;
			n[1] *= len;
			n[2] *= len;
		}
	}

	if (x < 0.0f || y < 0.0f || z < 0.0f)
	{
		FixWindingOrder();
	}
}

void VertexArray::FlipNormals()
{
	assert(normals.size() % 3 == 0);
	for (std::vector <float>::iterator i = normals.begin(); i != normals.end(); i++)
	{
		*i = -*i;
	}
}

void VertexArray::FlipWindingOrder()
{
	assert(faces.size() % 3 == 0);
	for (std::vector <unsigned int>::iterator i = faces.begin(); i != faces.end(); i += 3)
	{
		const unsigned int i1 = i[1];
		const unsigned int i2 = i[2];
		i[1] = i2;
		i[2] = i1;
	}
}

void VertexArray::FixWindingOrder()
{
	assert(faces.size() % 3 == 0);
	for (std::vector <unsigned int>::iterator i = faces.begin(); i != faces.end(); i += 3)
	{
		const float * n0 = &normals[i[0] * 3];
		const float * v0 = &vertices[i[0] * 3];
		const float * v1 = &vertices[i[1] * 3];
		const float * v2 = &vertices[i[2] * 3];
		const Vec3 norm0(n0[0], n0[1], n0[2]);
		const Vec3 vec0(v0[0], v0[1], v0[2]);
		const Vec3 vec1(v1[0], v1[1], v1[2]);
		const Vec3 vec2(v2[0], v2[1], v2[2]);
		const Vec3 norm1 = (vec1 - vec0).cross(vec2 - vec1);
		if (norm0.dot(norm1) < 0.0f)
		{
			const unsigned int i1 = i[1];
			const unsigned int i2 = i[2];
			i[1] = i2;
			i[2] = i1;
		}
	}
}

bool VertexArray::Serialize(joeserialize::Serializer & s)
{
	_SERIALIZE_(s,vertices);
	_SERIALIZE_(s,normals);
	//_SERIALIZE_(s,colors); fixme
	_SERIALIZE_(s,texcoords);
	_SERIALIZE_(s,faces);
	return true;
}
/* fixme
QT_TEST(vertexarray_test)
{
	VertexArray testarray;

	const float * ptr;
	int ptrnum;
	float somevec[3];
	somevec[0] = somevec[2] = 0;
	somevec[1] = 1000.0;
	testarray.SetNormals(somevec, 3);
	testarray.GetNormals(ptr, ptrnum);
	QT_CHECK_EQUAL(ptr[1], 1000.0);
	QT_CHECK_EQUAL(ptrnum, 3);
	testarray.SetNormals(0, 0);
	testarray.GetNormals(ptr, ptrnum);
	QT_CHECK_EQUAL(ptrnum, 0);
	QT_CHECK_EQUAL(ptr, 0);

	//by similarity, the vertex and face assignment functions are OK if the above normal test is OK

	testarray.GetTexCoords(ptr, ptrnum);
	QT_CHECK_EQUAL(ptrnum, 0);
	QT_CHECK_EQUAL(ptr, 0);
	testarray.SetTexCoords(somevec, 2);
	testarray.GetTexCoords(ptr, ptrnum);
	QT_CHECK_EQUAL(ptrnum, 2);
	QT_CHECK_EQUAL(ptr[1], 1000.0);

	testarray.Clear();
	testarray.SetNormals(somevec, 3);
	VertexArray otherarray(testarray);
	otherarray.GetNormals(ptr, ptrnum);
	QT_CHECK_EQUAL(ptr[1], 1000.0);
	QT_CHECK_EQUAL(ptrnum, 3);
	otherarray.Clear();
	otherarray.GetNormals(ptr, ptrnum);
	QT_CHECK_EQUAL(ptrnum, 0);
	otherarray = testarray;
	otherarray.GetNormals(ptr, ptrnum);
	QT_CHECK_EQUAL(ptr[1], 1000.0);
	QT_CHECK_EQUAL(ptrnum, 3);

	VertexArray addarray;
	otherarray.Clear();
	testarray.Clear();
	testarray.SetNormals(somevec, 3);
	otherarray.SetNormals(somevec, 3);
	addarray = testarray + otherarray;
	addarray.GetNormals(ptr, ptrnum);
	QT_CHECK_EQUAL(ptrnum, 6);
	QT_CHECK_EQUAL(ptr[1], 1000.0);
	QT_CHECK_EQUAL(ptr[4], 1000.0);

	testarray.Clear();
	VertexArray facearray1, facearray2;
	int someint[3];
	someint[0] = 0;
	someint[1] = 1;
	someint[2] = 2;
	facearray1.SetFaces(someint, 3);
	facearray2.SetFaces(someint, 3);
	testarray = facearray1 + facearray2;
	const int * ptri;
	testarray.GetFaces(ptri, ptrnum);
	QT_CHECK_EQUAL(ptrnum, 6);
	QT_CHECK_EQUAL(ptri[1], 1);
	QT_CHECK_EQUAL(ptri[4], 1);
	testarray.Clear();
	facearray1.SetVertices(somevec, 3);
	facearray2.SetVertices(somevec, 3);
	testarray = facearray1 + facearray2;
	testarray.GetFaces(ptri, ptrnum);
	QT_CHECK_EQUAL(ptrnum, 6);
	QT_CHECK_EQUAL(ptri[1], 1);
	QT_CHECK_EQUAL(ptri[4], 2);
}
*/
QT_TEST(vertexarray_buldfromfaces_test)
{
	std::vector <VertexArray::Float3> verts;
	verts.push_back(VertexArray::Float3(0,0,0));
	verts.push_back(VertexArray::Float3(1.0,-1.0,-1.0));
	verts.push_back(VertexArray::Float3(1.0,-1.0,1.0));
	verts.push_back(VertexArray::Float3(-1.0,-1.0,1.0));
	verts.push_back(VertexArray::Float3(-1.0,-1.0,-1.0));
	verts.push_back(VertexArray::Float3(1.0,1.0,-1.0));
	verts.push_back(VertexArray::Float3(1.0,1.0,1.0));
	verts.push_back(VertexArray::Float3(-1.0,1.0,1.0));
	verts.push_back(VertexArray::Float3(-1.0,1.0,-1.0));

	std::vector <VertexArray::Float3> norms;
	norms.push_back(VertexArray::Float3(0,0,0));
	norms.push_back(VertexArray::Float3(0.0,0.0,-1.0));
	norms.push_back(VertexArray::Float3(-1.0,-0.0,-0.0));
	norms.push_back(VertexArray::Float3(-0.0,-0.0,1.0));
	norms.push_back(VertexArray::Float3(-0.0,0.0,1.0));
	norms.push_back(VertexArray::Float3(1.0,-0.0,0.0));
	norms.push_back(VertexArray::Float3(1.0,0.0,0.0));
	norms.push_back(VertexArray::Float3(0.0,1.0,-0.0));
	norms.push_back(VertexArray::Float3(-0.0,-1.0,0.0));

	std::vector <VertexArray::Float2> texcoords(1);
	texcoords[0].u = 0;
	texcoords[0].v = 0;

	std::vector <VertexArray::Face> cubesides;
	cubesides.push_back(VertexArray::Face(VertexArray::VertexData(verts[5],norms[1],texcoords[0]),VertexArray::VertexData(verts[1],norms[1],texcoords[0]),VertexArray::VertexData(verts[8],norms[1],texcoords[0])));
	cubesides.push_back(VertexArray::Face(VertexArray::VertexData(verts[1],norms[1],texcoords[0]),VertexArray::VertexData(verts[4],norms[1],texcoords[0]),VertexArray::VertexData(verts[8],norms[1],texcoords[0])));
	cubesides.push_back(VertexArray::Face(VertexArray::VertexData(verts[3],norms[2],texcoords[0]),VertexArray::VertexData(verts[7],norms[2],texcoords[0]),VertexArray::VertexData(verts[8],norms[2],texcoords[0])));
	cubesides.push_back(VertexArray::Face(VertexArray::VertexData(verts[3],norms[2],texcoords[0]),VertexArray::VertexData(verts[8],norms[2],texcoords[0]),VertexArray::VertexData(verts[4],norms[2],texcoords[0])));
	cubesides.push_back(VertexArray::Face(VertexArray::VertexData(verts[2],norms[3],texcoords[0]),VertexArray::VertexData(verts[6],norms[3],texcoords[0]),VertexArray::VertexData(verts[3],norms[3],texcoords[0])));
	cubesides.push_back(VertexArray::Face(VertexArray::VertexData(verts[6],norms[4],texcoords[0]),VertexArray::VertexData(verts[7],norms[4],texcoords[0]),VertexArray::VertexData(verts[3],norms[4],texcoords[0])));
	cubesides.push_back(VertexArray::Face(VertexArray::VertexData(verts[1],norms[5],texcoords[0]),VertexArray::VertexData(verts[5],norms[5],texcoords[0]),VertexArray::VertexData(verts[2],norms[5],texcoords[0])));
	cubesides.push_back(VertexArray::Face(VertexArray::VertexData(verts[5],norms[6],texcoords[0]),VertexArray::VertexData(verts[6],norms[6],texcoords[0]),VertexArray::VertexData(verts[2],norms[6],texcoords[0])));
	cubesides.push_back(VertexArray::Face(VertexArray::VertexData(verts[5],norms[7],texcoords[0]),VertexArray::VertexData(verts[8],norms[7],texcoords[0]),VertexArray::VertexData(verts[7],norms[7],texcoords[0])));
	cubesides.push_back(VertexArray::Face(VertexArray::VertexData(verts[5],norms[7],texcoords[0]),VertexArray::VertexData(verts[7],norms[7],texcoords[0]),VertexArray::VertexData(verts[6],norms[7],texcoords[0])));
	cubesides.push_back(VertexArray::Face(VertexArray::VertexData(verts[1],norms[8],texcoords[0]),VertexArray::VertexData(verts[2],norms[8],texcoords[0]),VertexArray::VertexData(verts[3],norms[8],texcoords[0])));
	cubesides.push_back(VertexArray::Face(VertexArray::VertexData(verts[1],norms[8],texcoords[0]),VertexArray::VertexData(verts[3],norms[8],texcoords[0]),VertexArray::VertexData(verts[4],norms[8],texcoords[0])));

	VertexArray varray;
	varray.BuildFromFaces(cubesides);

	const float * tempfloat(NULL);
	const unsigned int * tempint(NULL);
	int tempnum;

	varray.GetNormals(tempfloat, tempnum);
	QT_CHECK(tempfloat != NULL);
	QT_CHECK_EQUAL(tempnum,72);

	varray.GetVertices(tempfloat, tempnum);
	QT_CHECK(tempfloat != NULL);
	QT_CHECK_EQUAL(tempnum,72);

	varray.GetTexCoords(tempfloat, tempnum);
	QT_CHECK(tempfloat != NULL);
	QT_CHECK_EQUAL(tempnum,48);

	varray.GetFaces(tempint, tempnum);
	QT_CHECK(tempint != NULL);
	QT_CHECK_EQUAL(tempnum,36);
}

