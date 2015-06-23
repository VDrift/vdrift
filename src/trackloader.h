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

#ifndef _TRACKLOADER_H
#define _TRACKLOADER_H

#include "track.h"
#include "cfg/ptree.h"
#include "joepack.h"

/*
[object.foo]
#position = 0, 0, 0
#rotation = 0, 0, 0

[object.foo.body]
texture = diffuse.png
model = body.joe
#clampuv = 0
#mipmap = true
#skybox = false
#nolighting = false
#alphablend = false
#doublesided = false
#isashadow = false
#collideable = true
#surface = 0
#mass = 0
#size = 1, 1, 1 # axis aligned bounding box
*/

class DynamicsWorld;
class ContentManager;
class btStridingMeshInterface;
class btCompoundShape;
class btCollisionShape;
class PTree;

class Track::Loader
{
public:
	Loader(
		ContentManager & content,
		DynamicsWorld & world,
		Track::Data & data,
		std::ostream & info_output,
		std::ostream & error_output,
		const std::string & trackpath,
		const std::string & trackdir,
		const std::string & texturedir,
		const std::string & sharedobjectpath,
		const int anisotropy,
		const bool reverse,
		const bool dynamic_shadows,
		const bool dynamic_objects);

	~Loader();

	bool BeginLoad();

	bool ContinueLoad();

	int GetNumObjects() const { return numobjects; }

	int GetNumLoaded() const { return numloaded; }

private:
	ContentManager & content;
	DynamicsWorld & world;
	Track::Data & data;
	std::ostream & info_output;
	std::ostream & error_output;

	const std::string & trackpath;
	const std::string & trackdir;
	const std::string & texturedir;
	const std::string & sharedobjectpath;
	const int anisotropy;
	const bool dynamic_objects;
	const bool dynamic_shadows;

	std::string objectpath;
	std::string objectdir;
	std::ifstream objectfile;
	JoePack pack;
	bool packload;
	int numobjects;
	int numloaded;
	int params_per_object;
	const int expected_params;
	const int min_params;
	bool error;
	bool list;

	// pod for references
	struct Body
	{
		Body() : nolighting(false), skybox(false), mesh(0), shape(0),
			mass(0), surface(0), collidable(false)
		{
			// ctor
		}
		Drawable drawable;
		bool nolighting;
		bool skybox;
		btStridingMeshInterface * mesh;
		btCollisionShape * shape;
		btVector3 inertia;
		btVector3 center;
		float mass;
		int surface;
		bool collidable;
	};
	typedef std::map<std::string, Body>::const_iterator body_iterator;
	std::map<std::string, Body> bodies;

	// compound track shape
	btCompoundShape * track_shape;

	// track config
	std::shared_ptr<PTree> track_config;
	const PTree * nodes;
	PTree::const_iterator node_it;

	bool LoadSurfaces();

	bool LoadRoads();

	bool CreateRacingLines();

	void CreateRacingLine(const RoadStrip & strip);

	bool LoadStartPositions(const PTree & info);

	bool LoadLapSections(const PTree & info);

	bool BeginObjectLoad();

	std::pair<bool, bool> ContinueObjectLoad();

	bool Begin();

	bool BeginOld();

	std::pair<bool, bool> Continue();

	std::pair<bool, bool> ContinueOld();

	void CalculateNumOld();

	bool LoadNode(const PTree & sec);

	bool LoadShape(const PTree & body_cfg, const Model & body_model, Body & body);

	body_iterator LoadBody(const PTree & cfg);

	void AddBody(SceneNode & scene, const Body & body);

	struct Object;
	bool AddObject(const Object & object);

	void Clear();
};

#endif // _TRACKLOADER_H
