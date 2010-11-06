#ifndef _SOUNDMANAGER_H
#define _SOUNDMANAGER_H

#include "manager.h"
#include "soundbuffer.h"

class SOUNDMANAGER : public MANAGER<SOUNDBUFFER>
{
public:
	SOUNDMANAGER() {}
	
	bool Load(const std::string & name, const SOUNDINFO & info, std::tr1::shared_ptr<SOUNDBUFFER> & sptr)
	{
		if (Get(name, sptr)) return true;
		
		//prefer ogg
		std::string filename = path+"/"+name+".ogg";
		if (!std::ifstream(filename.c_str()))
		{
			filename = path+"/"+name+".wav";
		}
		
		std::tr1::shared_ptr<SOUNDBUFFER> temp(new SOUNDBUFFER());
		if (temp->Load(filename, info, *error))
		{
			objects[name] = temp;
			sptr = temp;
			return true;
		}
		
		return false;
	}
};

#endif // _SOUNDMANAGER_H
