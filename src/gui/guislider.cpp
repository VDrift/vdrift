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
#include "graphics/texture.h"
#include <sstream>

GuiSlider::GuiSlider() :
	m_x(0), m_y(0), m_w(0), m_h(0),
	m_min_value(-0.02), m_max_value(0.02)
{
	set_value.call.bind<GuiSlider, &GuiSlider::SetValue>(this);
	set_min_value.call.bind<GuiSlider, &GuiSlider::SetMinValue>(this);
	set_max_value.call.bind<GuiSlider, &GuiSlider::SetMaxValue>(this);
}

GuiSlider::~GuiSlider()
{
	// dtor
}

void GuiSlider::SetValue(const std::string & valuestr)
{
	float value;
	std::istringstream s(valuestr);
	s >> value;

	float half_width = (m_max_value - m_min_value) * 0.5f;
	float center = m_min_value + half_width;
	if (value > center || value < center)
	{
		m_min_value = value - half_width;
		m_max_value = value + half_width;
		m_update = true;
	}
}

void GuiSlider::SetMinValue(const std::string & valuestr)
{
	float value;
	std::istringstream s(valuestr);
	s >> value;

	if (value > m_min_value || value < m_min_value)
	{
		m_min_value = value;
		m_update = true;
	}
}

void GuiSlider::SetMaxValue(const std::string & valuestr)
{
	float value;
	std::istringstream s(valuestr);
	s >> value;

	if (value > m_max_value || value < m_max_value)
	{
		m_max_value = value;
		m_update = true;
	}
}

Drawable & GuiSlider::GetDrawable(SceneNode & node)
{
	return node.GetDrawList().twodim.get(m_draw);
}

void GuiSlider::InitDrawable(
	SceneNode & node,
	const std::shared_ptr<Texture> & texture,
	float xywh[4],
	float z)
{
	m_x = xywh[0] - xywh[2] * 0.5f;
	m_y = xywh[1] - xywh[3] * 0.5f;
	m_w = xywh[2];
	m_h = xywh[3];

	m_texture = texture;

	assert(!m_draw.valid());
	m_draw = node.GetDrawList().twodim.insert(Drawable());

	Drawable & drawable = GetDrawable(node);
	drawable.SetTextures(m_texture->GetId());
	drawable.SetVertArray(&m_varray);
	drawable.SetDrawOrder(z);
	drawable.SetCull(false);
	drawable.SetColor(m_rgb[0], m_rgb[1], m_rgb[2], m_alpha);
}
