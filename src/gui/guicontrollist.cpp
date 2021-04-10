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
#include <minmax.h>
#include <sstream>

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

void GuiControlList::SignalEvent(Event ev)
{
	if (ev == MOVEUP)
	{
		int n = m_vertical ? m_active_element - 1 : m_active_element - m_cols;
		SetActiveElement(n);
	}
	else if (ev == MOVEDOWN)
	{
		int n = m_vertical ? m_active_element + 1 : m_active_element + m_cols;
		SetActiveElement(n);
	}
	else if (ev == MOVELEFT)
	{
		int n = m_vertical ? m_active_element - m_rows : m_active_element - 1;
		SetActiveElement(n);
	}
	else if (ev == MOVERIGHT)
	{
		int n = m_vertical ? m_active_element + m_rows : m_active_element + 1;
		SetActiveElement(n);
	}
	else
	{
		m_signaln[ev](m_active_element + m_list_offset);
		GuiControl::SignalEvent(ev);
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

	list_item = Clamp(list_item, 0, m_list_size);

	int active_item = m_active_element + m_list_offset;
	if (list_item != active_item)
	{
		m_signaln[BLUR](active_item);

		if (list_item > active_item)
		{
			while (list_item >= m_list_offset + int(m_rows * m_cols))
				ScrollFwd();
		}
		else
		{
			while (list_item < m_list_offset)
				ScrollRev();
		}
		m_active_element = list_item - m_list_offset;
		assert(m_active_element >= 0);
		m_signaln[FOCUS](list_item);
	}
}

void GuiControlList::ScrollFwd()
{
	int delta = m_vertical ? m_rows : m_cols;
	assert(m_list_offset % delta == 0);
	if (m_list_offset + int(m_rows * m_cols) < m_list_size)
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
	assert(m_list_offset % delta == 0);
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
	if (m_active_element != active_element)
	{
		m_signaln[BLUR](m_active_element + m_list_offset);

		m_active_element = active_element;
		if (active_element < 0)
			ScrollRev();
		else if (active_element >= int(m_rows * m_cols))
			ScrollFwd();

		m_active_element = Min(m_active_element, m_list_size - m_list_offset - 1);
		m_signaln[FOCUS](m_active_element + m_list_offset);
	}
}
