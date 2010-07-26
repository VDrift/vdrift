#include "contentmanager.h"

#include "texture.h"
#include "model.h"
#include "sound.h"

ContentManager::ContentManager(std::ostream & error) : 
	textures(error),
	models(error)
{
	// ctor
}

template<>
std::tr1::shared_ptr<TEXTURE> ContentManager::get<TEXTURE>(const ObjectLoader<TEXTURE> & loader)
{
	return textures.get(loader);
}

template<>
std::tr1::shared_ptr<MODEL> ContentManager::get<MODEL>(const ObjectLoader<MODEL> & loader)
{
	return models.get(loader);
}

void ContentManager::sweep()
{
	textures.sweep();
	models.sweep();
}

void ContentManager::clear()
{
	textures.clear();
	models.clear();
}

void ContentManager::debugPrint(std::ostream & error)
{
	textures.debugPrint(error);
	models.debugPrint(error);
}
