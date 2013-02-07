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

#include "mathvector.h"
#include "graphics/scenenode.h"
#include "graphics/vertexarray.h"

#include <ostream>
#include <utility> // std::pair
#include <vector>
#include <list>

class ContentManager;

class PARTICLE_SYSTEM
{
private:
	class PARTICLE
	{
	private:
		typedef MATHVECTOR<float,3> VEC;

		float transparency;
		float longevity;
		VEC start_position;
		float speed;
		VEC direction;
		float size;
		float time; ///< time since the particle was created; i.e. the particle's age
		int tid; ///< particle texture atlas tile id 0-8

		keyed_container <SCENENODE>::handle node;
		keyed_container <DRAWABLE>::handle draw;
		VERTEXARRAY varray;

		static keyed_container <DRAWABLE> & GetDrawlist(SCENENODE & node)
		{
			return node.GetDrawlist().particle;
		}

		void Set(const PARTICLE & other)
		{
			transparency = other.transparency;
			longevity = other.longevity;
			start_position = other.start_position;
			speed = other.speed;
			direction = other.direction;
			size = other.size;
			time = other.time;
			tid = other.tid;
			node = other.node;
			draw = other.draw;
			varray = other.varray;
		}

	public:
		PARTICLE(
			SCENENODE & parentnode,
			const VEC & new_start_position,
			const VEC & new_dir,
			float newspeed,
			float newtrans,
			float newlong,
			float newsize,
			int newtid,
			std::tr1::shared_ptr<TEXTURE> & texture) :
			transparency(newtrans),
			longevity(newlong),
			start_position(new_start_position),
			speed(newspeed),
			direction(new_dir),
			size(newsize),
			time(0),
			tid(newtid)
		{
			node = parentnode.AddNode();
			SCENENODE & noderef = parentnode.GetNode(node);
			draw = GetDrawlist(noderef).insert(DRAWABLE());
			DRAWABLE & drawref = GetDrawlist(noderef).get(draw);
			drawref.SetDrawEnable(false);
			drawref.SetVertArray(&varray);
			drawref.SetDiffuseMap(texture);
			drawref.SetCull(false,false);
		}

		PARTICLE(const PARTICLE & other)
		{
			Set(other);
		}

		PARTICLE & operator=(const PARTICLE & other)
		{
			Set(other);
			return *this;
		}

		keyed_container <SCENENODE>::handle & GetNode()
		{
			return node;
		}

		float lerp(float x, float y, float s)
		{
			float sclamp = std::max(0.f,std::min(1.0f,s));
			return x + sclamp*(y-x);
		}

		void Update(
			SCENENODE & parent,
			float dt,
			const QUATERNION <float> & camdir_conjugate,
			const MATHVECTOR <float, 3> & campos)
		{
			time += dt;

			SCENENODE & noderef = parent.GetNode(node);
			DRAWABLE & drawref = GetDrawlist(noderef).get(draw);
			drawref.SetVertArray(&varray);

			MATHVECTOR <float, 3> curpos = start_position + direction * time * speed;
			noderef.GetTransform().SetTranslation(curpos);
			noderef.GetTransform().SetRotation(camdir_conjugate);

			float sizescale = 1.0;
			float trans = transparency*std::pow((double)(1.0-time/longevity),4.0);
			float transmax = 1.0;
			if (trans > transmax)
				trans = transmax;
			if (trans < 0.0)
				trans = 0.0;

			sizescale = 0.2*(time/longevity)+0.4;

			// assume 9 tiles in texture atlas
			int vi = tid / 3;
			int ui = tid - vi * 3;
			float u1 = ui * 1 / 3.0f;
			float v1 = vi * 1 / 3.0f;
			float u2 = u1 + 1 / 3.0f;
			float v2 = v1 + 1 / 3.0f;
			float x1 = -sizescale;
			float y1 = -sizescale * 2 / 3.0f;
			float x2 = sizescale;
			float y2 = sizescale * 4 / 3.0f;

			varray.SetTo2DQuad(x1, y1, x2, y2, u1, v1, u2, v2);
			drawref.SetRadius(sizescale);

			bool drawenable = true;

			// scale the alpha by the closeness to the camera
			// if we get too close, don't draw
			// this prevents major slowdown when there are a lot of particles right next to the camera
			float camdist = (curpos - campos).Magnitude();
			//std::cout << camdist << std::endl;
			const float camdist_off = 3.0;
			const float camdist_full = 4.0;
			trans = lerp(0.f,trans,(camdist-camdist_off)/(camdist_full-camdist_off));
			if (trans <= 0)
				drawenable = false;

			drawref.SetColor(1,1,1,trans);
			drawref.SetDrawEnable(drawenable);
		}

		bool Expired() const
		{
			return (time > longevity);
		}
	};

	std::tr1::shared_ptr<TEXTURE> texture_atlas;
	std::vector <PARTICLE> particles;
	unsigned max_particles;
	unsigned cur_texture_tile;
	const unsigned texture_tiles;

	std::pair <float,float> transparency_range;
	std::pair <float,float> longevity_range;
	std::pair <float,float> speed_range;
	std::pair <float,float> size_range;
	MATHVECTOR <float, 3> direction;

	SCENENODE node;

public:
	PARTICLE_SYSTEM() :
		max_particles(512),
		cur_texture_tile(0),
		texture_tiles(9),
		transparency_range(0.5,1),
		longevity_range(5,14),
		speed_range(0.3,1),
		size_range(0.5,1),
		direction(0,1,0)
	{
		// ctor
	}

	/// returns true if particle texture atlas was loaded
	bool Load(
		const std::string & texpath,
		const std::string & texname,
		int anisotropy,
		ContentManager & content);

	void Update(
		float dt,
		const QUATERNION <float> & camdir,
		const MATHVECTOR <float, 3> & campos);

	/// all of the parameters are from 0.0 to 1.0 and scale to the ranges set with SetParameters.  testonly should be kept false and is only used for unit testing.
	void AddParticle(
		const MATHVECTOR <float,3> & position,
		float newspeed,
		bool testonly = false);

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
		MATHVECTOR <float,3> newdir);

	unsigned int NumParticles() {return particles.size();}

	SCENENODE & GetNode() {return node;}
};

#endif
