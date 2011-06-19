#ifndef _MODELMANAGER_H
#define _MODELMANAGER_H

#include "manager.h"

class MODEL;
class JOEPACK;

class MODELMANAGER: public MANAGER<MODEL>
{
public:
	bool Load(const std::string & path, const std::string & name, std::tr1::shared_ptr<MODEL> & sptr);

	bool Load(const std::string & path, const std::string & name, std::tr1::shared_ptr<MODEL> & sptr, JOEPACK & pack);

	bool Load(const std::string & path, const std::string & name, const_iterator & it);

	void setGenerateDrawList(bool genlist) {loadToDrawlists = genlist;}

	bool useDrawlists() const {return loadToDrawlists;}

	MODELMANAGER(std::ostream & error);

private:
	bool loadToDrawlists; ///< if false, load to vertex array/buffer objects
};

#endif // _MODELMANAGER_H
