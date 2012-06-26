#ifndef _PARTICLE_H
#define _PARTICLE_H

#include "mathvector.h"
#include "scenenode.h"
#include "vertexarray.h"

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
			std::tr1::shared_ptr<TEXTURE> texture) :
			transparency(newtrans),
			longevity(newlong),
			start_position(new_start_position),
			speed(newspeed),
			direction(new_dir),
			size(newsize),
			time(0)
		{
			node = parentnode.AddNode();
			SCENENODE & noderef = parentnode.GetNode(node);
			//std::cout << "Created node: " << &node.get() << endl;
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

			varray.SetToBillboard(-sizescale,-sizescale,sizescale,sizescale);
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

	std::vector <PARTICLE> particles;
	std::list <std::tr1::shared_ptr<TEXTURE> > textures;
	std::list <std::tr1::shared_ptr<TEXTURE> >::iterator cur_texture;

	std::pair <float,float> transparency_range;
	std::pair <float,float> longevity_range;
	std::pair <float,float> speed_range;
	std::pair <float,float> size_range;
	MATHVECTOR <float, 3> direction;

	SCENENODE node;

public:
	PARTICLE_SYSTEM() :
		transparency_range(0.5,1),
		longevity_range(5,14),
		speed_range(0.3,1),
		size_range(0.5,1),
		direction(0,1,0)
	{
		particles.reserve(128);
		cur_texture = textures.end();
	}

	///returns true if at least one particle texture was loaded
	bool Load(
		const std::list <std::string> & texlist,
		const std::string & texpath,
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
		bool testonly=false);

	void Clear();

	void SetParameters(
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
