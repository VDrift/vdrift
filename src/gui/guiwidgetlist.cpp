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

GUIWIDGETLIST::GUIWIDGETLIST()
{
	set_color.call.bind<GUIWIDGETLIST, &GUIWIDGETLIST::SetColor>(this);
	set_opacity.call.bind<GUIWIDGETLIST, &GUIWIDGETLIST::SetOpacity>(this);
	set_hue.call.bind<GUIWIDGETLIST, &GUIWIDGETLIST::SetHue>(this);
	set_sat.call.bind<GUIWIDGETLIST, &GUIWIDGETLIST::SetSat>(this);
	set_val.call.bind<GUIWIDGETLIST, &GUIWIDGETLIST::SetVal>(this);
	prev_row.call.bind<GUIWIDGETLIST, &GUIWIDGETLIST::SelectPrevRow>(this);
	next_row.call.bind<GUIWIDGETLIST, &GUIWIDGETLIST::SelectNextRow>(this);
	prev_col.call.bind<GUIWIDGETLIST, &GUIWIDGETLIST::SelectPrevCol>(this);
	next_col.call.bind<GUIWIDGETLIST, &GUIWIDGETLIST::SelectNextCol>(this);
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

void GUIWIDGETLIST::SetColor(unsigned n, const std::string & value)
{
	if (n < m_elements.size())
	{
		m_elements[n]->SetColor(value);
	}
}

void GUIWIDGETLIST::SetOpacity(unsigned n, const std::string & value)
{
	if (n < m_elements.size())
	{
		m_elements[n]->SetOpacity(value);
	}
}

void GUIWIDGETLIST::SetHue(unsigned n, const std::string & value)
{
	if (n < m_elements.size())
	{
		m_elements[n]->SetHue(value);
	}
}

void GUIWIDGETLIST::SetSat(unsigned n, const std::string & value)
{
	if (n < m_elements.size())
	{
		m_elements[n]->SetSat(value);
	}
}

void GUIWIDGETLIST::SetVal(unsigned n, const std::string & value)
{
	if (n < m_elements.size())
	{
		m_elements[n]->SetVal(value);
	}
}

void GUIWIDGETLIST::SelectPrevRow(unsigned)
{
	// todo
}

void GUIWIDGETLIST::SelectNextRow(unsigned)
{
	// todo
}

void GUIWIDGETLIST::SelectPrevCol(unsigned)
{
	// todo
}

void GUIWIDGETLIST::SelectNextCol(unsigned)
{
	// todo
}

void GUIWIDGETLIST::UpdateList(const std::string &)
{
	if (get_values.connected())
	{
		m_values.resize(m_rows * m_cols);
		get_values(m_list_offset, m_values);
	}
}
