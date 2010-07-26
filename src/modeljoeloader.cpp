#include "modeljoeloader.h"

#include "model_joe03.h"

MODEL * ModelJoeLoader::load(std::ostream & error) const
{
	MODEL_JOE03 * model = new MODEL_JOE03();
	if (model->Load(name, pack, error, genlist))
	{
		return model;
	}
	delete(model);
	return NULL;
}

const std::string & ModelJoeLoader::id() const
{
	return name;
}

