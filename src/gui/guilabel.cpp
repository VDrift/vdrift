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

#include "guilabel.h"
#include <cassert>

void GuiLabel::SetupDrawable(
	SceneNode & scene,
	const Font & font, int align,
	float scalex, float scaley,
	float xywh[4], float z)
{
	m_font = &font;
	m_x = xywh[0];
	m_y = xywh[1];
	m_w = xywh[2];
	m_h = xywh[3];
	m_scalex = scalex;
	m_scaley = scaley;
	m_align = align;

	m_draw = scene.GetDrawList().text.insert(Drawable());
	Drawable & drawref = GetDrawable(scene);
	drawref.SetDrawOrder(z);

	m_text_draw.Set(drawref, font, m_text, m_x, m_y, scalex, scaley, m_rgb[0], m_rgb[1], m_rgb[2]);
}

bool GuiLabel::GetProperty(const std::string & name, Delegated<const std::string &> & slot)
{
	if (name == "text")
	{
		slot.bind<GuiLabel, &GuiLabel::SetText>(this);
		return true;
	}
	return GuiWidget::GetProperty(name, slot);
}

void GuiLabel::SetText(const std::string & text)
{
	if (m_text != text)
	{
		assert(m_font);
		m_text = text;

		float x = m_x;
		float textw = m_font->GetWidth(m_text) * m_scalex;
		if (m_align == -1) x -= m_w * 0.5f;
		else if (m_align == 0) x -= textw * 0.5f;
		else if (m_align == 1) x -= (textw - m_w * 0.5f);

		m_text_draw.Revise(*m_font, m_text, x, m_y, m_scalex, m_scaley);
	}
}

Drawable & GuiLabel::GetDrawable(SceneNode & scene)
{
	return scene.GetDrawList().text.get(m_draw);
}
