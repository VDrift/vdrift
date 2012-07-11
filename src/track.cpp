#include "track.h"
#include "trackloader.h"
#include "dynamicsworld.h"
#include "tobullet.h"
#include "coordinatesystem.h"
#include "reseatable_reference.h"

#include <algorithm>
#include <list>
#include <map>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>

TRACK::TRACK() : racingline_visible(false)
{
	// Constructor.
}

TRACK::~TRACK()
{
	Clear();
}

bool TRACK::DeferredLoad(
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
	const bool dynamicshadows,
	const bool agressivecombine)
{
	Clear();

	world.reset(*this);
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

bool TRACK::CastRay(const MATHVECTOR <float, 3> & origin, const MATHVECTOR <float, 3> & direction, const float seglen, int & patch_id, MATHVECTOR <float, 3> & outtri, const BEZIER * & colpatch, MATHVECTOR <float, 3> & normal) const
{
	bool col = false;
	for (std::list <ROADSTRIP>::const_iterator i = data.roads.begin(); i != data.roads.end(); ++i)
	{
		MATHVECTOR <float, 3> coltri, colnorm;
		const BEZIER * colbez = NULL;
		if (i->Collide(origin, direction, seglen, patch_id, coltri, colbez, colnorm))
		{
			if (!col || (coltri-origin).Magnitude() < (outtri-origin).Magnitude())
			{
				outtri = coltri;
				normal = colnorm;
				colpatch = colbez;
			}

			col = true;
		}
	}

	return col;
}

void TRACK::Update()
{
	if (!data.loaded) return;

	std::list<MotionState>::const_iterator t = data.body_transforms.begin();
	for (int i = 0, e = data.body_nodes.size(); i < e; ++i, ++t)
	{
		TRANSFORM & vt = data.dynamic_node.GetNode(data.body_nodes[i]).GetTransform();
		vt.SetRotation(ToMathQuaternion<float>(t->rotation));
		vt.SetTranslation(ToMathVector<float>(t->position));
	}
}

std::pair <MATHVECTOR <float, 3>, QUATERNION <float> > TRACK::GetStart(unsigned int index) const
{
	assert(!data.start_positions.empty());
	unsigned int laststart = data.start_positions.size() - 1;
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













TRACK::DATA::DATA(): world(0), vertical_tracking_skyboxes(false), loaded(false), cull(true)
{
	// Constructor.
}





optional <const BEZIER *> ROADSTRIP::FindBezierAtOffset(const BEZIER * bezier, int offset) const
{
	std::vector<ROADPATCH>::const_iterator it = patches.end(); //this iterator will hold the found ROADPATCH

	//search for the roadpatch containing the bezier and store an iterator to it in "it"
	for (std::vector<ROADPATCH>::const_iterator i = patches.begin(); i != patches.end(); ++i)
	{
		if (&i->GetPatch() == bezier)
		{
			it = i;
			break;
		}
	}

	if (it == patches.end())
		return optional <const BEZIER *>(); //return nothing
	else
	{
		//now do the offset
		int curoffset = offset;
		while (curoffset != 0)
		{
			if (curoffset < 0)
			{
				//why is this so difficult?  all i'm trying to do is make the iterator loop around
				std::vector<ROADPATCH>::const_reverse_iterator rit(it);
				if (rit == patches.rend())
					rit = patches.rbegin();
				rit++;
				if (rit == patches.rend())
					rit = patches.rbegin();
				it = rit.base();
				if (it == patches.end())
					it = patches.begin();

				curoffset++;
			}
			else if (curoffset > 0)
			{
				it++;
				if (it == patches.end())
					it = patches.begin();

				curoffset--;
			}
		}

		assert(it != patches.end());
		return optional <const BEZIER *>(&it->GetPatch());
	}
}


