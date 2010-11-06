#ifndef _TEXTUREMANAGER_H
#define _TEXTUREMANAGER_H

#include "manager.h"
#include "texture.h"

class TEXTUREMANAGER : public MANAGER<TEXTURE>
{
public:
	TEXTUREMANAGER() {}
	
	bool Load(const std::string & name, const TEXTUREINFO & info, std::tr1::shared_ptr<TEXTURE> & sptr)
	{
		if (Get(name, sptr)) return true;
		
		std::tr1::shared_ptr<TEXTURE> temp(new TEXTURE());
		if (temp->Load(path + "/" + name, info, *error))
		{
			objects[name] = temp;
			sptr = temp;
			return true;
		}
		
		return false;
	}
};

#endif // _TEXTUREMANAGER_H