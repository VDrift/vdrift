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
#include "physics/dynamicsworld.h"
#include "coordinatesystem.h"
#include "tobullet.h"

#include "BulletCollision/CollisionShapes/btCollisionShape.h"
#include "BulletCollision/CollisionShapes/btStridingMeshInterface.h"

Track::Track() : racingline_visible(false)
{
	// Constructor.
}

Track::~Track()
{
	Clear();
}

bool Track::DeferredLoad(
	ContentManager & content,
	DynamicsWorld & world,
	std::ostream & info_output,
	std::ostream & error_output,
	const std::string & trackpath,
	const std::string & trackdir,
	const std::string & texturedir,
	const std::string & sharedobjectpath,
	const int anisotropy,
	const bool reverse,
	const bool dynamicobjects,
	const bool dynamicshadows)
{
	Clear();

	world.reset(*this);
	data.world = &world;

	loader.reset(
		new Loader(
			content, world, data,
			info_output, error_output,
			trackpath, trackdir,
			texturedir,	sharedobjectpath,
			anisotropy, reverse,
			dynamicobjects,
			dynamicshadows));

	return loader->BeginLoad();
}

bool Track::ContinueDeferredLoad()
{
	assert(loader.get());
	return loader->ContinueLoad();
}

int Track::ObjectsNum() const
{
	assert(loader.get());
	return loader->GetNumObjects();
}

int Track::ObjectsNumLoaded() const
{
	assert(loader.get());
	return loader->GetNumLoaded();
}

bool Track::Loaded() const
{
	return data.loaded;
}

void Track::Clear()
{
	for (auto & object : data.objects)
	{
		data.world->removeCollisionObject(object);
		delete object;
	}
	data.objects.clear();

	for (auto & shape : data.shapes)
	{
		delete shape;
	}
	data.shapes.clear();

	for (auto & mesh : data.meshes)
	{
		delete mesh;
	}
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

bool Track::CastRay(
	const Vec3 & origin,
	const Vec3 & direction,
	const float seglen,
	int & patch_id,
	Vec3 & outtri,
	const RoadPatch * & colpatch,
	Vec3 & normal) const
{
	bool col = false;
	for (const auto & road : data.roads)
	{
		Vec3 tri, norm;
		const RoadPatch * patch = NULL;
		if (road.Collide(origin, direction, seglen, patch_id, tri, patch, norm))
		{
			if (!col || (tri - origin).MagnitudeSquared() < (outtri - origin).MagnitudeSquared())
			{
				outtri = tri;
				normal = norm;
				colpatch = patch;
			}
			col = true;
		}
	}
	return col;
}

void Track::Update()
{
	if (!data.loaded) return;

	auto t = data.body_transforms.begin();
	for (int i = 0, e = data.body_nodes.size(); i < e; ++i, ++t)
	{
		Transform & vt = data.dynamic_node.GetNode(data.body_nodes[i]).GetTransform();
		vt.SetRotation(ToQuaternion<float>(t->rotation));
		vt.SetTranslation(ToMathVector<float>(t->position));
	}
}

std::pair <Vec3, Quat > Track::GetStart(unsigned int index) const
{
	assert(!data.start_positions.empty());
	unsigned int laststart = data.start_positions.size() - 1;
	if (index > laststart)
	{
		std::pair <Vec3, Quat > sp = data.start_positions[laststart];
		Vec3 backward = -Direction::Forward * 6 * (index - laststart);
		sp.second.RotateVector(backward);
		sp.first = sp.first + backward;
		return sp;
	}
	return data.start_positions[index];
}

Track::Data::Data() :
	world(0),
	reverse(false),
	loaded(false),
	cull(true),
	vertical_tracking_skyboxes(false)
{
	// Constructor.
}



