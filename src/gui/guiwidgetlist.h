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

#ifndef _GUIWIDGETLIST_H
#define _GUIWIDGETLIST_H

#include "guilist.h"
#include "guiwidget.h"
#include "signalslot.h"

class SCENENODE;
class DRAWABLE;

/// a widget that contains a list of widgets
class GUIWIDGETLIST : public GUIWIDGET, public GUILIST
{
public:
	/// base destructor
	virtual ~GUIWIDGETLIST();

	/// update state
	void Update(SCENENODE & scene, float dt);

	/// scale alpha [0, 1]
	void SetAlpha(SCENENODE & scene, float value);

	/// override visibility
	void SetVisible(SCENENODE & scene, bool value);

	/// element property slots
	Slot2<unsigned, const std::string &> set_color;
	Slot2<unsigned, const std::string &> set_opacity;
	Slot2<unsigned, const std::string &> set_hue;
	Slot2<unsigned, const std::string &> set_sat;
	Slot2<unsigned, const std::string &> set_val;

	/// list navigation slots (could be Slot0, but whatever)
	Slot1<unsigned> prev_row;
	Slot1<unsigned> next_row;
	Slot1<unsigned> prev_col;
	Slot1<unsigned> next_col;

	/// widget list update slot (could be Slot0, but ...)
	Slot1<const std::string &> update_list;

	/// value list range access signal
	Signal2<int, std::vector<std::string> &> get_values;

protected:
	std::vector<GUIWIDGET*> m_elements;
	std::vector<std::string> m_values;

	/// verboten
	GUIWIDGETLIST();

	/// element property setters
	void SetColor(unsigned n, const std::string & value);
	void SetOpacity(unsigned n, const std::string & value);
	void SetHue(unsigned n, const std::string & value);
	void SetSat(unsigned n, const std::string & value);
	void SetVal(unsigned n, const std::string & value);

	/// list navigation metods
	void SelectPrevRow(unsigned n);
	void SelectNextRow(unsigned n);
	void SelectPrevCol(unsigned n);
	void SelectNextCol(unsigned n);

	/// attached to update_list slot
	/// calls get_values to update value list
	void UpdateList(const std::string & value);

	/// called during Update to process m_values
	virtual void UpdateElements(SCENENODE & scene) = 0;
};

#endif // _GUIWIDGETLIST_H
