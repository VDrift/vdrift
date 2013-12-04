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

class TextureInterface
{
public:
	TextureInterface() : target(0), texid(0), width(0), height(0) { }
	virtual ~TextureInterface() { };
	unsigned GetTarget() const { return target; }
	unsigned GetId() const { return texid; }
	unsigned GetW() const { return width; }
	unsigned GetH() const { return height; }

protected:
	unsigned target;
	unsigned texid;
	unsigned width;
	unsigned height;
};

#endif // _TEXTURE_INTERFACE_H
