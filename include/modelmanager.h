#ifndef _MODELMANAGER_H
#define _MODELMANAGER_H

#include "manager.h"
#include "model_joe03.h"

class MODELMANAGER: public MANAGER<MODEL_JOE03>
{
public:
	bool Load(const std::string & name, std::tr1::shared_ptr<MODEL_JOE03> & sptr)
	{
		const_iterator it;
		if (Load(name, it))
		{
			sptr = it->second;
			return true;
		}
		return false;
	}
	
	bool Load(const std::string & name, const_iterator & it)
	{
		if (Get(name, it)) return true;
		
		std::string filepath = path + "/" + name;
		std::tr1::shared_ptr<MODEL_JOE03> temp(new MODEL_JOE03());
		if (std::ifstream(filepath.c_str()) && temp->Load(filepath, *error))
		{
			it = Set(name, temp);
			return true;
		}
		else
		{
			std::string fname = name;
			size_t n = fname.rfind("/");
			if (n != std::string::npos) fname.erase(0, n + 1);
			filepath = sharedpath + "/" + fname;
			if (temp->Load(filepath, *error))
			{
				it = Set(fname, temp);
				return true;
			}
		}
		return false;
	}
	
	bool Load(const std::string & name, std::tr1::shared_ptr<MODEL_JOE03> & sptr, JOEPACK * pack)
	{
		if (Get(name, sptr)) return true;
		
		std::tr1::shared_ptr<MODEL_JOE03> temp(new MODEL_JOE03());
		if (temp->Load(name, *error, true, pack))
		{
			objects[name] = temp;
			sptr = temp;
			return true;
		}
		
		return false;
	}
};

#endif // _MODELMANAGER_H
