#ifndef _TRACKLOADER_H
#define _TRACKLOADER_H

#include "track.h"
#include "cfg/ptree.h"
#include "joepack.h"

#include <ostream>
#include <string>

/*
[body.foo]
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
#mass = 0 # to be implemented
#size = 1, 1, 1 # axis aligned bounding box

[node.bla]
body = foo
#position = 0, 0, 0
#rotation = 0, 0, 0
*/

class COLLISION_WORLD;
class TEXTUREMANAGER;
class MODELMANAGER;
class btStridingMeshInterface;
class btCompoundShape;
class btCollisionShape;

class TRACK::LOADER
{
public:
	LOADER(
		TEXTUREMANAGER & textures,
		MODELMANAGER & models,
		COLLISION_WORLD & world,
		DATA & data,
		std::ostream & info_output,
		std::ostream & error_output,
		const std::string & trackpath,
		const std::string & trackdir,
		const std::string & texturedir,
		const std::string & texsize,
		const int anisotropy,
		const bool reverse,
		const bool dynamic_shadows,
		const bool agressive_combining);
		
	~LOADER();
	
	bool BeginLoad();
	
	bool ContinueLoad();
	
	int GetNumObjects() const {return numobjects;}

private:
	TEXTUREMANAGER & texture_manager;
	MODELMANAGER & model_manager;
	COLLISION_WORLD & world;
	DATA & data;
	std::ostream & info_output;
	std::ostream & error_output;
	
	const std::string & trackpath;
	const std::string & trackdir;
	const std::string & texturedir;
	const std::string & texsize;
	const int anisotropy;
	const bool reverse;
	const bool dynamic_shadows;
	const bool agressive_combining;
	
	std::string objectpath;
	std::string objectdir;
	std::ifstream objectfile;
	JOEPACK pack;
	bool packload;
	int numobjects;
	int params_per_object;
	const int expected_params;
	const int min_params;
	bool error;
	bool list;
	
	// pod for references
	struct BODY
	{
		BODY() : nolighting(false), mesh(0), shape(0), 
			mass(0), surface(0), collidable(true)
		{
			// ctor
		}
		DRAWABLE drawable;
		bool nolighting;
		btStridingMeshInterface * mesh;
		btCollisionShape * shape;
		btVector3 inertia;
		float mass;
		int surface;
		bool collidable;
	};
	typedef std::map<std::string, BODY>::const_iterator body_iterator;
	std::map<std::string, BODY> bodies;
	
	// compound track shape
	btCompoundShape * track_shape;
	
	// track config
	PTree track_config;
	const PTree * nodes;
	PTree::const_iterator node_it;
	
	bool LoadParameters();
	
	bool LoadSurfaces();
	
	bool LoadRoads();
	
	void ReverseRoads();
	
	bool LoadLapSequence();
	
	bool CreateRacingLines();
	
	bool BeginObjectLoad();
	
	std::pair<bool, bool> ContinueObjectLoad();
	
	bool Begin();
	
	bool BeginOld();
	
	std::pair<bool, bool> Continue();
	
	std::pair<bool, bool> ContinueOld();
	
	void CalculateNumOld();
	
	bool LoadNode(const PTree & sec);
	
	body_iterator LoadBody(const std::string & name);
	
	void AddBody(SCENENODE & scene, const BODY & body);
	
	void Clear();
};

#endif // _TRACKLOADER_H
