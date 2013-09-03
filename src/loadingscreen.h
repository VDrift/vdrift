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

#ifndef _LOADINGSCREEN_H
#define _LOADINGSCREEN_H

#include "gui/text_draw.h"
#include <string>

class ContentManager;

class LoadingScreen
{
public:
	SceneNode & GetNode() {return root;}

	void Update(float percentage, const std::string & optional_text, float posx, float posy);

	///initialize the loading screen given the root node for the loading screen
	bool Init(
		const std::string & texturepath,
		int displayw,
		int displayh,
		ContentManager & content,
		Font & font);

private:
	SceneNode root;
	keyed_container <Drawable>::handle bardraw;
	VertexArray barverts;
	keyed_container <Drawable>::handle barbackdraw;
	VertexArray barbackverts;
	keyed_container <Drawable>::handle boxdraw;
	VertexArray boxverts;
	float w, h, hscale;
	TextDrawable text;
};

#endif
