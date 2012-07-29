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

#include "hudbar.h"
#include "scenenode.h"

void HUDBAR::Set(
	SCENENODE & parent,
	std::tr1::shared_ptr<TEXTURE> bartex,
	float x, float y, float w, float h,
	float opacity,
	bool flip)
{
	draw = parent.GetDrawlist().twodim.insert(DRAWABLE());
	DRAWABLE & drawref = parent.GetDrawlist().twodim.get(draw);

	drawref.SetDiffuseMap(bartex);
	drawref.SetVertArray(&verts);
	drawref.SetCull(false, false);
	drawref.SetColor(1,1,1,opacity);
	drawref.SetDrawOrder(1);

	verts.SetTo2DButton(x, y, w, h, h*0.75, flip);
}

void HUDBAR::SetVisible(SCENENODE & parent, bool newvis)
{
	DRAWABLE & drawref = parent.GetDrawlist().twodim.get(draw);
	drawref.SetDrawEnable(newvis);
}
