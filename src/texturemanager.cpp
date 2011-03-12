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
	if (Get(path, name, sptr)) return true;
	
	// override some parameters in the info based on the manager's configuration
	TEXTUREINFO info = originalinfo;
	info.srgb = srgb;
	
	std::string filepath = basepath + "/" + path + "/" + name;
	std::tr1::shared_ptr<TEXTURE> temp(new TEXTURE());
	if (std::ifstream(filepath.c_str()) && temp->Load(filepath, info, error))
	{
		sptr = Set(path + "/" + name, temp)->second;
		return true;
	}
	else
	{
		if (temp->Load(sharedpath + "/" + name, info, error))
		{
			sptr = Set(name, temp)->second;
			return true;
		}
	}
	
	return false;
}