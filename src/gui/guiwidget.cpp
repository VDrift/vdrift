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

#include "guiwidget.h"
#include "hsvtorgb.h"
#include "graphics/drawable.h"

GuiWidget::GuiWidget() :
	m_r(1), m_g(1), m_b(1), m_a(1),
	m_h(0), m_s(0), m_v(1),
	m_visible(true),
	m_update(false)
{
	set_color.call.bind<GuiWidget, &GuiWidget::SetColor>(this);
	set_opacity.call.bind<GuiWidget, &GuiWidget::SetOpacity>(this);
	set_hue.call.bind<GuiWidget, &GuiWidget::SetHue>(this);
	set_sat.call.bind<GuiWidget, &GuiWidget::SetSat>(this);
	set_val.call.bind<GuiWidget, &GuiWidget::SetVal>(this);
}

void GuiWidget::Update(SceneNode & scene, float dt)
{
	if (m_update)
	{
		GetDrawable(scene).SetColor(m_r, m_g, m_b, m_a);
		GetDrawable(scene).SetDrawEnable(m_visible);
		m_update = false;
	}
}

void GuiWidget::SetAlpha(SceneNode & scene, float value)
{
	GetDrawable(scene).SetColor(m_r, m_g, m_b, m_a * value);
}

void GuiWidget::SetVisible(SceneNode & scene, bool value)
{
	GetDrawable(scene).SetDrawEnable(m_visible & value);
}

void GuiWidget::SetHSV(float h, float s, float v)
{
	m_h = h; m_s = s; m_v = v;
	HSVtoRGB(m_h, m_s, m_v, m_r, m_g, m_b);
	m_update = true;
}

void GuiWidget::SetRGB(float r, float g, float b)
{
	m_r = r, m_g = g, m_b = b;
	RGBtoHSV(m_r, m_g, m_b, m_h, m_s, m_v);
	m_update = true;
}

void GuiWidget::SetOpacity(float value)
{
	m_a = (value > 0) ? (value < 1) ? 1 - value : 0 : 1;
	m_update = true;
}

void GuiWidget::SetHue(float value)
{
	m_h = (value > 0) ? (value < 1) ? value : 1 : 0;
	HSVtoRGB(m_h, m_s, m_v, m_r, m_g, m_b);
	m_update = true;
}

void GuiWidget::SetSat(float value)
{
	m_s = (value > 0) ? (value < 1) ? value : 1 : 0;
	HSVtoRGB(m_h, m_s, m_v, m_r, m_g, m_b);
	m_update = true;
}

void GuiWidget::SetVal(float value)
{
	m_v = (value > 0) ? (value < 1) ? value : 1 : 0;
	HSVtoRGB(m_h, m_s, m_v, m_r, m_g, m_b);
	m_update = true;
}

void GuiWidget::SetColor(const std::string & value)
{
	if (value.empty()) return;

	std::istringstream s(value);
	Vec3 v;
	s >> v;
	SetRGB(v[0], v[1], v[2]);
}

void GuiWidget::SetOpacity(const std::string & value)
{
	if (value.empty()) return;

	std::istringstream s(value);
	float v;
	s >> v;
	SetOpacity(v);
}

void GuiWidget::SetHue(const std::string & value)
{
	if (value.empty()) return;

	std::istringstream s(value);
	float v;
	s >> v;
	SetHue(v);
}

void GuiWidget::SetSat(const std::string & value)
{
	if (value.empty()) return;

	std::istringstream s(value);
	float v;
	s >> v;
	SetSat(v);
}

void GuiWidget::SetVal(const std::string & value)
{
	if (value.empty()) return;

	std::istringstream s(value);
	float v;
	s >> v;
	SetVal(v);
}
