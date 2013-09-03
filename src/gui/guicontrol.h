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

#ifndef _GUICONTROL_H
#define _GUICONTROL_H

#include "signalslot.h"
#include "cfg/config.h"
#include <string>
#include <map>

class SceneNode;

class GuiControl
{
public:
	enum Event
	{
		FOCUS = 0,
		BLUR,
		MOVEUP,
		MOVEDOWN,
		MOVELEFT,
		MOVERIGHT,
		SELECTUP,
		SELECTDOWN,
		SCROLLF,
		SCROLLR,
		EVENTNUM
	};
	enum EventV
	{
		SELECTX = 0,
		SELECTY,
		EVENTVNUM
	};

	GuiControl();

	virtual ~GuiControl();

	/// Return true if control contains x, y
	virtual bool Focus(float x, float y);

	/// Signal to slots attached to events
	virtual void Signal(Event ev);

	const std::string & GetDescription() const;

	void SetDescription(const std::string & value);

	/// Set control rectangle
	void SetRect(float xmin, float ymin, float xmax, float ymax);

	/// Register event actions
	void RegisterActions(
		const std::map<std::string, Slot1<const std::string &>*> & vactionmap,
		const std::map<std::string, Slot0*> & actionmap,
		const Config::const_iterator section,
		const Config & cfg);

	/// Register event actions to signal
	static void SetActions(
		const std::map<std::string, Slot0*> & actionmap,
		const std::string & actionstr,
		Signal0 & signal);

	/// Register event value actions to signal (onselectx, onselecty)
	static void SetActions(
		const std::map<std::string, Slot1<const std::string &>*> & actionmap,
		const std::string & actionstr,
		Signal1<const std::string &> & signal);

	/// available control signals
	static const std::vector<std::string> signal_names;

protected:
	float m_xmin, m_ymin, m_xmax, m_ymax;
	float m_focusx, m_focusy;
	std::string m_description;
	Signal0 m_signal[EVENTNUM];
	Signal1<const std::string &> m_signalv[EVENTVNUM];
};

#endif //_GUICONTROL_H
