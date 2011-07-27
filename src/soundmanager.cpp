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

	for (std::vector <PATH>::const_iterator p = basepaths.begin(); p != basepaths.end(); p++)
	{
		// prefer ogg
		std::string filepath = p->path + "/" + path + "/" +  name + ".ogg";
		if (p->shared)
			filepath = p->path + "/" + name + ".ogg";
		if (!std::ifstream(filepath.c_str()))
		{
			filepath = p->path + "/" + path + "/" + name + ".wav";
			if (p->shared)
				filepath = p->path + "/" + name + ".wav";
		}

		std::tr1::shared_ptr<SOUNDBUFFER> temp(new SOUNDBUFFER());
		if (std::ifstream(filepath.c_str()) && temp->Load(filepath, info, error))
		{
			sptr = Set(p->GetAssetPath(path, name), temp)->second;
			return true;
		}
	}

	return false;
}