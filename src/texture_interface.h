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

#ifndef _TEXTURE_INTERFACE_H
#define _TEXTURE_INTERFACE_H

#include "glew.h"

/// an abstract base class for a simple texture interface
class TEXTURE_INTERFACE
{
public:
	virtual bool Loaded() const = 0;
	virtual void Activate() const = 0;
	virtual void Deactivate() const = 0;
	virtual bool IsRect() const {return false;}
	virtual unsigned int GetW() const = 0;
	virtual unsigned int GetH() const = 0;
	virtual GLuint GetID() const = 0;
};

#endif // _TEXTURE_INTERFACE_H
