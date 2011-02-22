#include "soundmanager.h"
#include "soundbuffer.h"
#include <fstream>

SOUNDMANAGER::SOUNDMANAGER(std::ostream & error) :
	MANAGER<SOUNDBUFFER>(error)
{
	// ctor
}

bool SOUNDMANAGER::Load(
	const std::string & path,
	const std::string & name,
	const SOUNDINFO & info,
	std::tr1::shared_ptr<SOUNDBUFFER> & sptr)
{
	if (Get(path, name, sptr)) return true;
	
	// prefer ogg
	std::string filepath = basepath + "/" + path + "/" +  name + ".ogg";
	if (!std::ifstream(filepath.c_str()))
	{
		filepath = basepath + "/" + path + "/" + name + ".wav";
	}
	
	std::tr1::shared_ptr<SOUNDBUFFER> temp(new SOUNDBUFFER());
	if (std::ifstream(filepath.c_str()) && temp->Load(filepath, info, error))
	{
		sptr = Set(path + "/" + name, temp)->second;
		return true;
	}
	else
	{
		// prefer ogg
		std::string filepath = sharedpath + "/" + name + ".ogg";
		if (!std::ifstream(filepath.c_str()))
		{
			filepath = sharedpath + "/" + name + ".wav";
		}
		
		if (temp->Load(filepath, info, error))
		{
			sptr = Set(name, temp)->second;
			return true;
		}
	}
	
	return false;
}