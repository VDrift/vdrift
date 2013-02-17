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
	"onselectx",
	"onselecty",
	"onselect",
	"onfocus",
	"onblur",
	"onmoveup",
	"onmovedown",
	"onmoveleft",
	"onmoveright",
};
const std::vector<std::string> GUICONTROL::signal_names(names, names + sizeof(names) / sizeof(names[0]));

GUICONTROL::GUICONTROL() :
	m_xmin(0),
	m_ymin(0),
	m_xmax(0),
	m_ymax(0)
{
	// ctor
}

GUICONTROL::~GUICONTROL()
{
	// dtor
}

bool GUICONTROL::HasFocus(float x, float y) const
{
	return x <= m_xmax && x >= m_xmin && y <= m_ymax && y >= m_ymin;
}

void GUICONTROL::Select(float x, float y) const
{
	if (!HasFocus(x, y))
		return;

	if (m_selectx.connected())
	{
		float sx = (x - m_xmin) / (m_xmax - m_xmin);
		sx = (sx <= 1) ? (sx >= 0) ? sx : 0 : 1;
		std::stringstream s;
		s << sx;
		m_selectx(s.str());
	}

	if (m_selecty.connected())
	{
		float sy = (y - m_ymin) / (m_ymax - m_ymin);
		std::stringstream s;
		s << sy;
		m_selecty(s.str());
	}
}

void GUICONTROL::Signal(EVENT ev) const
{
	m_signal[ev]();
}

const std::string & GUICONTROL::GetDescription() const
{
	return m_description;
}

void GUICONTROL::SetDescription(const std::string & value)
{
	m_description = value;
}

void GUICONTROL::SetRect(float xmin, float ymin, float xmax, float ymax)
{
	m_xmin = xmin;
	m_ymin = ymin;
	m_xmax = xmax;
	m_ymax = ymax;
}

void GUICONTROL::RegisterActions(
	const std::map<std::string, Slot1<const std::string &>*> & vactionmap,
	const std::map<std::string, Slot0*> & actionmap,
	const Config::const_iterator section,
	const Config & cfg)
{
	std::string actionstr;

	if (cfg.get(section, signal_names[0], actionstr))
		SetActions(vactionmap, actionstr, m_selectx);

	if (cfg.get(section, signal_names[1], actionstr))
		SetActions(vactionmap, actionstr, m_selecty);

	for (size_t i = 0; i < EVENTNUM; ++i)
	{
		if (cfg.get(section, signal_names[i + 2], actionstr))
			SetActions(actionmap, actionstr, m_signal[i]);
	}
}

void GUICONTROL::SetActions(
	const std::map<std::string, Slot0*> & actionmap,
	const std::string & actionstr,
	Signal0 & signal)
{
	std::stringstream st(actionstr);
	while (st.good())
	{
		std::string action;
		st >> action;
		std::map<std::string, Slot0*>::const_iterator it = actionmap.find(action);
		if (it != actionmap.end())
			it->second->connect(signal);
	}
}

void GUICONTROL::SetActions(
	const std::map<std::string, Slot1<const std::string &>*> & actionmap,
	const std::string & actionstr,
	Signal1<const std::string &> & signal)
{
	std::stringstream st(actionstr);
	while (st.good())
	{
		std::string action;
		st >> action;
		std::map<std::string, Slot1<const std::string &>*>::const_iterator it = actionmap.find(action);
		if (it != actionmap.end())
			it->second->connect(signal);
	}
}
