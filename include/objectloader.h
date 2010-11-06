#ifndef _OBJECTLOADER_H
#define _OBJECTLOADER_H

#include "scenenode.h"
#include "joepack.h"

#include <map> // for std::pair

class TEXTUREMANAGER;
class MODELMANAGER;
class TRACKSURFACE;
class TRACKOBJECT;

class OBJECTLOADER
{
public:
	OBJECTLOADER(
		SCENENODE & sceneroot,
		TEXTUREMANAGER & textures,
		MODELMANAGER & models,
		std::list<TRACKOBJECT> & objects,
		std::ostream & info_output,
		std::ostream & error_output,
		const std::vector<TRACKSURFACE> & surfaces,
		const std::string & trackpath,
		const std::string & trackdir,
		const std::string & texsize,
		const int anisotropy,
		const bool vertical_tracking_skyboxes,
		const bool dynamicshadowsenabled,
		const bool agressivecombine,
		const bool cull);

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
	TEXTUREMANAGER & textures;
	MODELMANAGER & models;
	std::list<TRACKOBJECT> & objects;
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
	
	void CalculateNumObjects();
	
	///read from the file stream and put it in "output".
	/// return true if the get was successful, else false
	template <typename T>
	bool GetParam(std::ifstream & f, T & output)
	{
		if (!f.good())
			return false;

		std::string instr;
		f >> instr;
		if (instr.empty())
			return false;

		while (!instr.empty() && instr[0] == '#' && f.good())
		{
			f.ignore(1024, '\n');
			f >> instr;
		}

		if (!f.good() && !instr.empty() && instr[0] == '#')
			return false;

		std::stringstream sstr(instr);
		sstr >> output;
		return true;
	}
};

#endif // _OBJECTLOADER_H
