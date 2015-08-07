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

#ifndef _TRACK_H
#define _TRACK_H

#include "roadstrip.h"
#include "mathvector.h"
#include "quaternion.h"
#include "graphics/scenenode.h"
#include "physics/motionstate.h"
#include "physics/tracksurface.h"

#include <memory>
#include <iosfwd>
#include <string>
#include <list>
#include <set>
#include <vector>

class Model;
class Texture;
class RoadStrip;
class DynamicsWorld;
class ContentManager;
class btStridingMeshInterface;
class btCollisionShape;
class btCollisionObject;

class Track
{
public:
	Track();
	~Track();

	/// Only begins loading the track.
    /// The track won't be loaded until more calls to ContinueDeferredLoad().
    /// Use Loaded() to see if loading is complete yet.
    /// Returns true if successful.
	bool DeferredLoad(
		ContentManager & content,
		DynamicsWorld & world,
		std::ostream & info_output,
		std::ostream & error_output,
		const std::string & trackpath,
		const std::string & trackdir,
		const std::string & effects_texturepath,
		const std::string & sharedobjectpath,
		const int anisotropy,
		const bool reverse,
		const bool dynamicobjects,
		const bool dynamicshadows);

	bool ContinueDeferredLoad();

	/// Start loading thread.
	void Load();

	/// Number of objects to load in total.
	int ObjectsNum() const;

	/// Number of objects loaded.
	int ObjectsNumLoaded() const;

	/// Track loading status.
	bool Loaded() const;

	void Clear();

	bool CastRay(
		const Vec3 & origin,
		const Vec3 & direction,
		const float seglen,
		int & patch_id,
		Vec3 & outtri,
		const Bezier * & colpatch,
		Vec3 & normal) const;

	/// Synchronize graphics and physics.
	void Update();

	std::pair <Vec3, Quat > GetStart(unsigned int index) const;

	int GetNumStartPositions() const
	{
		return data.start_positions.size();
	}

	const std::list <RoadStrip> & GetRoadList() const
	{
		return data.roads;
	}

	unsigned int GetSectors() const
	{
		return data.lap.size();
	}

	const Bezier * GetSectorPatch(unsigned int sector) const
	{
		assert (sector < data.lap.size());
		return data.lap[sector];
	}

	void SetRacingLineVisibility(bool newvis)
	{
		racingline_visible = newvis;
	}

	bool IsReversed() const
	{
		return data.reverse;
	}

	bool IsFixedSkybox() const
	{
		return !data.vertical_tracking_skyboxes;
	}

	const std::vector<TrackSurface> & GetSurfaces() const
	{
		return data.surfaces;
	}

	SceneNode & GetRacinglineNode()
	{
		if (racingline_visible)
			return data.racingline_node;
		else
			return empty_node;
	}

	SceneNode & GetTrackNode()
	{
		return data.static_node;
	}

	SceneNode & GetBodyNode()
	{
		return data.dynamic_node;
	}

private:
	struct Data
	{
		DynamicsWorld* world;

		// content used by track
		std::set<std::shared_ptr<Model> > models;
		std::set<std::shared_ptr<Texture> > textures;

		// static track objects
		SceneNode static_node;
		std::vector<TrackSurface> surfaces;
		std::vector<btStridingMeshInterface*> meshes;
		std::vector<btCollisionShape*> shapes;
		std::vector<btCollisionObject*> objects;

		// dynamic track objects
		SceneNode dynamic_node;
		std::vector<SceneNode::Handle> body_nodes;
		std::list<MotionState> body_transforms;

		// road information
		std::vector<const Bezier*> lap;
		std::list<RoadStrip> roads;
		std::vector<std::pair<Vec3, Quat > > start_positions;

		SceneNode racingline_node;

		// track state
		bool reverse;
		bool loaded;
		bool cull;
		bool vertical_tracking_skyboxes;

		Data();
	};

	Data data;
	bool racingline_visible;
	SceneNode empty_node;

	// temporary loading data
	class Loader;
	std::unique_ptr<Loader> loader;
};

#endif
