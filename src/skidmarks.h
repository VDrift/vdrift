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

#ifndef _SKIDMARKS_H
#define _SKIDMARKS_H

#include "graphics/scenenode.h"
#include "graphics/vertexarray.h"
#include "mathvector.h"
#include "quaternion.h"

#include <memory>
#include <string>
#include <vector>

class ContentManager;
class Texture;

class SkidMarks
{
public:
	/// Load texture
	void Load(
		const std::string & texpath,
		const std::string & texname,
		int anisotropy,
		ContentManager & content);

	void Clear();

	void Reset(int num_emitters, int max_marks = 1024);

	void UpdateEmitter(int id, float intensity, Vec3 corner_left, Vec3 corner_right);

	void UpdateGraphics(
		const Quat & camdir,
		const Vec3 & campos,
		float znear, float zfar,
		float sinfovh);

	SceneNode & GetNode() { return node; }

private:
	struct Mark
	{
		Vec3 corners[4];
		Vec3 center;
		float radius;
		float fade;
	};
	struct Emitter
	{
		float energy = 0;
		int markid = -1;
	};
	std::vector<Mark> marks;
	std::vector<Emitter> emitters;

	SceneNode::DrawableHandle draw;
	std::shared_ptr<Texture> texture;
	VertexArray varray;
	SceneNode node;

	int first_mark = 0;
	int next_mark = 0;
	int max_marks = 0;
	float max_mark_length_sq = (0.2f * 0.2f);
	float min_emission_energy = 5.0f;

	void NewMark(Emitter & e, float energy = 0);
};

#endif // _SKIDMARKS_H
