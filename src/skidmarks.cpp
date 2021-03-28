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

#include "skidmarks.h"
#include "content/contentmanager.h"
#include "graphics/texture.h"
#include "conecull.h"

#include <cassert>

static inline keyed_container<Drawable> & GetDrawList(SceneNode & node)
{
	return node.GetDrawList().normal_blend;
}

static inline Drawable & GetDrawable(SceneNode & node, SceneNode::DrawableHandle draw)
{
	return GetDrawList(node).get(draw);
}

void SkidMarks::Load(
	const std::string & texpath,
	const std::string & texname,
	int anisotropy,
	ContentManager & content)
{
	TextureInfo texinfo;
	texinfo.anisotropy = anisotropy;
	content.load(texture, texpath, texname, texinfo);

	draw = GetDrawList(node).insert(Drawable());
	Drawable & drawref = GetDrawable(node, draw);
	drawref.SetAlpha(0.5f);
	drawref.SetDrawEnable(false);
	drawref.SetVertArray(&varray);
	drawref.SetTextures(texture->GetId());
	drawref.SetDecal(true);
	drawref.SetCull(false);
}

void SkidMarks::Clear()
{
	marks.clear();
	emitters.clear();
	max_marks = 0;
	first_mark = 0;
	next_mark = 0;
}

void SkidMarks::Reset(int anum_emitters, int amax_marks)
{
	Clear();

	marks.resize(amax_marks);
	emitters.resize(anum_emitters);
	max_marks = amax_marks;
}

inline float BoundingRadius(Vec3 corners[4], Vec3 center)
{
	float rs = (corners[0] - center).MagnitudeSquared();
	for (int i = 1; i < 4; ++i)
		rs = Max(rs, (corners[i] - center).MagnitudeSquared());
	return std::sqrt(rs);
}

//#include <fstream>
//static std::ofstream dlog("log.txt");

void SkidMarks::UpdateEmitter(int id, float intensity, Vec3 corner_left, Vec3 corner_right)
{
	assert(id >= 0 && id < int(emitters.size()));
	if (max_marks == 0)
		return;

	auto & e = emitters[id];
	if (e.markid >= 0)
	{
		// update emitter energy
		e.energy = e.energy * 0.9f + intensity;

		auto & m = marks[e.markid];
		if (e.energy < min_emission_energy && m.radius < 1E-6f)
		{
			// reset mark
			//dlog << "reset "<< id << " " << e.markid << " " << e.energy << std::endl;
			m.corners[0] = corner_left;
			m.corners[1] = corner_right;
			m.fade = 1;
			return;
		}

		// update mark
		//dlog << "update " << id << " " << e.markid << " " << e.energy << std::endl;
		m.corners[2] = corner_left;
		m.corners[3] = corner_right;
		Vec3 center0 = (m.corners[0] + m.corners[1]) * 0.5f;
		Vec3 center1 = (m.corners[2] + m.corners[3]) * 0.5f;
		m.center = (center0 + center1) * 0.5f;
		m.radius = BoundingRadius(m.corners, m.center);

		// check mark length
		float cs = (center0 - center1).MagnitudeSquared();
		if (cs < max_mark_length_sq)
			return;

		if (e.energy + 1 < min_emission_energy)
		{
			// end mark trail
			//dlog << "end" << " r " << m.radius << std::endl;
			m.fade = -1;
			e.markid = -1;
			e.energy = 0;
			return;
		}

		// continue mark trail with a new mark
		//dlog << "continue " << id << " " << e.markid << " r " << m.radius << std::endl;
		NewMark(e, e.energy);
		auto & nm = marks[e.markid];
		nm.corners[0] = corner_left;
		nm.corners[1] = corner_right;
		nm.fade = 0;
		return;
	}

	if (intensity < 0.5f)
		return;

	// start new mark trail
	NewMark(e, intensity);
	//dlog << "start " << id << " " << e.markid << std::endl;
}

template <typename Mark>
inline void ProccessMark(Cone & c, Mark & m, VertexArray & v)
{
	//dlog << "mark " << m.radius << std::endl;
	if (m.radius <= 0 || c.cull(m.center, m.radius))
		return;
/*
	unsigned char a1 = 255;//(1 - m.intensity[0]) * 255;
	unsigned char a2 = 255;//(1 - m.intensity[1]) * 255;
*/
	float v0, v1;
	if (m.fade > 0)
	{
		v0 = 1.0f;
		v1 = 0.5f;
	}
	else if (m.fade < 0)
	{
		v0 = 0.5f;
		v1 = 1.0f;
	}
	else
	{
		v0 = 0.0f;
		v1 = 0.5f;
	}

	const unsigned int faces[6] = {
		0, 1, 2,
		2, 1, 3,
	};
	const float uvs[8] = {
		0.0f, v0,
		1.0f, v0,
		0.0f, v1,
		1.0f, v1,
	};
	const float verts[12] = {
		m.corners[0][0], m.corners[0][1], m.corners[0][2],
		m.corners[1][0], m.corners[1][1], m.corners[1][2],
		m.corners[2][0], m.corners[2][1], m.corners[2][2],
		m.corners[3][0], m.corners[3][1], m.corners[3][2],
	};
/*
	const unsigned char cols[16] = {
		255, 255, 255, a1,
		255, 255, 255, a1,
		255, 255, 255, a2,
		255, 255, 255, a2,
	};
*/
	v.Add(faces, 6, verts, 12, uvs, 8);//0, 0, cols, 16);
}

void SkidMarks::UpdateGraphics(
	const Quat & camdir,
	const Vec3 & campos,
	float znear, float zfar,
	float sinfovh)
{
	if (first_mark == next_mark)
	{
		GetDrawable(node, draw).SetDrawEnable(false);
		return;
	}

	varray.Clear();
	Cone cone(campos, camdir.AxisY(), sinfovh);
	if (first_mark < next_mark)
	{
		for (int i = first_mark; i < next_mark; ++i)
			ProccessMark(cone, marks[i], varray);
	}
	else
	{
		for (int i = 0; i < next_mark; ++i)
			ProccessMark(cone, marks[i], varray);
		for (int i = first_mark; i < max_marks; ++i)
			ProccessMark(cone, marks[i], varray);
	}
	GetDrawable(node, draw).SetDrawEnable(varray.GetNumVertices() > 0);
	//dlog << first_mark << " " << next_mark << " verts " << varray.GetNumVertices() << std::endl;
}

void SkidMarks::NewMark(Emitter & e, float energy)
{
	e.markid = next_mark;
	e.energy = energy;
	marks[next_mark].radius = 0;
	marks[next_mark].fade = 1;

	// advance used marks range pointers
	next_mark++;
	if (next_mark == max_marks) next_mark = 0;
	if (next_mark == first_mark) first_mark++;
	if (first_mark == max_marks) first_mark = 0;
}
