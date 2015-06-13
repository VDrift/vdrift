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

#include "particle.h"
#include "content/contentmanager.h"
#include "graphics/texture.h"
#include "unittest.h"

static inline float clamp(float v, float vmin, float vmax)
{
	return std::max(vmin, std::min(vmax, v));
}

static inline float lerp(float x, float y, float s)
{
	return x + clamp(s, 0, 1) * (y - x);
}

ParticleSystem::ParticleSystem() :
	max_particles(512),
	texture_tiles(9),
	cur_texture_tile(0),
	transparency_range(0.5,1),
	longevity_range(5,14),
	speed_range(0.3,1),
	size_range(0.5,1),
	direction(0,1,0)
{
	// ctor
}

void ParticleSystem::Load(
	const std::string & texpath,
	const std::string & texname,
	int anisotropy,
	ContentManager & content)
{
	TextureInfo texinfo;
	texinfo.anisotropy = anisotropy;
	content.load(texture, texpath, texname, texinfo);

	draw = GetDrawList(node).insert(Drawable());
	Drawable & drawref = GetDrawList(node).get(draw);
	drawref.SetDrawEnable(false);
	drawref.SetVertArray(&varray);
	drawref.SetTextures(texture->GetId());
	drawref.SetCull(false);
}

void ParticleSystem::Update(float dt)
{
	//  update particles
	for (size_t i = 0; i < particles.size(); i++)
	{
		particles[i].time += dt;
	}

	// remove expired particles
	for (size_t i = 0; i < particles.size(); i++)
	{
		if (particles[i].time > particles[i].longevity)
		{
			//only bother to swap if it's not already at the end
			size_t last = particles.size() - 1;
			if (i != last)
				particles[i] = particles[last];

			particles.pop_back();
		}
	}
}

void ParticleSystem::UpdateGraphics(
	const Quat & camdir,
	const Vec3 & campos,
	float znear,
	float zfar,
	float /*fovy*/,
	float /*fovz*/)
{
	if (max_particles == 0)
		return;

	node.GetTransform().SetTranslation(campos);
	node.GetTransform().SetRotation(-camdir);

	// get particle position in camera space
	distance_from_cam.clear();
	for (unsigned i = 0; i < particles.size(); ++i)
	{
		Particle & p = particles[i];
		Vec3 pos = p.start_position;
		pos = pos + p.direction * p.time * p.speed - campos;
		camdir.RotateVector(pos);

		// signed distance along z-axis in camera space
		distance_from_cam.push_back(-pos[2]);

		// store camera space position
		p.position = pos;
	}

	// sort particles by distance to camera

	// update vertex data
	varray.Clear();
	for (size_t i = 0; i < particles.size(); ++i)
	{
		// cull particles outside of [znear, zfar]
		// todo: cull particles outside of view frustum
		if (distance_from_cam[i] < znear || distance_from_cam[i] > zfar)
			continue;

		Particle & p = particles[i];
		Vec3 pos = p.position;

		float trans = p.transparency * std::pow((1.0f - p.time / p.longevity), 4);
		trans = clamp(trans, 0.0f, 1.0f);

		float sizescale = 0.2f * (p.time / p.longevity) + 0.4f;
/*
		// scale the alpha by the closeness to the camera. if we get too close, don't draw
		// this prevents major slowdown when there are a lot of particles right next to the camera
		float camdist = pos.Magnitude();
		const float camdist_off = 3.0;
		const float camdist_full = 4.0;
		trans = lerp(0.f, trans, (camdist - camdist_off) / (camdist_full - camdist_off));
*/
		// assume 9 tiles in texture atlas
		int vi = p.tid / 3;
		int ui = p.tid - vi * 3;
		float u1 = ui * 1 / 3.0f;
		float v1 = vi * 1 / 3.0f;
		float u2 = u1 + 1 / 3.0f;
		float v2 = v1 + 1 / 3.0f;
		float x1 = -sizescale;
		float y1 = -sizescale * 2 / 3.0f;
		float x2 = sizescale;
		float y2 = sizescale * 4 / 3.0f;
		unsigned char alpha = trans * 255;

		const unsigned int faces[6] = {
			0, 2, 1,
			0, 3, 2,
		};
		const float uvs[8] = {
			u1, v1,
			u2, v1,
			u2, v2,
			u1, v2,
		};
		const float verts[12] = {
			pos[0] + x1, pos[1] + y1, pos[2],
			pos[0] + x2, pos[1] + y1, pos[2],
			pos[0] + x2, pos[1] + y2, pos[2],
			pos[0] + x1, pos[1] + y2, pos[2],
		};
		const unsigned char cols[16] = {
			255, 255, 255, alpha,
			255, 255, 255, alpha,
			255, 255, 255, alpha,
			255, 255, 255, alpha,
		};

		varray.Add(faces, 6, verts, 12, uvs, 8, 0, 0, cols, 16);
	}

	GetDrawList(node).get(draw).SetDrawEnable(varray.GetNumIndices() > 0);
}

void ParticleSystem::AddParticle(
	const Vec3 & position,
	float newspeed)
{
	if (max_particles == 0)
		return;

	while (particles.size() >= max_particles)
		particles.pop_back();

	particles.push_back(Particle());
	Particle & p = particles.back();
	p.start_position = position;
	p.direction = direction;
	p.transparency = transparency_range.first + newspeed * (transparency_range.second - transparency_range.first);
	p.speed = speed_range.first + newspeed * (speed_range.second - speed_range.first);
	p.size = size_range.first + newspeed * (size_range.second - size_range.first);
	p.longevity = longevity_range.first + newspeed * (longevity_range.second - longevity_range.first);
	p.time = 0;
	p.tid = cur_texture_tile;

	cur_texture_tile = (cur_texture_tile + 1) % texture_tiles;
}

void ParticleSystem::Clear()
{
	particles.clear();
}

void ParticleSystem::SetParameters(
	int maxparticles,
	float transmin,
	float transmax,
	float longmin,
	float longmax,
	float speedmin,
	float speedmax,
	float sizemin,
	float sizemax,
	Vec3 newdir)
{
	max_particles = maxparticles < 0 ? 0 : (maxparticles > 1024 ? 1024 : maxparticles);
	particles.reserve(max_particles);
	distance_from_cam.reserve(max_particles);

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
	std::ostringstream out;
	ParticleSystem s;
	ContentManager c(out);
	s.SetParameters(4,1.0,1.0,0.5,1.0,1.0,1.0,1.0,1.0,Vec3(0,1,0));
	s.Load(std::string(), std::string(), 0, c);

	//test basic particle management:  adding particles and letting them expire and get removed over time
	QT_CHECK_EQUAL(s.NumParticles(),0);
	s.AddParticle(Vec3(0,0,0),0);
	QT_CHECK_EQUAL(s.NumParticles(),1);
	s.AddParticle(Vec3(0,0,0),1);
	QT_CHECK_EQUAL(s.NumParticles(),2);
	s.Update(0.45);
	QT_CHECK_EQUAL(s.NumParticles(),2);
	s.Update(0.10);
	QT_CHECK_EQUAL(s.NumParticles(),1);
	s.Update(0.50);
	QT_CHECK_EQUAL(s.NumParticles(),0);
}
