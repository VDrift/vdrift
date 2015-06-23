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

#ifndef _PARTICLE_H
#define _PARTICLE_H

#include "graphics/scenenode.h"
#include "graphics/vertexarray.h"
#include "mathvector.h"
#include "quaternion.h"

#include <memory>
#include <string>
#include <utility> // std::pair
#include <vector>

class ContentManager;
class Texture;

class ParticleSystem
{
public:
	ParticleSystem();

	/// Load texture atlas and setup drawable.
	void Load(
		const std::string & texpath,
		const std::string & texname,
		int anisotropy,
		ContentManager & content);

	/// Parameters are from 0.0 to 1.0 and scale to the ranges set with SetParameters.
	void AddParticle(
		const Vec3 & position,
		float newspeed);

	/// Particle physics update.
	void Update(float dt);

	/// Partcles graphics update based on last physics state.
	/// Call once per frame.
	void UpdateGraphics(
		const Quat & camdir,
		const Vec3 & campos,
		float znear, float zfar,
		float fovy = 0, float fovz = 0);

	void Clear();

	void SetParameters(
		int maxparticles,
		float transmin,
		float transmax,
		float longmin,
		float longmax,
		float speedmin,
		float speedmax,
		float sizemin,
		float sizemax,
		Vec3 newdir);

	unsigned NumParticles() { return particles.size(); }

	SceneNode & GetNode() { return node; }

private:
	struct Particle
	{
		Vec3 start_position; ///< start position in world space
		Vec3 direction;		///< direction in world space
		Vec3 position;		///< position in camera space
		float transparency; ///< transparency factor
		float speed;		///< initial velocity along direction
		float size;			///< initial size
		float longevity;	///< particle age limit
		float time;			///< particle age, time since the particle was created
		int tid;			///< particle texture atlas tile id 0-8
	};
	std::vector<Particle> particles;
	std::vector<float> distance_from_cam;
	unsigned max_particles;
	unsigned texture_tiles;
	unsigned cur_texture_tile;

	std::pair<float,float> transparency_range;
	std::pair<float,float> longevity_range;
	std::pair<float,float> speed_range;
	std::pair<float,float> size_range;
	Vec3 direction;

	SceneNode::DrawableHandle draw;
	std::shared_ptr<Texture> texture;
	VertexArray varray;
	SceneNode node;

	static keyed_container<Drawable> & GetDrawList(SceneNode & node)
	{
		return node.GetDrawList().particle;
	}
};

#endif
