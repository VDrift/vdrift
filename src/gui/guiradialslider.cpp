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
#include "graphics/texture.h"

GuiRadialSlider::GuiRadialSlider() :
	m_x(0), m_y(0), m_w(0), m_h(0),
	m_start_angle(0), m_end_angle(0),
	m_radius(0),
	m_value(0),
	m_dar(1),
	m_fill(0)
{
	set_value.call.bind<GuiRadialSlider, &GuiRadialSlider::SetValue>(this);
}

GuiRadialSlider::~GuiRadialSlider()
{
	//dtor
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
	float start_angle, float end_angle, float radius,
	float dar, int fill, std::ostream & /*error_output*/)
{
	m_texture = texture;
	m_x = xywh[0] - xywh[2] * 0.5f;
	m_y = xywh[1] - xywh[3] * 0.5f;
	m_w = xywh[2];
	m_h = xywh[3];
	m_start_angle = start_angle;
	m_end_angle = end_angle;
	m_radius = radius;
	m_dar = dar;
	m_fill = fill;

	InitDrawable(node, z);
	UpdateVertexArray();
}

void GuiRadialSlider::SetValue(const std::string & valuestr)
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

Drawable & GuiRadialSlider::GetDrawable(SceneNode & node)
{
	return node.GetDrawList().twodim.get(m_draw);
}

void GuiRadialSlider::InitDrawable(SceneNode & node, float draworder)
{
	assert(!m_draw.valid());
	m_draw = node.GetDrawList().twodim.insert(Drawable());

	Drawable & drawable = GetDrawable(node);
	drawable.SetTextures(m_texture->GetId());
	drawable.SetVertArray(&m_varray);
	drawable.SetDrawOrder(draworder);
	drawable.SetCull(false);
	drawable.SetColor(m_rgb[0], m_rgb[1], m_rgb[2], m_alpha);
}

void GuiRadialSlider::UpdateVertexArray()
{
	const float deg = M_PI / 180.0f;
	float anchor[2] = {m_x + m_w * 0.5f, m_y + m_h * 0.5f + m_radius};
	float angle = m_end_angle * m_value + m_start_angle * (1.0f - m_value);
	if (m_fill == 0)
	{
		// billboard relative to anchor twelve o'clock position
		float x1 = -m_w * 0.5f;
		float y1 = -m_radius - m_h * 0.5f;
		float x2 = +m_w * 0.5f;
		float y2 = -m_radius + m_h * 0.5f;

		m_varray.SetToBillboard(x1, y1, x2, y2);
		m_varray.Rotate(angle * deg, 0.0f, 0.0f, 1.0f);
		m_varray.Scale(m_dar, 1.0f, 1.0f);
		m_varray.Translate(anchor[0], anchor[1], 0.0f);
	}
	else
	{
		float a0, a1, ad;
		if (m_fill > 0)
		{
			a0 = (m_start_angle - 90) * deg;
			a1 = (angle - 90) * deg;
			ad = angle - m_start_angle;
		}
		else // m_fill < 0
		{
			a0 = (angle - 90) * deg;
			a1 = (m_end_angle - 90) * deg;;
			ad =  m_end_angle - angle;
		}
		float r0 = m_radius - m_h * 0.5f;
		float r1 = m_radius + m_h * 0.5f;
		unsigned n = fabs(m_radius * ad) + 1;

		m_varray.SetTo2DRing(r0, r1, a0, a1, n);
		m_varray.Scale(m_dar, 1.0f, 1.0f);
		m_varray.Translate(anchor[0], anchor[1], 0.0f);
	}
}
