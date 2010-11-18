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
		
		// prefer ogg
		std::string filepath = path + "/" + name + ".ogg";
		if (!std::ifstream(filepath.c_str()))
		{
			filepath = path + "/" + name + ".wav";
		}
		std::tr1::shared_ptr<SOUNDBUFFER> temp(new SOUNDBUFFER());
		if (std::ifstream(filepath.c_str()) && temp->Load(filepath, info, *error))
		{
			objects[name] = temp;
			sptr = temp;
			return true;
		}
		else
		{
			size_t n = filepath.rfind("/");
			if (n != std::string::npos) filepath.erase(0, n + 1);
			if (temp->Load(sharedpath + "/" + filepath, info, *error))
			{
				objects[filepath] = temp;
				sptr = temp;
				return true;
			}
		}
		
		return false;
	}
};

#endif // _SOUNDMANAGER_H
