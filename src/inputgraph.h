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

#ifndef _INPUTGRAPH_H
#define _INPUTGRAPH_H

#include "sprite2d.h"
#include "graphics/scenenode.h"

#include <string>

class InputGraph
{
private:
	SceneNode graphroot;
	Sprite2D vslider;
	Sprite2D hslider;
	Sprite2D vball;
	Sprite2D hball;
	float hwidth;
	float vheight;

	void SetVisible(bool newvis)
	{
		graphroot.SetChildVisibility(newvis);
	}
public:
	bool Init(const std::string & texturepath, ContentManager & content)
	{
		float hheight, hball_scale, hx_pos, hy_pos;
		float vwidth, vx_pos, vy_pos;

		hwidth = 0.2;
		hheight = 0.2*1.33333;
		hx_pos = 0.5-hwidth/2;
		hy_pos = 0.8;
		hball_scale = 4.0;

		if (!hslider.Load(graphroot, texturepath, "slider2.png", content, 1))
			return false;
		hslider.SetToBillboard(hx_pos, hy_pos, hwidth, hheight);
		if (!hball.Load(graphroot, texturepath, "ball2.png", content, 2))
			return false;
		hball.SetToBillboard(hx_pos+hwidth/2-hwidth/(2.0*hball_scale), hy_pos+hheight/2-hheight/(2*hball_scale), hwidth/hball_scale, hheight/hball_scale);

		vwidth = hwidth/2;
		vheight = hheight/2;
		vx_pos = 0.35-vwidth/2;
		vy_pos = 0.87;
		if (!vslider.Load(graphroot, texturepath, "accdec-slider.png", content, 1))
			return false;
		vslider.SetToBillboard(vx_pos, vy_pos, vwidth, vheight);
		if (!vball.Load(graphroot, texturepath, "accdec-marker.png", content, 2))
			return false;
		vball.SetToBillboard(vx_pos, vy_pos, vwidth, vheight);
		return true;
	}

	void Update(const std::vector <float> & inputs)
	{
		const float throttle = inputs[CarInput::THROTTLE]-inputs[CarInput::BRAKE];
		float steer_value = inputs[CarInput::STEER_RIGHT];
		Vec3 translation;

		if (std::abs(inputs[CarInput::STEER_LEFT]) > std::abs(inputs[CarInput::STEER_RIGHT])) //use whichever control is larger
			steer_value = -inputs[CarInput::STEER_LEFT];

		translation = hball.GetTransform(graphroot).GetTranslation();
		translation[0] = steer_value*hwidth/2.7;
		hball.GetTransform(graphroot).SetTranslation(translation);
		translation = vball.GetTransform(graphroot).GetTranslation();
		translation[1] = -throttle*vheight/2.7;
		vball.GetTransform(graphroot).SetTranslation(translation);
	}

	void Hide()
	{
		SetVisible(false);
	}

	void Show()
	{
		SetVisible(true);
	}

	SceneNode & GetNode() {return graphroot;}
};

#endif
