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

#include "guiwidgetlist.h"
#include "guiwidget.h"
#include <sstream>

GuiWidgetList::GuiWidgetList()
{
	// override widget callbacks
	GuiWidget::set_color.call.bind<GuiWidgetList, &GuiWidgetList::SetColorAll>(this);
	GuiWidget::set_opacity.call.bind<GuiWidgetList, &GuiWidgetList::SetOpacityAll>(this);
	GuiWidget::set_hue.call.bind<GuiWidgetList, &GuiWidgetList::SetHueAll>(this);
	GuiWidget::set_sat.call.bind<GuiWidgetList, &GuiWidgetList::SetSatAll>(this);
	GuiWidget::set_val.call.bind<GuiWidgetList, &GuiWidgetList::SetValAll>(this);

	setn_color.call.bind<GuiWidgetList, &GuiWidgetList::SetColor>(this);
	setn_opacity.call.bind<GuiWidgetList, &GuiWidgetList::SetOpacity>(this);
	setn_hue.call.bind<GuiWidgetList, &GuiWidgetList::SetHue>(this);
	setn_sat.call.bind<GuiWidgetList, &GuiWidgetList::SetSat>(this);
	setn_val.call.bind<GuiWidgetList, &GuiWidgetList::SetVal>(this);
	scroll.call.bind<GuiWidgetList, &GuiWidgetList::ScrollList>(this);
	update_list.call.bind<GuiWidgetList, &GuiWidgetList::UpdateList>(this);
}

GuiWidgetList::~GuiWidgetList()
{
	for (size_t i = 0; i < m_elements.size(); ++i)
	{
		delete m_elements[i];
	}
}

void GuiWidgetList::Update(SceneNode & scene, float dt)
{
	if (!m_values.empty())
	{
		UpdateElements(scene);
		m_values.clear();
	}

	for (size_t i = 0; i < m_elements.size(); ++i)
	{
		m_elements[i]->Update(scene, dt);
	}
}

void GuiWidgetList::SetAlpha(SceneNode & scene, float value)
{
	for (size_t i = 0; i < m_elements.size(); ++i)
	{
		m_elements[i]->SetAlpha(scene, value);
	}
}

bool GuiWidgetList::GetProperty(const std::string & name, Slot2<int, const std::string &> *& slot)
{
	if (name == "hue")
		return (slot = &setn_hue);
	if (name == "sat")
		return (slot = &setn_sat);
	if (name == "val")
		return (slot = &setn_val);
	if (name == "opacity")
		return (slot = &setn_opacity);
	if (name == "color")
		return (slot = &setn_color);
	if (name == "scroll")
		return (slot = &scroll);
	return (slot = NULL);
}

void GuiWidgetList::SetColor(int n, const std::string & value)
{
	size_t i = n - m_list_offset;
	if (i < m_elements.size())
	{
		m_elements[i]->SetColor(value);
	}
}

void GuiWidgetList::SetOpacity(int n, const std::string & value)
{
	size_t i = n - m_list_offset;
	if (i < m_elements.size())
	{
		m_elements[i]->SetOpacity(value);
	}
}

void GuiWidgetList::SetHue(int n, const std::string & value)
{
	size_t i = n - m_list_offset;
	if (i < m_elements.size())
	{
		m_elements[i]->SetHue(value);
	}
}

void GuiWidgetList::SetSat(int n, const std::string & value)
{
	size_t i = n - m_list_offset;
	if (i < m_elements.size())
	{
		m_elements[i]->SetSat(value);
	}
}

void GuiWidgetList::SetVal(int n, const std::string & value)
{
	size_t i = n - m_list_offset;
	if (i < m_elements.size())
	{
		m_elements[i]->SetVal(value);
	}
}

void GuiWidgetList::ScrollList(int /*n*/, const std::string & value)
{
	if (value == "fwd")
	{
		int delta = m_vertical ? m_rows : m_cols;
		if (m_list_offset < m_list_size - delta)
		{
			m_list_offset += delta;
			if (get_values.connected())
			{
				m_values.resize(m_rows * m_cols);
				get_values(m_list_offset, m_values);
			}
		}
	}
	else if (value == "rev")
	{
		int delta = m_vertical ? m_rows : m_cols;
		if (m_list_offset >= delta)
		{
			m_list_offset -= delta;
			if (get_values.connected())
			{
				m_values.resize(m_rows * m_cols);
				get_values(m_list_offset, m_values);
			}
		}
	}
}

void GuiWidgetList::UpdateList(const std::string & vnum)
{
	if (!get_values.connected())
		return;

	std::istringstream s(vnum);
	s >> m_list_size;

	if (m_list_size <= m_list_offset)
		m_list_offset = m_list_size - m_list_size % (m_rows * m_cols);

	m_values.resize(m_rows * m_cols);
	get_values(m_list_offset, m_values);
}

void GuiWidgetList::SetColorAll(const std::string & value)
{
	for (size_t i = 0; i < m_elements.size(); ++i)
		m_elements[i]->SetColor(value);
}

void GuiWidgetList::SetOpacityAll(const std::string & value)
{
	for (size_t i = 0; i < m_elements.size(); ++i)
		m_elements[i]->SetOpacity(value);
}

void GuiWidgetList::SetHueAll(const std::string & value)
{
	for (size_t i = 0; i < m_elements.size(); ++i)
		m_elements[i]->SetHue(value);
}

void GuiWidgetList::SetSatAll(const std::string & value)
{
	for (size_t i = 0; i < m_elements.size(); ++i)
		m_elements[i]->SetSat(value);
}

void GuiWidgetList::SetValAll(const std::string & value)
{
	for (size_t i = 0; i < m_elements.size(); ++i)
		m_elements[i]->SetVal(value);
}

Drawable & GuiWidgetList::GetDrawable(SceneNode & scene)
{
	assert(0); // don't want to end up here
	return m_elements[0]->GetDrawable(scene);
}
