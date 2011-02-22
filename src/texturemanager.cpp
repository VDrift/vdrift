#include "texturemanager.h"
#include "texture.h"
#include <fstream>

TEXTUREMANAGER::TEXTUREMANAGER(std::ostream & error) : 
	MANAGER<TEXTURE>(error)
{
	// ctor
}

bool TEXTUREMANAGER::Load(const std::string & path, const std::string & name, const TEXTUREINFO & info, std::tr1::shared_ptr<TEXTURE> & sptr)
{
	if (Get(path, name, sptr)) return true;
	
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