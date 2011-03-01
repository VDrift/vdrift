#ifndef _TRACK_H
#define _TRACK_H

#include "scenenode.h"
#include "tracksurface.h"
#include "roadstrip.h"
#include "mathvector.h"
#include "quaternion.h"
#include "LinearMath/btDefaultMotionState.h"
#include "LinearMath/btAlignedObjectArray.h"

#include <string>
#include <iostream>
#include <list>
#include <memory>
#include <vector>

class COLLISION_WORLD;
class TEXTUREMANAGER;
class MODELMANAGER;
class OBJECTLOADER;
class ROADSTRIP;
class btStridingMeshInterface;
class btCollisionShape;
class btCollisionObject;

class TRACK
{
public:
	TRACK();
	
	~TRACK();
	
	/// true if successful.  only begins loading the track; the track won't be loaded until more calls to ContinueDeferredLoad().  use Loaded() to see if loading is complete yet.
	bool DeferredLoad(
		TEXTUREMANAGER & textures,
		MODELMANAGER & models,
		COLLISION_WORLD & world,
		std::ostream & info_output,
		std::ostream & error_output,
		const std::string & trackpath,
		const std::string & trackdir,
		const std::string & effects_texturepath,
		const std::string & texsize,
		const int anisotropy,
		const bool reverse,
		const bool dynamicshadowsenabled,
		const bool doagressivecombining);
	
	bool ContinueDeferredLoad();
	
	int DeferredLoadTotalObjects() const;
	
	bool Loaded() const
	{
		return data.loaded;
	}
	
	void Clear();
	
	bool CastRay(
		const MATHVECTOR <float, 3> & origin,
		const MATHVECTOR <float, 3> & direction,
		const float seglen,
		int & patch_id,
		MATHVECTOR <float, 3> & outtri,
		const BEZIER * & colpatch,
		MATHVECTOR <float, 3> & normal) const;
		
	/// syncronize graphics and physics once per frame
	void Update();
	
	std::pair <MATHVECTOR <float, 3>, QUATERNION <float> > GetStart(unsigned int index) const;
	
	int GetNumStartPositions() const
	{
		return data.start_positions.size();
	}
	
	const std::list <ROADSTRIP> & GetRoadList() const
	{
		return data.roads;
	}
	
	unsigned int GetSectors() const
	{
		return data.lap.size();
	}
	
	const BEZIER * GetLapSequence(unsigned int sector) const
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
		return data.direction == DATA::DIRECTION_REVERSE;
	}
	
	const std::vector<TRACKSURFACE> & GetSurfaces() const
	{
		return data.surfaces;
	}
	
	SCENENODE & GetRacinglineNode()
	{
		if (racingline_visible)
			return data.racingline_node;
		else 
			return empty_node;
	}
	
	SCENENODE & GetTrackNode()
	{
		return data.static_node;
	}
	
	SCENENODE & GetBodyNode()
	{
		return data.dynamic_node;
	}

private:
	struct DATA
	{
		COLLISION_WORLD * world;
		
		// static track objects
		SCENENODE static_node;
		std::vector <TRACKSURFACE> surfaces;
		std::vector <std::tr1::shared_ptr<MODEL> > models;
		btAlignedObjectArray <btStridingMeshInterface *> meshes;
		btAlignedObjectArray <btCollisionShape *> shapes;
		btAlignedObjectArray <btCollisionObject *> objects;
		
		// dynamic track objects
		SCENENODE dynamic_node;
		std::vector<keyed_container<SCENENODE>::handle> body_nodes;
		std::list<btDefaultMotionState> body_transforms;
		
		// road information
		std::vector <const BEZIER *> lap;
		std::list <ROADSTRIP> roads;
		std::vector <std::pair<MATHVECTOR <float, 3>, QUATERNION <float> > > start_positions;
		
		// racing line data
		SCENENODE racingline_node;
		std::tr1::shared_ptr<TEXTURE> racingline_texture;
		
		// track state
		enum
		{
			DIRECTION_FORWARD,
			DIRECTION_REVERSE
		} direction;
		bool vertical_tracking_skyboxes;
		bool loaded;
		bool cull;
		
		DATA();
		
	} data;
	
	bool racingline_visible;
	SCENENODE empty_node;
	
	// temporary loading data
	class LOADER;
	std::auto_ptr <LOADER> loader;
};

#endif
