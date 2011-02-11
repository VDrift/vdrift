#ifndef _TRACK_H
#define _TRACK_H

#include "scenenode.h"
#include "trackobject.h"
#include "tracksurface.h"
#include "roadstrip.h"
#include "mathvector.h"
#include "quaternion.h"

#include <string>
#include <iostream>
#include <list>
#include <memory>
#include <vector>

class TEXTUREMANAGER;
class MODELMANAGER;
class OBJECTLOADER;
class ROADSTRIP;

class TRACK
{
public:
	TRACK(std::ostream & info, std::ostream & error);
	
	~TRACK();
	
	void Clear();
	
	///returns true if successful.  loads the entire track with this one function call.
	bool Load(
		TEXTUREMANAGER & textures,
		MODELMANAGER & models,
		const std::string & trackpath,
		const std::string & trackdir,
		const std::string & effects_texturepath,
		const std::string & texsize,
		const int anisotropy,
		const bool reverse);
	
	///returns true if successful.  only begins loading the track; the track won't be loaded until more calls to ContinueDeferredLoad().  use Loaded() to see if loading is complete yet.
	bool DeferredLoad(
		TEXTUREMANAGER & textures,
		MODELMANAGER & models,
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

	std::pair <MATHVECTOR <float, 3>, QUATERNION <float> > GetStart(unsigned int index) const;
	
	int GetNumStartPositions() const
	{
		return start_positions.size();
	}
	
	bool Loaded() const
	{
		return loaded;
	}
	
	bool CastRay(
		const MATHVECTOR <float, 3> & origin,
		const MATHVECTOR <float, 3> & direction,
		const float seglen,
		int & patch_id,
		MATHVECTOR <float, 3> & outtri,
		const BEZIER * & colpatch,
		MATHVECTOR <float, 3> & normal) const;
	
	const std::list <ROADSTRIP> & GetRoadList() const
	{
		return roads;
	}
	
	unsigned int GetSectors() const
	{
		return lapsequence.size();
	}
	
	const BEZIER * GetLapSequence(unsigned int sector) const
	{
		assert (sector < lapsequence.size());
		return lapsequence[sector];
	}
	
	void SetRacingLineVisibility(bool newvis)
	{
		racingline_visible = newvis;
	}
	
	void Unload()
	{
		Clear();
	}
	
	bool IsReversed() const
	{
		return direction == DIRECTION_REVERSE;
	}
	
	const std::vector<TRACKOBJECT> & GetTrackObjects() const
	{
		return objects;
	}
	
	SCENENODE & GetRacinglineNode()
	{
		if (racingline_visible)
			return racingline_node;
		else 
			return empty_node;
	}

	SCENENODE & GetTrackNode()
	{
		return tracknode;
	}

private:
	std::ostream & info_output;
	std::ostream & error_output;
	std::vector <std::pair <MATHVECTOR <float, 3>, QUATERNION <float> > > start_positions;
	std::vector <TRACKSURFACE> tracksurfaces;
	std::vector <TRACKOBJECT> objects;
	std::vector <std::tr1::shared_ptr<MODEL> > models;
	bool vertical_tracking_skyboxes;
	
	enum
	{
		DIRECTION_FORWARD,
		DIRECTION_REVERSE
	} direction;
	
	//road information
	std::list <ROADSTRIP> roads;
	
	//the sequence of beziers that a car needs to hit to do a lap
	std::vector <const BEZIER *> lapsequence;
	
	//racing line data
	SCENENODE racingline_node;
	SCENENODE empty_node;
	std::tr1::shared_ptr<TEXTURE> racingline_texture;
	bool racingline_visible;
	
	SCENENODE tracknode;
	
	bool loaded;
	bool cull;
	
	std::auto_ptr <OBJECTLOADER> objload;
	
	bool CreateRacingLines(
		const std::string & texturepath,
		const std::string & texsize,
		TEXTUREMANAGER & textures);
	
	bool LoadParameters(const std::string & trackpath);
	
	bool LoadSurfaces(const std::string & trackpath);
	
	bool LoadObjects(
		SCENENODE & sceneroot,
		TEXTUREMANAGER & textures,
		MODELMANAGER & models,
		const std::string & trackpath,
		const std::string & trackdir,
		const std::string & texsize,
		const int anisotropy);
	
	///returns false on error
	bool BeginObjectLoad(
		SCENENODE & sceneroot,
		TEXTUREMANAGER & textures,
		MODELMANAGER & models,
		const std::string & trackpath,
		const std::string & trackdir,
		const std::string & texsize,
		const int anisotropy,
		const bool dynamicshadowsenabled,
		const bool doagressivecombining);
	
	///returns a pair of bools: the first bool is true if there was an error, the second bool is true if an object was loaded
	std::pair <bool, bool> ContinueObjectLoad();
	
	bool LoadRoads(const std::string & trackpath, bool reverse);
	
	bool LoadLapSequence(const std::string & trackpath, bool reverse);
	
	void ClearRoads() {roads.clear();}
	
	void Reverse();
};

#endif
