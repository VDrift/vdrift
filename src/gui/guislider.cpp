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

#include "guislider.h"

GuiSlider::GuiSlider() :
	m_value(0), m_x(0), m_y(0), m_w(0), m_h(0), m_fill(0)
{
	set_value.call.bind<GuiSlider, &GuiSlider::SetValue>(this);
}

GuiSlider::~GuiSlider()
{
	// dtor
}

void GuiSlider::Update(SceneNode & scene, float dt)
{
	if (m_update)
	{
		float x, w;
		if (m_fill > 0)
		{
			x = m_x;
			w = m_w * m_value;
		}
		else if (m_fill < 0)
		{
			x = m_x + m_w * m_value;
			w = m_w * (1 - m_value);
		}
		else
		{
			x = m_x + m_w * m_value - m_h * 0.1;
			w = m_h * 0.2;
		}
		m_slider.SetToBillboard(x, m_y, w, m_h);

		GuiWidget::Update(scene, dt);
	}
}

void GuiSlider::SetupDrawable(
	SceneNode & scene,
	std::shared_ptr<Texture> texture,
	float xywh[4], float z, int fill,
	std::ostream & /*error_output*/)
{
	m_value = 0;
	m_x = xywh[0] - xywh[2] * 0.5f;
	m_y = xywh[1] - xywh[3] * 0.5f;
	m_w = xywh[2];
	m_h = xywh[3];
	m_fill = fill;
	m_slider.Load(scene, texture, z);
}

void GuiSlider::SetValue(const std::string & valuestr)
{
	float value;
	std::istringstream s(valuestr);
	s >> value;
	if (value != m_value)
	{
		m_value = value;
		m_update = true;
	}
}

Drawable & GuiSlider::GetDrawable(SceneNode & scene)
{
	return m_slider.GetDrawable(scene);
}
