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

#ifndef _HUDBAR_H
#define _HUDBAR_H

#include "graphics/vertexarray.h"
#include "graphics/drawable.h"

class SCENENODE;
class TEXTURE;

class HUDBAR
{
public:
	void Set(
		SCENENODE & parent,
		std::tr1::shared_ptr<TEXTURE> bartex,
		float x, float y, float w, float h,
		float opacity,
		bool flip);

	void SetVisible(SCENENODE & parent, bool newvis);

private:
	keyed_container<DRAWABLE>::handle draw;
	VERTEXARRAY verts;
};

#endif // _HUDBAR_H
