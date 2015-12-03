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

#include "guilinearslider.h"

GuiLinearSlider::GuiLinearSlider() :
	m_vertical(false)
{
	// ctor
}

GuiLinearSlider::~GuiLinearSlider()
{
	// dtor
}

void GuiLinearSlider::Update(SceneNode & scene, float dt)
{
	if (m_update)
	{
		UpdateVertexArray();
		GuiWidget::Update(scene, dt);
	}
}

void GuiLinearSlider::SetupDrawable(
	SceneNode & node,
	const std::shared_ptr<Texture> & texture,
	float xywh[4], float z, bool vertical,
	std::ostream & /*error_output*/)
{
	m_vertical = vertical;

	InitDrawable(node, texture, xywh, z);
}

void GuiLinearSlider::UpdateVertexArray()
{
	float x1, y1, x2, y2;
	if (m_vertical)
	{
		x1 = m_x;
		y1 = m_y + m_h * m_min_value;
		x2 = m_x + m_w;
		y2 = m_y + m_h * m_max_value;
	}
	else
	{
		x1 = m_x + m_w * m_min_value;
		y1 = m_y;
		x2 = m_x + m_w * m_max_value;
		y2 = m_y + m_h;
	}
	m_varray.SetToBillboard(x1, y1, x2, y2);
}
