#ifndef _PARTICLE_H
#define _PARTICLE_H

#include "mathvector.h"
#include "reseatable_reference.h"
#include "optional.h"
#include "scenegraph.h"
#include "vertexarray.h"
#include "texture.h"

#include <ostream>
#include <vector>
#include <cmath>
#include <algorithm>
#include <map>

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
		
		reseatable_reference <SCENENODE> node;
		reseatable_reference <DRAWABLE> draw;
		VERTEXARRAY varray;
		
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
			//std::cout << "\tCopied node: " << &node.get() << endl;
			draw = other.draw;
			varray = other.varray;
			
			//reseat the drawable's varray reference
			draw->SetVertArray(&varray);
		}

	public:
		PARTICLE(SCENENODE & parentnode, const VEC & new_start_position, const VEC & new_dir,
			 float newspeed, float newtrans, float newlong, float newsize, TEXTURE_GL & tex)
			: transparency(newtrans), longevity(newlong), start_position(new_start_position),
			  speed(newspeed), direction(new_dir), size(newsize), time(0)
		{
			node = parentnode.AddNode();
			//std::cout << "Created node: " << &node.get() << endl;
			draw = node->AddDrawable();
			draw->SetDrawEnable(false);
			draw->SetVertArray(&varray);
			draw->SetDiffuseMap(&tex);
			draw->SetSmoke(true);
			draw->SetCull(false,false);
			draw->SetPartialTransparency(true);
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
		
		SCENENODE & GetNode()
		{
			return *node;
		}
		
		void Update(float dt, const QUATERNION <float> & camdir_conjugate)
		{
			time += dt;
			
			node->GetTransform().SetTranslation(start_position + direction * time * speed);
			//std::cout << "particle position: " << start_position << std::endl;
			//MATHVECTOR <float,3> v(0,1,0);
			//camdir_conjugate.RotateVector(v);
			//std::cout << v << std::endl;
			//QUATERNION <float> rot = camdir_conjugate;
			//rot = rot * camdir_conjugate;
			//rot.Rotate(3.141593*0.5,1,0,0);
			node->GetTransform().SetRotation(camdir_conjugate);
			
			float sizescale = 1.0;
			float trans = transparency*std::pow((double)(1.0-time/longevity),4.0);
			float transmax = 1.0;
			if (trans > transmax)
				trans = transmax;
			if (trans < 0.0)
				trans = 0.0;
	
			sizescale = 5.0*(time/longevity)+1.0;
			
			varray.SetToBillboard(-sizescale,-sizescale,sizescale,sizescale);
			draw->SetRadius(sizescale);
			draw->SetColor(1,1,1,trans);
			draw->SetDrawEnable(true);
		}
		
		bool Expired() const
		{
			return (time > longevity);
		}
	};
	
	std::vector <PARTICLE> particles;
	std::list <TEXTURE_GL> textures;
	std::list <TEXTURE_GL>::iterator cur_texture;
	
	std::pair <float,float> transparency_range;
	std::pair <float,float> longevity_range;
	std::pair <float,float> speed_range;
	std::pair <float,float> size_range;
	MATHVECTOR <float, 3> direction;
	
	reseatable_reference <SCENENODE> node;
	
public:
	PARTICLE_SYSTEM() : transparency_range(0.5,1), longevity_range(5,14), speed_range(0.3,1),
		size_range(0.5,1), direction(0,1,0)
	{
		particles.reserve(128);
	}
	
	///returns true if at least one particle texture was loaded
	bool Load(SCENENODE & parentnode, const std::list <std::string> & texlist, int anisotropy, const std::string & texsize, std::ostream & error_output);
	void Update(float dt, const QUATERNION <float> & camdir);
	
	/// all of the parameters are from 0.0 to 1.0 and scale to the ranges set with SetParameters.  testonly should be kept false and is only used for unit testing.
	void AddParticle(const MATHVECTOR <float,3> & position, float newspeed, float newtrans, float newlong, float newsize, bool testonly=false);
	void Clear();
	void SetParameters(float transmin, float transmax, float longmin, float longmax,
		float speedmin, float speedmax, float sizemin, float sizemax,
		MATHVECTOR <float,3> newdir);
	unsigned int NumParticles() {return particles.size();}
};

#endif
