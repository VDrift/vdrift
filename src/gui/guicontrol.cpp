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

#include "guicontrol.h"

static const std::string names[] = {
	"onfocus",
	"onblur",
	"onmoveup",
	"onmovedown",
	"onmoveleft",
	"onmoveright",
	"onselect",
	"onselectd",
	"onscrollf",
	"onscrollr",
	"onselectx",
	"onselecty",
};
const std::vector<std::string> GuiControl::signal_names(names, names + sizeof(names) / sizeof(names[0]));

GuiControl::GuiControl() :
	m_xmin(0), m_ymin(0),
	m_xmax(0), m_ymax(0),
	m_focusx(0), m_focusy(0)
{
	// ctor
}

GuiControl::~GuiControl()
{
	// dtor
}

bool GuiControl::Focus(float x, float y)
{
	if (x <= m_xmax && x >= m_xmin && y <= m_ymax && y >= m_ymin)
	{
		m_focusx = x;
		m_focusy = y;
		return true;
	}
	return false;
}

void GuiControl::Signal(Event ev)
{
	if (ev == SELECTDOWN)
	{
		if (m_signalv[SELECTX].connected())
		{
			float sx = (m_focusx - m_xmin) / (m_xmax - m_xmin);
			sx = (sx <= 1) ? (sx >= 0) ? sx : 0 : 1;
			std::ostringstream s;
			s << sx;
			m_signalv[SELECTX](s.str());
		}

		if (m_signalv[SELECTX].connected())
		{
			float sy = (m_focusy - m_ymin) / (m_ymax - m_ymin);
			std::ostringstream s;
			s << sy;
			m_signalv[SELECTY](s.str());
		}
	}
	m_signal[ev]();
}

void GuiControl::SetRect(float xmin, float ymin, float xmax, float ymax)
{
	m_xmin = xmin;
	m_ymin = ymin;
	m_xmax = xmax;
	m_ymax = ymax;
}
