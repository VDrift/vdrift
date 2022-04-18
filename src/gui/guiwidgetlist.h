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

class SceneNode;
class Drawable;

/// a widget that contains a list of widgets
class GuiWidgetList : public GuiWidget, public GuiList
{
public:
	/// base destructor
	virtual ~GuiWidgetList();

	/// update state
	void Update(SceneNode & scene, float dt) override;

	/// scale alpha [0, 1]
	void SetAlpha(SceneNode & scene, float value) override;

	bool GetProperty(const std::string & name, Delegated<const std::string &> & slot) override;

	/// element properties
	virtual bool GetProperty(const std::string & name, Delegated<int, const std::string &> & slot);

	void SetOpacity(int n, const std::string & value);
	void SetHue(int n, const std::string & value);
	void SetSat(int n, const std::string & value);
	void SetVal(int n, const std::string & value);

	/// scroll list, value designates scroll direction: fwd, rev
	void ScrollList(int n, const std::string & value);

	/// update list, parameter holds list item count
	void UpdateList(const std::string & vnum);

	/// value list range access signal
	Signald<int, std::vector<std::string> &> get_values;

protected:
	std::vector<GuiWidget*> m_elements;
	std::vector<std::string> m_values;

	GuiWidgetList() {};
	GuiWidgetList(const GuiWidgetList & other);
	GuiWidgetList & operator=(const GuiWidgetList & other);

	/// called during Update to process m_values
	virtual void UpdateElements(SceneNode & scene) = 0;

	/// override widget property callbacks
	void SetOpacityAll(const std::string & value);
	void SetHueAll(const std::string & value);
	void SetSatAll(const std::string & value);
	void SetValAll(const std::string & value);

	/// ugh, dead weight
	Drawable & GetDrawable(SceneNode & scene) override;
};

#endif // _GUIWIDGETLIST_H
