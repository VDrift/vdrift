#include "particlesystem.h"

#include "contentmanager.h"
#include "textureloader.h"
#include "unittest.h"

ParticleSystem::ParticleSystem() :
	transparency_range(0.5, 1),
	longevity_range(5, 14),
	speed_range(0.3, 1),
	size_range(0.5, 1),
	direction(0, 1, 0),
	max_particles(128)
{
	particles.reserve(max_particles);
	cur_texture = textures.end();
}

bool ParticleSystem::Load(
	const std::list <std::string> & texlist,
	int anisotropy,
	const std::string & texsize,
	ContentManager * content,
	std::ostream & error_output)
{
	if (!content) return false;

	TextureLoader texload;
	texload.anisotropy = anisotropy;
	texload.size = texsize;
	for (std::list <std::string>::const_iterator i = texlist.begin(); i != texlist.end(); ++i)
	{
		texload.name = *i;
		TexturePtr texture = content->get<TEXTURE>(texload);
		textures.push_back(texture);
	}
	cur_texture = textures.end();
	return !textures.empty();
}

bool inverseorder(int i1, int i2)
{
	return i2 < i1;
}

void ParticleSystem::Update(float dt, const QUATERNION <float> & camdir, const MATHVECTOR <float, 3> & campos)
{
	QUATERNION <float> camdir_conj = -camdir;

	std::vector <int> expired_list;

	for (unsigned int i = 0; i < particles.size(); i++)
	{
		particles[i].Update(node, dt, camdir_conj, campos);
		if (particles[i].Expired())
			expired_list.push_back(i);
	}

	//must sort our expired list so highest numbers come first or swap & pop won't work
	std::sort(expired_list.begin(), expired_list.end(), inverseorder);

	if (expired_list.size() == particles.size() && !particles.empty())
	{
		//std::cout << "Clearing all nodes" << std::endl;
		Clear();
	}
	else if (!expired_list.empty())
	{
		assert(expired_list.size() < particles.size());

		//std::cout << "Getting ready to delete " << expired_list.size() << "/" << particles.size() << std::endl;

		//do the old swap & pop trick to remove expired particles quickly.
		//all swaps must be done before the popping starts, because popping invalidates iterators
		int newback = particles.size()-1;
		for (std::vector <int>::iterator i = expired_list.begin(); i != expired_list.end(); ++i)
		{
			//only bother to swap if it's not already at the end
			if (*i != newback)
			{
				std::swap(particles[*i], particles[newback]);
			}

			//std::cout << "Deleted node: " << &particles[newback].GetNode() << endl;
			node.Delete(particles[newback].GetNode());

			newback--;
		}

		//do all of the pops
		//std::cout << expired_list.size() << ", " << particles.size() << ", " << newback << std::endl;
		assert((int)particles.size() - (int)expired_list.size() == newback + 1);
		for (unsigned int i = 0; i < expired_list.size(); i++)
			particles.pop_back();
		//assert((int)expired_list.size() == (int)particles.size() - newback);
	}
}

void ParticleSystem::AddParticle(
	const MATHVECTOR <float,3> & position,
	float newspeed,
	float newtrans,
	float newlong,
	float newsize,
	bool testonly)
{
	if (cur_texture == textures.end())
		cur_texture = textures.begin();

	TexturePtr tex;
	if (!testonly)
	{
		assert(cur_texture != textures.end()); //this should only happen if the textures array is empty, which should never happen unless we're doing a unit test
		tex = *cur_texture;
	}

	while (particles.size() >= max_particles)
	{
		particles.pop_back();
	}

	particles.push_back(
		Particle(node, position, direction,
				speed_range.first+newspeed*(speed_range.second-speed_range.first),
				transparency_range.first+newspeed*(transparency_range.second-transparency_range.first),
				longevity_range.first+newspeed*(longevity_range.second-longevity_range.first),
				size_range.first+newspeed*(size_range.second-size_range.first),
				tex));

	if (cur_texture != textures.end())
		cur_texture++;
}

void ParticleSystem::Clear()
{
	for (std::vector <Particle>::iterator i = particles.begin(); i != particles.end(); ++i)
	{
		node.Delete(i->GetNode());
	}
	particles.clear();
}

void ParticleSystem::SetParameters(
	float transmin,
	float transmax,
	float longmin,
	float longmax,
	float speedmin,
	float speedmax,
	float sizemin,
	float sizemax,
	MATHVECTOR <float,3> newdir)
{
	transparency_range.first = transmin;
	transparency_range.second = transmax;
	longevity_range.first = longmin;
	longevity_range.second = longmax;
	speed_range.first = speedmin;
	speed_range.second = speedmax;
	size_range.first = sizemin;
	size_range.second = sizemax;
	direction = newdir;
}

QT_TEST(particle_test)
{
	ParticleSystem s;
	s.SetParameters(1.0,1.0,0.5,1.0,1.0,1.0,1.0,1.0,MATHVECTOR<float,3>(0,1,0));
	std::stringstream out;
	s.Load(std::list<std::string> (), 0, "large", NULL, out);

	//test basic particle management:  adding particles and letting them expire and get removed over time
	QT_CHECK_EQUAL(s.NumParticles(),0);
	s.AddParticle(MATHVECTOR<float,3>(0,0,0),0,0,0,0,true);
	QT_CHECK_EQUAL(s.NumParticles(),1);
	s.AddParticle(MATHVECTOR<float,3>(0,0,0),1,1,1,1,true);
	QT_CHECK_EQUAL(s.NumParticles(),2);
	QUATERNION <float> dir;
	MATHVECTOR <float, 3> pos;
	s.Update(0.45, dir, pos);
	QT_CHECK_EQUAL(s.NumParticles(),2);
	s.Update(0.1, dir, pos);
	QT_CHECK_EQUAL(s.NumParticles(),1);
	s.Update(0.5, dir, pos);
	QT_CHECK_EQUAL(s.NumParticles(),0);
}
