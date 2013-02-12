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

#include <ostream>
#include <string>
#include <utility> // std::pair
#include <vector>

class ContentManager;

class PARTICLE_SYSTEM
{
public:
	PARTICLE_SYSTEM();

	/// Load texture atlas and setup drawable.
	void Load(
		const std::string & texpath,
		const std::string & texname,
		int anisotropy,
		ContentManager & content);

	/// Parameters are from 0.0 to 1.0 and scale to the ranges set with SetParameters.
	void AddParticle(
		const MATHVECTOR<float,3> & position,
		float newspeed);

	/// Particle physics update.
	void Update(float dt);

	/// Partcles graphics update based on last physics state.
	/// Call once per frame.
	void UpdateGraphics(
		const QUATERNION<float> & camdir,
		const MATHVECTOR<float, 3> & campos);

	/// Particle system is double buffered
	/// Call SyncGraphics between FinishDraw and BeginDraw
	/// to submit latest particle state for drawing
	void SyncGraphics();

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
		MATHVECTOR<float,3> newdir);

	unsigned NumParticles() { return particles.size(); }

	SCENENODE & GetNode() { return node; }

private:
	struct PARTICLE
	{
		MATHVECTOR<float,3> start_position; ///< start position in world space
		MATHVECTOR<float,3> direction;		///< direction in world space
		MATHVECTOR<float,3> position;		///< position in camera space
		float transparency; ///< transparency factor
		float speed;		///< initial velocity along direction
		float size;			///< initial size
		float longevity;	///< particle age limit
		float time;			///< particle age, time since the particle was created
		int tid;			///< particle texture atlas tile id 0-8
	};
	std::vector<PARTICLE> particles;
	std::vector<float> distance_from_cam;
	unsigned max_particles;
	unsigned texture_tiles;
	unsigned cur_texture_tile;
	unsigned cur_varray;

	std::pair<float,float> transparency_range;
	std::pair<float,float> longevity_range;
	std::pair<float,float> speed_range;
	std::pair<float,float> size_range;
	MATHVECTOR<float, 3> direction;

	keyed_container<DRAWABLE>::handle draw;
	VERTEXARRAY varrays[2]; ///< use double buffered vertex array
	SCENENODE node;

	static keyed_container<DRAWABLE> & GetDrawlist(SCENENODE & node)
	{
		return node.GetDrawlist().particle;
	}
};

#endif
