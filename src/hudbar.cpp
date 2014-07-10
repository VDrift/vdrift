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
#include "graphics/scenenode.h"
#include "graphics/texture.h"

void HudBar::Set(
	SceneNode & parent,
	std::tr1::shared_ptr<Texture> & tex,
	float x, float y, float w, float h,
	float opacity,
	bool flip)
{
	draw = parent.GetDrawList().twodim.insert(Drawable());
	Drawable & drawref = parent.GetDrawList().twodim.get(draw);

	drawref.SetTextures(tex->GetId());
	drawref.SetVertArray(&verts);
	drawref.SetCull(false);
	drawref.SetColor(1,1,1,opacity);
	drawref.SetDrawOrder(1);

	texture = tex;

	verts.SetTo2DButton(x, y, w, h, h*0.75, flip);
}

void HudBar::SetVisible(SceneNode & parent, bool newvis)
{
	Drawable & drawref = parent.GetDrawList().twodim.get(draw);
	drawref.SetDrawEnable(newvis);
}
