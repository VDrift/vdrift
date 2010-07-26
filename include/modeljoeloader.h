#ifndef _MODELJOELOADER_H
#define _MODELJOELOADER_H

#include "objectloader.h"

#include <string>

class MODEL;
class JOEPACK;

class ModelJoeLoader : public ObjectLoader<MODEL>
{
public:
	std::string name;
	JOEPACK * pack;
	bool genlist;
	
	ModelJoeLoader() : pack(NULL), genlist(true) {};
	virtual ~ModelJoeLoader() {};
	virtual MODEL * load(std::ostream & error) const;
	virtual const std::string & id() const;
};

#endif // _MODELJOELOADER_H
