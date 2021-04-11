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

GuiWidgetList::~GuiWidgetList()
{
	for (auto element : m_elements)
	{
		delete element;
	}
}

void GuiWidgetList::Update(SceneNode & scene, float dt)
{
	if (!m_values.empty())
	{
		UpdateElements(scene);
		m_values.clear();
	}

	for (auto element : m_elements)
	{
		element->Update(scene, dt);
	}
}

void GuiWidgetList::SetAlpha(SceneNode & scene, float value)
{
	for (auto element : m_elements)
	{
		element->SetAlpha(scene, value);
	}
}

bool GuiWidgetList::GetProperty(const std::string & name, Delegated<const std::string &> & slot)
{
	#define HASH(s) (s[2] & 0xf)
	#define CASE(s, fn)\
		case HASH(s):\
			if (name == s) {\
				slot.bind<GuiWidgetList, &GuiWidgetList::fn>(this);\
				return true;\
			} break;
	if (name.size() < 3)
		return false;
	switch (HASH(name.c_str()))
	{
		CASE("hue", SetHueAll)
		CASE("sat", SetSatAll)
		CASE("val", SetValAll)
		CASE("opacity", SetOpacityAll)
	}
	return false;
}

bool GuiWidgetList::GetProperty(const std::string & name, Delegated<int, const std::string &> & slot)
{
	if (name.size() < 3)
		return false;
	switch (HASH(name.c_str()))
	{
		CASE("hue", SetHue)
		CASE("sat", SetSat)
		CASE("val", SetVal)
		CASE("opacity", SetOpacity)
		CASE("scroll", ScrollList)
	}
	return false;
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
		if (m_list_offset + int(m_rows * m_cols) < m_list_size)
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

void GuiWidgetList::SetOpacityAll(const std::string & value)
{
	for (auto element : m_elements)
		element->SetOpacity(value);
}

void GuiWidgetList::SetHueAll(const std::string & value)
{
	for (auto element : m_elements)
		element->SetHue(value);
}

void GuiWidgetList::SetSatAll(const std::string & value)
{
	for (auto element : m_elements)
		element->SetSat(value);
}

void GuiWidgetList::SetValAll(const std::string & value)
{
	for (auto element : m_elements)
		element->SetVal(value);
}

Drawable & GuiWidgetList::GetDrawable(SceneNode & scene)
{
	assert(0); // don't want to end up here
	return m_elements[0]->GetDrawable(scene);
}
