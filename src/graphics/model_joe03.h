/************************************************************************/
/*                                                                      */
/* This file is part of VDrift.                                         */
/*                                                                      */
/* VDrift is free software: you can redistribute it and/or modify       */
/* it under the terms of the GNU General Public License as published by */
/* the Free Software Foundation, either version 3 of the License, or    */
/* (at your option) any later version.                                  */
/*                                                                      */
/* VDrift is distributed in the hope that it will be useful,            */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of       */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the        */
/* GNU General Public License for more details.                         */
/*                                                                      */
/* You should have received a copy of the GNU General Public License    */
/* along with VDrift.  If not, see <http://www.gnu.org/licenses/>.      */
/*                                                                      */
/************************************************************************/

#ifndef _MODEL_JOE03_H
#define _MODEL_JOE03_H

#include "model.h"

#include <iosfwd>
#include <string>

class JoePack;
struct JoeObject;

// This class handles all of the loading code
class ModelJoe03 : public Model
{
public:
	virtual ~ModelJoe03()
	{
		Clear();
	}

	virtual bool Load(const std::string & strFileName, std::ostream & error_output)
	{
		return Load(strFileName, error_output, 0);
	}

	virtual bool CanSave() const
	{
		return false;
	}

	bool Load(const std::string & strFileName, std::ostream & error_output, const JoePack * pack);

	static const unsigned int JOE_MAX_FACES;
	static const unsigned int JOE_VERSION;

private:
	// This reads in the data from the MD2 file and stores it in the member variable
	void ReadData(FILE * m_FilePointer, const JoePack * pack, JoeObject & Object);

	bool LoadFromHandle(FILE * f, const JoePack * pack, std::ostream & error_output);
};

#endif
