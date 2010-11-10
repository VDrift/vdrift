#ifndef _MODELMANAGER_H
#define _MODELMANAGER_H

#include "manager.h"
#include "model_joe03.h"

class MODELMANAGER: public MANAGER<MODEL_JOE03>
{
public:
	bool Load(const std::string & name, std::tr1::shared_ptr<MODEL_JOE03> & sptr)
	{
		if (Get(name, sptr)) return true;
		
		std::tr1::shared_ptr<MODEL_JOE03> temp(new MODEL_JOE03());
		if (temp->Load(path + "/" + name, *error))
		{
			objects[name] = temp;
			sptr = temp;
			return true;
		}
		
		return false;
	}
	
	bool Load(const std::string & name, std::tr1::shared_ptr<MODEL_JOE03> & sptr, JOEPACK * pack)
	{
		if (Get(name, sptr)) return true;
		
		std::tr1::shared_ptr<MODEL_JOE03> temp(new MODEL_JOE03());
		if (temp->Load(name, pack, *error))
		{
			objects[name] = temp;
			sptr = temp;
			return true;
		}
		
		return false;
	}
};

#endif // _MODELMANAGER_H
