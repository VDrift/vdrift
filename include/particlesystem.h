#ifndef _PARTICLESYSTEM_H
#define _PARTICLESYSTEM_H

#include "particle.h"

#include <string>
#include <ostream>
#include <vector>
#include <list>
#include <map>

class ContentManager;

class ParticleSystem
{
public:
	ParticleSystem();

	///returns true if at least one particle texture was loaded
	bool Load(
		const std::list <std::string> & texlist,
		int anisotropy,
		const std::string & texsize,
		ContentManager * content,
		std::ostream & error_output);

	void Update(
		float dt,
		const QUATERNION <float> & camdir,
		const MATHVECTOR <float, 3> & campos);

	/// all of the parameters are from 0.0 to 1.0 and scale to the ranges set with SetParameters.  testonly should be kept false and is only used for unit testing.
	void AddParticle(
		const MATHVECTOR <float,3> & position,
		float newspeed,
		float newtrans,
		float newlong,
		float newsize,
		bool testonly = false);

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

	unsigned int NumParticles()
	{
		return particles.size();
	}

	SCENENODE & GetNode()
	{
		return node;
	}

private:
	std::vector <Particle> particles;
	std::list <TexturePtr> textures;
	std::list <TexturePtr>::iterator cur_texture;

	std::pair <float, float> transparency_range;
	std::pair <float, float> longevity_range;
	std::pair <float, float> speed_range;
	std::pair <float, float> size_range;
	MATHVECTOR <float, 3> direction;

	const unsigned int max_particles;

	SCENENODE node;
};

#endif // _PARTICLESYSTEM_H
