#ifndef _TEXTUREMANAGER_H
#define _TEXTUREMANAGER_H

#include "manager.h"

class TEXTURE;
class TEXTUREINFO;

class TEXTUREMANAGER : public MANAGER<TEXTURE>
{
public:
	TEXTUREMANAGER(std::ostream & error);
	
	bool Load(const std::string & path, const std::string & name, const TEXTUREINFO & info, std::tr1::shared_ptr<TEXTURE> & sptr);
};

#endif // _TEXTUREMANAGER_H