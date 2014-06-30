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

#ifndef _HUDGAUGE_H
#define _HUDGAUGE_H

#include "graphics/scenenode.h"
#include "graphics/vertexarray.h"
#include "memory.h"

class Font;
class Texture;

class HudGauge
{
public:
	HudGauge();

	// startangle and endangle in rad
	// startvalue + endvalue = n * valuedelta
	void Set(
		SceneNode & parent,
		const std::tr1::shared_ptr<Texture> & texture,
		const Font & font,
		float hwratio,
		float centerx,
		float centery,
		float radius,
		float startangle,
		float endangle,
		float startvalue,
		float endvalue,
		float valuedelta);

	void Revise(
		SceneNode & parent,
		const Font & font,
		float startvalue,
		float endvalue,
		float valuedelta);

	void Update(float value);

private:
	SceneNode::DrawableHandle pointer_draw;
	SceneNode::DrawableHandle dialnum_draw;
	SceneNode::DrawableHandle dial_draw;
	std::tr1::shared_ptr<Texture> texture;
	VertexArray pointer_rotated;
	VertexArray pointer;
	VertexArray dial_label;
	VertexArray dial_marks;
	float centerx;
	float centery;
	float scalex;
	float scaley;
	float startangle;
	float endangle;
	float scale;
};

#endif // _HUDGAUGE_H
