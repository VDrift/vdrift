#ifndef _MODELMANAGER_H
#define _MODELMANAGER_H

#include "manager.h"
#include "model_joe03.h"

class MODELMANAGER: public MANAGER<MODEL_JOE03>
{
public:
	bool Load(const std::string & path, const std::string & name, std::tr1::shared_ptr<MODEL_JOE03> & sptr)
	{
		const_iterator it;
		if (Load(path, name, it))
		{
			sptr = it->second;
			return true;
		}
		return false;
	}
	
	bool Load(const std::string & path, const std::string & name, const_iterator & it)
	{
		if (Get(path, name, it)) return true;
		
		std::string filepath = basepath + "/" + path + "/" + name;
		std::tr1::shared_ptr<MODEL_JOE03> temp(new MODEL_JOE03());
		if (std::ifstream(filepath.c_str()) && temp->Load(filepath, *error, useDrawlists()))
		{
			it = Set(path + "/" + name, temp);
			return true;
		}
		else
		{
			filepath = sharedpath + "/" + name;
			if (temp->Load(filepath, *error, useDrawlists()))
			{
				it = Set(name, temp);
				return true;
			}
		}
		return false;
	}
	
	bool Load(const std::string & name, std::tr1::shared_ptr<MODEL_JOE03> & sptr, JOEPACK * pack)
	{
		if (Get("", name, sptr)) return true;
		
		std::tr1::shared_ptr<MODEL_JOE03> temp(new MODEL_JOE03());
		if (temp->Load(name, *error, useDrawlists(), pack))
		{
			objects[name] = temp;
			sptr = temp;
			return true;
		}
		
		return false;
	}
	
	void setGenerateDrawList(bool genlist) {loadToDrawlists = genlist;}
	bool useDrawlists() const {return loadToDrawlists;}
	
	MODELMANAGER() : loadToDrawlists(true) {}
	
private:
	bool loadToDrawlists; ///< if false, load to vertex array/buffer objects
};

#endif // _MODELMANAGER_H
