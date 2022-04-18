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

using StrSignalMap = std::map<std::string, Signald<const std::string &>*>;
using StrVecSlotMap = std::map<std::string, Delegated<int, std::vector<std::string> &>>;
using StrSlotMap = std::map<std::string, Delegated<const std::string &>>;
using IntSlotMap = std::map<std::string, Delegated<int>>;
using SlotMap = std::map<std::string, Delegated<>>;

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

	// each control registers a ControlCb
	// which other controls can signal to focus(activate) it
	struct ControlCb
	{
		GuiPage * page = 0;
		GuiControl * control = 0;
		void call() { assert(page && control); page->SetActiveControl(*control); };
	};
	// allow a conrol to signal slots with an extra parameter
	struct SignalVal
	{
		std::string value;
		Delegated<const std::string &> delegate;
		void call() { delegate(value); };
	};
	// allow a control list to signal slots with an extra parameter
	struct SignalValn
	{
		std::string value;
		Delegated<int, const std::string&> delegate;
		void call(int n) { delegate(n, value); };
	};
	std::vector<ControlCb> control_set;		// control focus callbacks
	std::vector<SignalVal> action_set;		// action value callbacks
	std::vector<SignalValn> action_setn;	// action value nth callbacks
	Signald<> onfocus, oncancel;			// page action signals

	void Clear(SceneNode & parentnode);

	void Clear();

	void SetActiveControl(GuiControl & control);
};

#endif
