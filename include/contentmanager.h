#ifndef _CONTENTMANAGER_H
#define _CONTENTMANAGER_H

#include "objectmanager.h"

class TEXTURE;
class MODEL;

class ContentManager
{
public:
	ContentManager(std::ostream & error);
	
	template <typename T>
	std::tr1::shared_ptr<T> get(const ObjectLoader<T> & loader);
	
	void sweep();
	
	void clear();
	
	void debugPrint(std::ostream & error);
	
private:
	ObjectManager<TEXTURE> textures;
	ObjectManager<MODEL> models;
};

#endif // _CONTENTMANAGER_H
