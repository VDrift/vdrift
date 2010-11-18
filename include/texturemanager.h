#ifndef _TEXTUREMANAGER_H
#define _TEXTUREMANAGER_H

#include "manager.h"
#include "texture.h"
#include <fstream>

class TEXTUREMANAGER : public MANAGER<TEXTURE>
{
public:
	TEXTUREMANAGER() {}
	
	bool Load(const std::string & name, const TEXTUREINFO & info, std::tr1::shared_ptr<TEXTURE> & sptr)
	{
		if (Get(name, sptr)) return true;
		
		std::string filepath = path + "/" + name;
		std::tr1::shared_ptr<TEXTURE> temp(new TEXTURE());
		if (std::ifstream(filepath.c_str()) && temp->Load(filepath, info, *error))
		{
			objects[name] = temp;
			sptr = temp;
			return true;
		}
		else
		{
			std::string filename = name;
			size_t n = name.rfind("/");
			if (n != std::string::npos) filename.erase(0, n + 1);
			if (temp->Load(sharedpath + "/" + filename, info, *error))
			{
				objects[filename] = temp;
				sptr = temp;
				return true;
			}
		}
		
		return false;
	}
};

#endif // _TEXTUREMANAGER_H