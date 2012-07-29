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

#include "scenenode.h"
#include "vertexarray.h"
#include "text_draw.h"

#include <ostream>
#include <string>

class ContentManager;

class LOADINGSCREEN
{
public:
	SCENENODE & GetNode() {return root;}

	void Update(float percentage, const std::string & optional_text, float posx, float posy);

	///initialize the loading screen given the root node for the loading screen
	bool Init(
		const std::string & texturepath,
		int displayw,
		int displayh,
		ContentManager & content,
		FONT & font);

private:
	SCENENODE root;
	keyed_container <DRAWABLE>::handle bardraw;
	VERTEXARRAY barverts;
	keyed_container <DRAWABLE>::handle barbackdraw;
	VERTEXARRAY barbackverts;
	keyed_container <DRAWABLE>::handle boxdraw;
	VERTEXARRAY boxverts;
	float w, h, hscale;
	TEXT_DRAWABLE text;
};

#endif
