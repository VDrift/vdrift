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

#include "guiradialslider.h"

GuiRadialSlider::GuiRadialSlider() :
	m_start_angle(0),
	m_end_angle(0),
	m_radius(0),
	m_dar(1)
{
	// ctor
}

GuiRadialSlider::~GuiRadialSlider()
{
	// dtor
}

void GuiRadialSlider::Update(SceneNode & scene, float dt)
{
	if (m_update)
	{
		UpdateVertexArray();
		GuiWidget::Update(scene, dt);
	}
}

void GuiRadialSlider::SetupDrawable(
	SceneNode & node,
	const std::shared_ptr<Texture> & texture,
	float xywh[4], float z,
	float start_angle, float end_angle,
	float radius, float dar,
	std::ostream & /*error_output*/)
{
	m_start_angle = start_angle;
	m_end_angle = end_angle;
	m_radius = radius;
	m_dar = dar;

	InitDrawable(node, texture, xywh, z);
	UpdateVertexArray();
}

void GuiRadialSlider::UpdateVertexArray()
{
	const float deg2rad = M_PI / 180;
	float anchor[2] = {m_x + m_w * 0.5f, m_y + m_h * 0.5f};
	float r1 = m_radius * 2.0f - m_h * 0.5f;
	float r2 = m_h * 0.5f;

	if (set_value.connected()) // fixme: this is kinda sketchy
	{
		float half_width = (m_max_value - m_min_value) * 0.5f;
		float x1 = -m_h * half_width;
		float x2 = m_h * half_width;
		float value = (m_min_value + m_max_value) * 0.5f;
		float angle = m_start_angle * (1.0f - value) + m_end_angle * value;

		m_varray.SetToBillboard(x1, -r2, x2, -r1);
		m_varray.Rotate(angle * deg2rad, 0.0f, 0.0f, 1.0f);
	}
	else
	{
		float angle_range = m_end_angle - m_start_angle;
		float slider_range = m_max_value - m_min_value;
		float a1 = m_start_angle + angle_range * m_min_value - 90;
		float a2 = m_start_angle + angle_range * m_max_value - 90;
		unsigned n = std::abs(m_radius * angle_range * slider_range) + 1;

		m_varray.SetTo2DRing(r1, r2, a1 * deg2rad, a2 * deg2rad, n);
	}

	m_varray.Scale(m_dar, 1.0f, 1.0f);
	m_varray.Translate(anchor[0], anchor[1], 0.0f);
}
