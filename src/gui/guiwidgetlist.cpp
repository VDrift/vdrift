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

GUIWIDGETLIST::GUIWIDGETLIST()
{
	// override widget callbacks (ugly)
	GUIWIDGET::set_color.call.bind<GUIWIDGETLIST, &GUIWIDGETLIST::SetColor1>(this);
	GUIWIDGET::set_opacity.call.bind<GUIWIDGETLIST, &GUIWIDGETLIST::SetOpacity1>(this);
	GUIWIDGET::set_hue.call.bind<GUIWIDGETLIST, &GUIWIDGETLIST::SetHue1>(this);
	GUIWIDGET::set_sat.call.bind<GUIWIDGETLIST, &GUIWIDGETLIST::SetSat1>(this);
	GUIWIDGET::set_val.call.bind<GUIWIDGETLIST, &GUIWIDGETLIST::SetVal1>(this);

	set_color.call.bind<GUIWIDGETLIST, &GUIWIDGETLIST::SetColor>(this);
	set_opacity.call.bind<GUIWIDGETLIST, &GUIWIDGETLIST::SetOpacity>(this);
	set_hue.call.bind<GUIWIDGETLIST, &GUIWIDGETLIST::SetHue>(this);
	set_sat.call.bind<GUIWIDGETLIST, &GUIWIDGETLIST::SetSat>(this);
	set_val.call.bind<GUIWIDGETLIST, &GUIWIDGETLIST::SetVal>(this);
	scroll_list.call.bind<GUIWIDGETLIST, &GUIWIDGETLIST::ScrollList>(this);
	update_list.call.bind<GUIWIDGETLIST, &GUIWIDGETLIST::UpdateList>(this);
}

GUIWIDGETLIST::~GUIWIDGETLIST()
{
	for (size_t i = 0; i < m_elements.size(); ++i)
	{
		delete m_elements[i];
	}
}

void GUIWIDGETLIST::Update(SCENENODE & scene, float dt)
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

void GUIWIDGETLIST::SetAlpha(SCENENODE & scene, float value)
{
	for (size_t i = 0; i < m_elements.size(); ++i)
	{
		m_elements[i]->SetAlpha(scene, value);
	}
}

void GUIWIDGETLIST::SetVisible(SCENENODE & scene, bool value)
{
	for (size_t i = 0; i < m_elements.size(); ++i)
	{
		m_elements[i]->SetVisible(scene, value);
	}
}

void GUIWIDGETLIST::SetColor(int n, const std::string & value)
{
	size_t i = n - m_list_offset;
	if (i < m_elements.size())
	{
		m_elements[i]->SetColor(value);
	}
}

void GUIWIDGETLIST::SetOpacity(int n, const std::string & value)
{
	size_t i = n - m_list_offset;
	if (i < m_elements.size())
	{
		m_elements[i]->SetOpacity(value);
	}
}

void GUIWIDGETLIST::SetHue(int n, const std::string & value)
{
	size_t i = n - m_list_offset;
	if (i < m_elements.size())
	{
		m_elements[i]->SetHue(value);
	}
}

void GUIWIDGETLIST::SetSat(int n, const std::string & value)
{
	size_t i = n - m_list_offset;
	if (i >= 0 && i < m_elements.size())
	{
		m_elements[i]->SetSat(value);
	}
}

void GUIWIDGETLIST::SetVal(int n, const std::string & value)
{
	size_t i = n - m_list_offset;
	if (i < m_elements.size())
	{
		m_elements[i]->SetVal(value);
	}
}

void GUIWIDGETLIST::ScrollList(int n, const std::string & value)
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

void GUIWIDGETLIST::UpdateList(const std::string & vnum)
{
	int list_size(0), list_item (0);
	std::stringstream s(vnum);
	s >> list_size >> list_item;

	m_list_size = list_size;
	m_list_offset = list_item - list_item % (m_rows * m_cols);

	if (get_values.connected())
	{
		m_values.resize(m_rows * m_cols);
		get_values(m_list_offset, m_values);
	}
}

void GUIWIDGETLIST::SetColor1(const std::string & value)
{
	for (size_t i = 0; i < m_elements.size(); ++i)
		m_elements[i]->SetColor(value);
}

void GUIWIDGETLIST::SetOpacity1(const std::string & value)
{
	for (size_t i = 0; i < m_elements.size(); ++i)
		m_elements[i]->SetOpacity(value);
}

void GUIWIDGETLIST::SetHue1(const std::string & value)
{
	for (size_t i = 0; i < m_elements.size(); ++i)
		m_elements[i]->SetHue(value);
}

void GUIWIDGETLIST::SetSat1(const std::string & value)
{
	for (size_t i = 0; i < m_elements.size(); ++i)
		m_elements[i]->SetSat(value);
}

void GUIWIDGETLIST::SetVal1(const std::string & value)
{
	for (size_t i = 0; i < m_elements.size(); ++i)
		m_elements[i]->SetVal(value);
}

DRAWABLE & GUIWIDGETLIST::GetDrawable(SCENENODE & scene)
{
	assert(0); // don't want to end up here
	return m_elements[0]->GetDrawable(scene);
}
