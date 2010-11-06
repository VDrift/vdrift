#ifndef _MODEL_JOE03_H
#define _MODEL_JOE03_H

#include "model.h"

#include <string>
#include <ostream>

class JOEPACK;
class JOEObject;

// This class handles all of the loading code
class MODEL_JOE03 : public MODEL
{
public:
	virtual ~MODEL_JOE03()
	{
		Clear();
	}

	virtual bool Load(const std::string & strFileName, std::ostream & error_output, bool genlist=true)
	{
		return Load(strFileName, NULL, error_output, genlist);
	}
	
	virtual bool CanSave() const
	{
		return false;
	}
	
	bool Load(std::string strFileName, JOEPACK * pack, std::ostream & error_output, bool genlist=true);
	
	bool LoadFromHandle(FILE * f, JOEPACK * pack, std::ostream & error_output);

private:
	static const int JOE_MAX_FACES;
	static const int JOE_VERSION;
	static const float MODEL_SCALE;
	
	std::string modelpath;
	std::string modelname;
	
	int BinaryRead(void * buffer, unsigned int size, unsigned int count, FILE * f, JOEPACK * pack);

	void ReadData(FILE * m_FilePointer, JOEPACK * pack, JOEObject * pObject); // This reads in the data from the MD2 file and stores it in the member variable
	
	bool NeedsNormalSwap(JOEObject * pObject);
};

#endif
