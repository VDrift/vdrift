#ifndef _TEXTUREMANAGER_H
#define _TEXTUREMANAGER_H

#include "manager.h"
#include "texture.h"
#include <fstream>

class TEXTUREMANAGER : public MANAGER<TEXTURE>
{
public:
	TEXTUREMANAGER() {}
	
	bool Load(const std::string & path, const std::string & name, const TEXTUREINFO & info, std::tr1::shared_ptr<TEXTURE> & sptr)
	{
		if (Get(path, name, sptr)) return true;
		
		std::string filepath = basepath + "/" + path + "/" + name;
		std::tr1::shared_ptr<TEXTURE> temp(new TEXTURE());
		if (std::ifstream(filepath.c_str()) && temp->Load(filepath, info, *error))
		{
			sptr = Set(path + "/" + name, temp)->second;
			return true;
		}
		else
		{
			if (temp->Load(sharedpath + "/" + name, info, *error))
			{
				sptr = Set(name, temp)->second;
				return true;
			}
		}
		
		return false;
	}
};

#endif // _TEXTUREMANAGER_H