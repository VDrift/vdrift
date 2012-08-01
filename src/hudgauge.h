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

class FONT;

class HUDGAUGE
{
public:
	HUDGAUGE();

	// startangle and endangle in rad
	// startvalue + endvalue = n * valuedelta
	void Set(
		SCENENODE & parent,
		const std::tr1::shared_ptr<TEXTURE> & texture,
		const FONT & font,
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
		SCENENODE & parent,
		const FONT & font,
		float startvalue,
		float endvalue,
		float valuedelta);

	void Update(float value);

private:
	keyed_container<DRAWABLE>::handle pointer_draw;
	keyed_container<DRAWABLE>::handle dialnum_draw;
	keyed_container<DRAWABLE>::handle dial_draw;
	std::tr1::shared_ptr<TEXTURE> texture;
	VERTEXARRAY pointer_rotated;
	VERTEXARRAY pointer;
	VERTEXARRAY dial_label;
	VERTEXARRAY dial_marks;
	float centerx;
	float centery;
	float scalex;
	float scaley;
	float startangle;
	float endangle;
	float scale;
};

#endif // _HUDGAUGE_H
