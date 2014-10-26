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

#ifndef _GUICONTROLLIST_H
#define _GUICONTROLLIST_H

#include "guilist.h"
#include "guicontrol.h"

/// a widget that mimics a list of controls
class GuiControlList : public GuiControl, public GuiList
{
public:
	GuiControlList();

	~GuiControlList();

	/// Return true if control contains x, y
	bool Focus(float x, float y);

	/// Signal slots attached to events
	void Signal(Event ev);

	/// List update slot, parameter holds list item count
	Slot1<const std::string &> update_list;

	/// Set active element, parameter holds list item index
	Slot1<const std::string &> set_nth;

	/// List scroll slots
	Slot0 scroll_fwd;
	Slot0 scroll_rev;

	Signal1<int> m_signaln[EVENTNUM];

private:
	int m_active_element;

	void UpdateList(const std::string & value);

	void SetToNth(const std::string & value);

	void ScrollFwd();

	void ScrollRev();

	void SetActiveElement(int active_element);
};

#endif // _GUICONTROLLIST_H
