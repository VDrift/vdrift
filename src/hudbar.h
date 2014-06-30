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
#include "graphics/scenenode.h"
#include "memory.h"

class Texture;

class HudBar
{
public:
	void Set(
		SceneNode & parent,
		std::tr1::shared_ptr<Texture> & tex,
		float x, float y, float w, float h,
		float opacity,
		bool flip);

	void SetVisible(SceneNode & parent, bool newvis);

private:
	SceneNode::DrawableHandle draw;
	std::tr1::shared_ptr<Texture> texture;
	VertexArray verts;
};

#endif // _HUDBAR_H
