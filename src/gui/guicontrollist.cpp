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

#include "guicontrollist.h"

GuiControlList::GuiControlList() :
	m_active_element(0)
{
	update_list.call.bind<GuiControlList, &GuiControlList::UpdateList>(this);
	set_nth.call.bind<GuiControlList, &GuiControlList::SetToNth>(this);
	scroll_fwd.call.bind<GuiControlList, &GuiControlList::ScrollFwd>(this);
	scroll_rev.call.bind<GuiControlList, &GuiControlList::ScrollRev>(this);
}

GuiControlList::~GuiControlList()
{
	// dtor
}

bool GuiControlList::Focus(float x, float y)
{
	if (GuiControl::Focus(x, y))
	{
		int active_element = GetElemId(x, y);
		SetActiveElement(active_element);
		return true;
	}
	return false;
}

void GuiControlList::Signal(Event ev)
{
	if (ev == MOVEUP)
	{
		unsigned col, row;
		GetElemPos(m_active_element, col, row);
		int n = m_vertical ? m_active_element - 1 : m_active_element - m_cols;
		SetActiveElement(n);
	}
	else if (ev == MOVEDOWN)
	{
		unsigned col, row;
		GetElemPos(m_active_element, col, row);
		int n = m_vertical ? m_active_element + 1 : m_active_element + m_cols;
		SetActiveElement(n);
	}
	else if (ev == MOVELEFT)
	{
		unsigned col, row;
		GetElemPos(m_active_element, col, row);
		int n = m_vertical ? m_active_element - m_rows : m_active_element - 1;
		SetActiveElement(n);
	}
	else if (ev == MOVERIGHT)
	{
		unsigned col, row;
		GetElemPos(m_active_element, col, row);
		int n = m_vertical ? m_active_element + m_rows : m_active_element + 1;
		SetActiveElement(n);
	}
	else
	{
		m_signaln[ev](m_active_element + m_list_offset);
		GuiControl::Signal(ev);
	}
}

void GuiControlList::UpdateList(const std::string & value)
{
	std::istringstream s(value);
	s >> m_list_size;

	int list_item = m_active_element + m_list_offset;
	if (list_item >= m_list_size)
	{
		m_signaln[BLUR](list_item);
		list_item = m_list_size - 1;

		if (list_item < 0)
		{
			m_active_element = 0;
			m_list_offset = 0;
			return;
		}

		if (list_item < m_list_offset)
		{
			m_active_element = list_item % (m_rows * m_cols);
			m_list_offset = list_item - m_active_element;
		}

		m_signaln[FOCUS](list_item);
	}
}

void GuiControlList::SetToNth(const std::string & value)
{
	int list_item(0);
	std::istringstream s(value);
	s >> list_item;

	if (list_item != m_active_element + m_list_offset)
	{
		m_signaln[BLUR](m_active_element + m_list_offset);

		int active_element = list_item % (m_rows * m_cols);
		int list_offset = list_item - active_element;
		int delta = list_offset - m_list_offset;
		m_active_element = active_element + delta;
		if (delta > 0)
		{
			while (list_offset > m_list_offset)
				ScrollFwd();
		}
		else if (delta < 0)
		{
			while (list_offset < m_list_offset)
				ScrollRev();
		}

		m_signaln[FOCUS](m_active_element + m_list_offset);
	}
}

void GuiControlList::ScrollFwd()
{
	int delta = m_vertical ? m_rows : m_cols;
	if (m_list_offset < m_list_size - m_list_size % int(m_rows * m_cols))
	{
		m_list_offset += delta;
		m_active_element -= delta;
		m_signaln[SCROLLF](m_active_element + m_list_offset);
		m_signal[SCROLLF]();
	}
}

void GuiControlList::ScrollRev()
{
	int delta = m_vertical ? m_rows : m_cols;
	if (m_list_offset >= delta)
	{
		m_list_offset -= delta;
		m_active_element += delta;
		m_signaln[SCROLLR](m_active_element + m_list_offset);
		m_signal[SCROLLR]();
	}
}

void GuiControlList::SetActiveElement(int active_element)
{
	int list_item = active_element + m_list_offset;
	if (m_active_element != active_element &&
		list_item >= 0 && list_item < m_list_size)
	{
		m_signaln[BLUR](m_active_element + m_list_offset);

		m_active_element = active_element;
		if (active_element < 0)
			ScrollRev();
		else if (active_element >= int(m_rows * m_cols))
			ScrollFwd();

		m_signaln[FOCUS](m_active_element + m_list_offset);
	}
}
