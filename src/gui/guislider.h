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

#ifndef _GUISLIDER_H
#define _GUISLIDER_H

#include "guiwidget.h"
#include "sprite2d.h"

class GuiSlider : public GuiWidget
{
public:
	GuiSlider();

	~GuiSlider();

	virtual void Update(SceneNode & scene, float dt);

	void SetupDrawable(
		SceneNode & scene,
		std::shared_ptr<Texture> texture,
		float xywh[4], float z, int fill,
  		std::ostream & error_output);

	Slot1<const std::string &> set_value;

private:
	Sprite2D m_slider;
	float m_value, m_x, m_y, m_w, m_h;
	int m_fill;

	void SetValue(const std::string & value);
	Drawable & GetDrawable(SceneNode & scene);
	GuiSlider(const GuiSlider & other);
};

#endif
