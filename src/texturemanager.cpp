#include "texturemanager.h"
#include "texture.h"
#include <fstream>

TEXTUREMANAGER::TEXTUREMANAGER(std::ostream & error) :
	MANAGER<TEXTURE>(error), srgb(false)
{
	// ctor
}

bool TEXTUREMANAGER::Load(const std::string & path, const std::string & name, const TEXTUREINFO & originalinfo, std::tr1::shared_ptr<TEXTURE> & sptr)
{
	const bool debug = true;
	
	if (Get(path, name, sptr)) return true;

	// override some parameters in the info based on the manager's configuration
	TEXTUREINFO info = originalinfo;
	info.srgb = srgb;

	std::tr1::shared_ptr<TEXTURE> temp(new TEXTURE());
	
	for (std::vector <PATH>::const_iterator p = basepaths.begin(); p != basepaths.end(); p++)
	{
		std::string filepath = p->path + "/" + path + "/" + name;
		if (p->shared)
			filepath = p->path + "/" + name;
		if ((info.surface || std::ifstream(filepath.c_str())) && temp->Load(filepath, info, error))
		{
			sptr = Set(p->GetAssetPath(path, name), temp)->second;
			return true;
		}
	}
	
	if (debug)
	{
		std::cerr << "tried paths: ";
		for (std::vector <PATH>::const_iterator p = basepaths.begin(); p != basepaths.end(); p++)
		{
			if (p != basepaths.begin())
				std::cerr << ", ";
			std::cerr << p->path + "/" + path + "/" + name;
		}
		std::cerr << std::endl;
	}

	return false;
}