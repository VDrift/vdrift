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
#include <string>
#include <map>

class Config;
class SCENENODE;

class GUICONTROL
{
public:
	GUICONTROL();

	virtual ~GUICONTROL();

	/// Return true if control in focus
	bool InFocus(float x, float y) const;

	void OnSelect(float x, float y) const;

	void OnSelect() const;

	void OnFocus() const;

	void OnBlur() const;

	void OnMoveUp() const;

	void OnMoveDown() const;

	void OnMoveLeft() const;

	void OnMoveRight() const;

	const std::string & GetDescription() const;

	void SetDescription(const std::string & value);

	void SetRect(float xmin, float ymin, float xmax, float ymax);

	/// Register event actions
	void RegisterActions(
		const std::map<std::string, Slot1<const std::string &>*> & vactionmap,
		const std::map<std::string, Slot0*> & actionmap,
		const std::string & name,
		const Config & cfg);

	/// Register event actions to signal
	static void SetActions(
		const std::map<std::string, Slot0*> & actionmap,
		const std::string & actionstr,
		Signal0 & signal);

	static void SetActions(
		const std::map<std::string, Slot1<const std::string &>*> & actionmap,
		const std::string & actionstr,
		Signal1<const std::string &> & signal);

	static const std::vector<std::string> signals;

protected:
	float m_xmin, m_ymin, m_xmax, m_ymax;
	std::string m_description;
	Signal1<const std::string &> onselectx;
	Signal1<const std::string &> onselecty;
	Signal0 onselect;
	Signal0 onfocus;
	Signal0 onblur;
	Signal0 onmoveup;
	Signal0 onmovedown;
	Signal0 onmoveleft;
	Signal0 onmoveright;
};

#endif //_GUICONTROL_H
