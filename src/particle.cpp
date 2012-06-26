#include "particle.h"
#include "contentmanager.h"
#include "textureinfo.h"
#include "unittest.h"

bool PARTICLE_SYSTEM::Load(
	const std::list <std::string> & texlist,
	const std::string & texpath,
	int anisotropy,
	ContentManager & content)
{
	TEXTUREINFO texinfo;
	texinfo.anisotropy = anisotropy;
	for (std::list <std::string>::const_iterator i = texlist.begin(); i != texlist.end(); ++i)
	{
		std::tr1::shared_ptr<TEXTURE> tex;
		content.load(texpath, *i, texinfo, tex);
		textures.push_back(tex);
	}
	cur_texture = textures.end();
	return !textures.empty();
}

bool inverseorder(int i1, int i2)
{
	return i2 < i1;
}

void PARTICLE_SYSTEM::Update(float dt, const QUATERNION <float> & camdir, const MATHVECTOR <float, 3> & campos)
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

void PARTICLE_SYSTEM::AddParticle(
	const MATHVECTOR <float,3> & position,
	float newspeed,
	bool testonly)
{
	if (cur_texture == textures.end())
		cur_texture = textures.begin();

	std::tr1::shared_ptr<TEXTURE> tex;
	if (!testonly)
	{
		assert(cur_texture != textures.end()); //this should only happen if the textures array is empty, which should never happen unless we're doing a unit test
		tex = *cur_texture;
	}

	const unsigned int max_particles = 8*128;

	while (particles.size() >= max_particles)
		particles.pop_back();

	particles.push_back(PARTICLE(node, position, direction,
			    speed_range.first+newspeed*(speed_range.second-speed_range.first),
			    transparency_range.first+newspeed*(transparency_range.second-transparency_range.first),
			    longevity_range.first+newspeed*(longevity_range.second-longevity_range.first),
			    size_range.first+newspeed*(size_range.second-size_range.first),
				tex));

	if (cur_texture != textures.end())
		cur_texture++;
}

void PARTICLE_SYSTEM::Clear()
{
	for (std::vector <PARTICLE>::iterator i = particles.begin(); i != particles.end(); ++i)
	{
		node.Delete(i->GetNode());
	}

	particles.clear();
}

void PARTICLE_SYSTEM::SetParameters(float transmin, float transmax, float longmin, float longmax,
	float speedmin, float speedmax, float sizemin, float sizemax,
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
	std::stringstream out;
	PARTICLE_SYSTEM s;
	ContentManager c(out);
	s.SetParameters(1.0,1.0,0.5,1.0,1.0,1.0,1.0,1.0,MATHVECTOR<float,3>(0,1,0));
	s.Load(std::list<std::string> (), std::string(), 0, c);

	//test basic particle management:  adding particles and letting them expire and get removed over time
	QT_CHECK_EQUAL(s.NumParticles(),0);
	s.AddParticle(MATHVECTOR<float,3>(0,0,0),0,true);
	QT_CHECK_EQUAL(s.NumParticles(),1);
	s.AddParticle(MATHVECTOR<float,3>(0,0,0),1,true);
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
