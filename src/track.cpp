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

#include "track.h"
#include "trackloader.h"
#include "physics/world.h"
#include "physics/ray.h"
#include "coordinatesystem.h"
#include "tobullet.h"

// required to be able to call the destructor
#include "BulletCollision/CollisionShapes/btCollisionShape.h"
#include "BulletCollision/CollisionShapes/btStridingMeshInterface.h"

struct TRACK::RayTestProcessor : public sim::World::RayProcessor
{
	DATA & m_data;
	sim::Ray * m_ray;

	RayTestProcessor(DATA & data);

	void set(sim::Ray & ray);

	btScalar addSingleResult(
		btCollisionWorld::LocalRayResult & rayResult,
		bool normalInWorldSpace);
};

TRACK::TRACK()
{
	// Constructor.
}

TRACK::~TRACK()
{
	Clear();
}

bool TRACK::DeferredLoad(
	ContentManager & content,
	sim::World & world,
	std::ostream & info_output,
	std::ostream & error_output,
	const std::string & trackpath,
	const std::string & trackdir,
	const std::string & texturedir,
	const std::string & sharedobjectpath,
	const int anisotropy,
	const bool reverse,
	const bool dynamicobjects,
	const bool dynamicshadows,
	const bool agressivecombine)
{
	Clear();

	world.setRayProcessor(*data.rayTestProcessor);
	data.world = &world;

	loader.reset(
		new LOADER(
			content, world, data,
			info_output, error_output,
			trackpath, trackdir,
			texturedir,	sharedobjectpath,
			anisotropy, reverse,
			dynamicobjects,
			dynamicshadows,
			agressivecombine));

	return loader->BeginLoad();
}

bool TRACK::ContinueDeferredLoad()
{
	assert(loader.get());
	return loader->ContinueLoad();
}

int TRACK::ObjectsNum() const
{
	assert(loader.get());
	return loader->GetNumObjects();
}

int TRACK::ObjectsNumLoaded() const
{
	assert(loader.get());
	return loader->GetNumLoaded();
}

bool TRACK::Loaded() const
{
    return data.loaded;
}

void TRACK::Clear()
{
	for (int i = 0, n = data.objects.size(); i < n; ++i)
	{
		data.world->removeCollisionObject(data.objects[i]);
		delete data.objects[i];
	}
	data.objects.clear();

	for (int i = 0, n = data.shapes.size(); i < n; ++i)
	{
		btCollisionShape * shape = data.shapes[i];
		delete shape;
	}
	data.shapes.clear();

	for (int i = 0, n = data.meshes.size(); i < n; ++i)
		delete data.meshes[i];
	data.meshes.clear();

	data.static_node.Clear();
	data.surfaces.clear();
	data.models.clear();
	data.dynamic_node.Clear();
	data.body_nodes.clear();
	data.body_transforms.clear();
	data.lap.clear();
	data.roads.clear();
	data.start_positions.clear();
	data.racingline_node.Clear();
	data.loaded = false;
}

void TRACK::Update()
{
	if (!data.loaded) return;

	std::list<sim::MotionState>::const_iterator t = data.body_transforms.begin();
	for (size_t i = 0, e = data.body_nodes.size(); i < e; ++i, ++t)
	{
		TRANSFORM & vt = data.dynamic_node.GetNode(data.body_nodes[i]).GetTransform();
		vt.SetRotation(cast(t->rotation));
		vt.SetTranslation(cast(t->position));
	}
}

std::pair <MATHVECTOR <float, 3>, QUATERNION <float> > TRACK::GetStart(unsigned int index) const
{
	assert(!data.start_positions.empty());
	size_t laststart = data.start_positions.size() - 1;
	if (index > laststart)
	{
		std::pair <MATHVECTOR <float, 3>, QUATERNION <float> > sp = data.start_positions[laststart];
		MATHVECTOR <float, 3> backward = -direction::Forward * 6 * (index - laststart);
		sp.second.RotateVector(backward);
		sp.first = sp.first + backward;
		return sp;
	}
	return data.start_positions[index];
}

TRACK::DATA::DATA() :
	world(0),
	vertical_tracking_skyboxes(false),
	racingline_visible(false),
	reverse(false),
	loaded(false),
	cull(true)
{
	rayTestProcessor = new RayTestProcessor(*this);
}

TRACK::DATA::~DATA()
{
	assert(rayTestProcessor);
	delete rayTestProcessor;
}

TRACK::RayTestProcessor::RayTestProcessor(DATA & data) :
	m_data(data),
	m_ray(0)
{
	// Constructor
}

void TRACK::RayTestProcessor::set(sim::Ray & ray)
{
	m_ray = &ray;
}

btScalar TRACK::RayTestProcessor::addSingleResult(
	btCollisionWorld::LocalRayResult & rayResult,
	bool normalInWorldSpace)
{
	// We only support static mesh collision shapes
	if (!(rayResult.m_collisionObject->getCollisionFlags() & btCollisionObject::CF_STATIC_OBJECT) &&
		(rayResult.m_collisionObject->getCollisionShape()->getShapeType() != TRIANGLE_MESH_SHAPE_PROXYTYPE))
	{
		return 1.0;
	}
	//std::cerr << rayResult.m_hitFraction << "    ";

	// Road bezier patch ray test
	assert(m_ray);
	btVector3 rayFrom = m_ray->m_rayFrom;
	btVector3 rayTo = m_ray->m_rayTo;
	btScalar rayLen = m_ray->m_rayLen;
	btVector3 rayVec = rayTo - rayFrom;

	// patch space
	MATHVECTOR<float, 3> from(rayFrom[1], rayFrom[2], rayFrom[0]);
	MATHVECTOR<float, 3> to(rayTo[1], rayTo[2], rayTo[0]);
	MATHVECTOR<float, 3> dir(rayVec[1], rayVec[2], rayVec[0]);
	dir = dir / rayLen;

	int patchId = m_ray->m_patchid;
	const BEZIER * colBez = 0;
	MATHVECTOR<float, 3> colPoint;
	MATHVECTOR<float, 3> colNormal;
	for (std::list<ROADSTRIP>::const_iterator i = m_data.roads.begin(); i != m_data.roads.end(); ++i)
	{
		const BEZIER * bez(0);
		MATHVECTOR<float, 3> norm;
		MATHVECTOR<float, 3> point(to);
		if (i->Collide(from, dir, rayLen, patchId, point, bez, norm) &&
			((point - from).MagnitudeSquared() < (colPoint - from).MagnitudeSquared()))
		{
			colPoint = point;
			colNormal = norm;
			colBez = bez;
		}
	}

	if (colBez)
	{
		btVector3 hitPoint(colPoint[2], colPoint[0], colPoint[1]);
		btVector3 hitNormal(colNormal[2], colNormal[0], colNormal[1]);
		btScalar dist_p = hitNormal.dot(hitPoint);
		btScalar dist_a = hitNormal.dot(rayFrom);
		btScalar dist_b = hitNormal.dot(rayTo);
		btScalar fraction = (dist_a - dist_p) / (dist_a - dist_b);
		rayResult.m_hitFraction = fraction;
		rayResult.m_hitNormalLocal = hitNormal;
		m_ray->m_patch = colBez;
		m_ray->m_patchid = patchId;
	}
	//std::cerr << rayResult.m_hitFraction << "    ";

	return m_ray->addSingleResult(rayResult, true);
}
