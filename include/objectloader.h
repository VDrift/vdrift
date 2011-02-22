#ifndef _OBJECTLOADER_H
#define _OBJECTLOADER_H

#include "scenenode.h"
#include "joepack.h"
#include "config.h"

class TEXTUREMANAGER;
class MODELMANAGER;
class TRACKSURFACE;
class TRACKOBJECT;
class MODEL;

class OBJECTLOADER
{
public:
	OBJECTLOADER(
		SCENENODE & sceneroot,
		TEXTUREMANAGER & texture_manager,
		MODELMANAGER & model_manager,
		std::vector<std::tr1::shared_ptr<MODEL> > & models,
		std::vector<TRACKOBJECT> & objects,
		std::ostream & info_output,
		std::ostream & error_output,
		const std::vector<TRACKSURFACE> & surfaces,
		const std::string & trackpath,
		const std::string & trackdir,
		const std::string & texsize,
		int anisotropy,
		bool vertical_tracking_skyboxes,
		bool dynamicshadowsenabled,
		bool agressivecombine,
		bool cull) :
		sceneroot(sceneroot),
		texture_manager(texture_manager),
		model_manager(model_manager),
		models(models),
		objects(objects),
		info_output(info_output),
		error_output(error_output),
		surfaces(surfaces),
		trackpath(trackpath),
		trackdir(trackdir),
		texsize(texsize),
		anisotropy(anisotropy),
		vertical_tracking_skyboxes(vertical_tracking_skyboxes),
		dynamicshadowsenabled(dynamicshadowsenabled),
		agressivecombine(agressivecombine),
		cull(cull),
		objectpath(trackpath + "/objects"),
		objectdir(trackdir +  "/objects"),
		packload(false),
		numobjects(0),
		params_per_object(17),
		expected_params(17),
		min_params(14),
		error(false),
		list(false)
	{
		// ctor
	}
	
	bool GetError() const
	{
		return error;
	}
	
	int GetNumObjects() const
	{
		return numobjects;
	}
	
	///returns false on error
	bool BeginObjectLoad();
	
	///returns a pair of bools: the first bool is true if there was an error, the second bool is true if an object was loaded
	std::pair <bool,bool> ContinueObjectLoad();
	
private:
	SCENENODE & sceneroot;
	TEXTUREMANAGER & texture_manager;
	MODELMANAGER & model_manager;
	std::vector<std::tr1::shared_ptr<MODEL> > & models;
	std::vector<TRACKOBJECT> & objects;
	std::ostream & info_output;
	std::ostream & error_output;
	
	const std::vector<TRACKSURFACE> & surfaces;
	const std::string & trackpath;
	const std::string & trackdir;
	const std::string & texsize;
	const int anisotropy;
	const bool vertical_tracking_skyboxes;
	const bool dynamicshadowsenabled;
	const bool agressivecombine;
	const bool cull;
	
	SCENENODE unoptimized_scene;
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
	
	CONFIG track_config;
	CONFIG::const_iterator track_it;
	
	/// pod for references
	struct BODY
	{
		BODY() : nolighting(false), collidable(true), surface(0) {}
		
		std::tr1::shared_ptr<MODEL> model;
		DRAWABLE drawable;
		bool nolighting;
		bool collidable;
		int surface;
	};
	std::map<std::string, BODY> bodies;
	
	typedef std::map<std::string, BODY>::const_iterator body_iterator;
	
	body_iterator LoadBody(const std::string & name);
	
	void AddBody(SCENENODE & scene, const BODY & body);
	
	bool LoadNode(const CONFIG & cfg, const CONFIG::const_iterator & sec);
	
	bool Begin();
	void CalculateNum();
	std::pair<bool, bool> Continue();
	
	bool BeginOld();
	void CalculateNumOld();
	std::pair<bool, bool> ContinueOld();
};

#endif // _OBJECTLOADER_H
