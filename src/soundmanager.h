#ifndef _SOUNDMANAGER_H
#define _SOUNDMANAGER_H

#include "manager.h"

class SOUNDBUFFER;
struct SOUNDINFO;

class SOUNDMANAGER : public MANAGER<SOUNDBUFFER>
{
public:
	SOUNDMANAGER(std::ostream & error);

	bool Load(const std::string & path, const std::string & name, const SOUNDINFO & info, std::tr1::shared_ptr<SOUNDBUFFER> & sptr);
};

#endif // _SOUNDMANAGER_H
