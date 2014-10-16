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

#ifndef _MODEL_OBJ_H
#define _MODEL_OBJ_H

#include "model.h"

#include <iosfwd>
#include <string>

class ModelObj : public Model
{
private:

public:
	ModelObj(const std::string & filepath, std::ostream & error_output) : Model(filepath, error_output) {}

	///returns true on success
	virtual bool Load(const std::string & filepath, std::ostream & error_log);
	virtual bool CanSave() const {return true;}
	virtual bool Save(const std::string & strFileName, std::ostream & error_output) const;
};

#endif
