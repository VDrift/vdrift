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

#ifndef _GUIPAGE_H
#define _GUIPAGE_H

#include "graphics/scenenode.h"
#include "signalslot.h"

#include <map>
#include <vector>
#include <iosfwd>
#include <string>

class GuiLanguage;
class GuiWidget;
class GuiControl;
class GuiLabel;
class Font;
class Config;
class ContentManager;
class PathManager;

typedef std::map<std::string, Signal1<const std::string &>*> StrSignalMap;
typedef std::map<std::string, Slot2<int, std::vector<std::string> &>*> StrVecSlotMap;
typedef std::map<std::string, Slot1<const std::string &>*> StrSlotMap;
typedef std::map<std::string, Slot1<int>*> IntSlotMap;
typedef std::map<std::string, Slot0*> SlotMap;

class GuiPage
{
public:
	GuiPage();

	~GuiPage();

	bool Load(
		const std::string & path,
		const std::string & texpath,
		const float screenhwratio,
		const GuiLanguage & lang,
		const Font & font,
		const StrSignalMap & vsignalmap,
		const StrVecSlotMap & vnactionmap,
		const StrSlotMap & vactionmap,
		IntSlotMap nactionmap,
		SlotMap actionmap,
		ContentManager & content,
		std::ostream & error_output);

	void SetVisible(bool value);

	void SetAlpha(float value);

	/// execute game actions and update gui options
	void ProcessInput(
		float cursorx, float cursory,
		bool cursormoved, bool cursordown, bool cursorjustup,
		bool moveleft, bool moveright,
		bool moveup, bool movedown,
		bool select, bool cancel);

	/// tell all child widgets to do as update tick
	void Update(float dt);

	void SetLabelText(const std::map<std::string, std::string> & label_text);

	GuiLabel * GetLabel(const std::string & name);

	SceneNode & GetNode();

private:
	std::map <std::string, GuiLabel *> labels;
	std::vector <GuiControl *> controls;
	std::vector <GuiWidget *> widgets;
	GuiControl * default_control;
	GuiControl * active_control;
	SceneNode node;
	std::string name;

	// each control registers a ControlCB
	// which other controls can signal to focus(activate) it
	// it uses PAGE::SetActiveControl
	struct ControlCb
	{
		GuiPage * page;
		GuiControl * control;
		Slot0 action;

		ControlCb();
		ControlCb(const ControlCb & other);
		ControlCb & operator=(const ControlCb & other);
		void call();
	};
	// allow a conrol to signal slots with an extra parameter
	struct SignalVal
	{
		std::string value;
		Signal1<const std::string &> signal;
		Slot0 action;

		SignalVal();
		SignalVal(const SignalVal & other);
		SignalVal & operator=(const SignalVal & other);
		void call();
	};
	// allow a control list to signal slots with an extra parameter
	struct SignalValn
	{
		std::string value;
		Signal2<int, const std::string&> signal;
		Slot1<int> action;

		SignalValn();
		SignalValn(const SignalValn & other);
		SignalValn & operator=(const SignalValn & other);
		void call(int n);
	};
	std::vector<ControlCb> control_set;		// control focus callbacks
	std::vector<SignalVal> action_set;		// action value callbacks
	std::vector<SignalValn> action_setn;	// action value nth callbacks
	Signal0 onfocus, oncancel;				// page action signals

	void Clear(SceneNode & parentnode);

	void Clear();

	void SetActiveControl(GuiControl & control);
};

#endif
