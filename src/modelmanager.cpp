#include "modelmanager.h"
#include "model_joe03.h"

MODELMANAGER::MODELMANAGER(std::ostream & error) : 
	MANAGER<MODEL>(error), loadToDrawlists(true)
{
	// ctor
}

bool MODELMANAGER::Load(
	const std::string & path,
	const std::string & name,
	std::tr1::shared_ptr<MODEL> & sptr)
{
	const_iterator it;
	if (Load(path, name, it))
	{
		sptr = it->second;
		return true;
	}
	return false;
}

bool MODELMANAGER::Load(
	const std::string & path,
	const std::string & name,
	std::tr1::shared_ptr<MODEL> & sptr,
	JOEPACK & pack)
{
	if (Get("", name, sptr)) return true;
	
	MODEL_JOE03 * model = new MODEL_JOE03();
	if (model->Load(name, error, useDrawlists(), &pack))
	{
		std::tr1::shared_ptr<MODEL> temp(model);
		objects[name] = temp;
		sptr = temp;
		return true;
	}
	delete model;
	
	if (Load(path, name, sptr))
	{
		return true;
	}
	
	return false;
}

bool MODELMANAGER::Load(
	const std::string & path,
	const std::string & name,
	const_iterator & it)
{
	if (Get(path, name, it)) return true;
	
	std::string filepath = basepath + "/" + path + "/" + name;
	MODEL_JOE03 * model = new MODEL_JOE03();
	if (std::ifstream(filepath.c_str()) &&
		model->Load(filepath, error, useDrawlists()))
	{
		std::tr1::shared_ptr<MODEL> temp(model);
		it = Set(path + "/" + name, temp);
		return true;
	}
	else
	{
		filepath = sharedpath + "/" + name;
		if (model->Load(filepath, error, useDrawlists()))
		{
			std::tr1::shared_ptr<MODEL> temp(model);
			it = Set(name, temp);
			return true;
		}
	}
	delete model;
	return false;
}
