#ifndef _GUIPAGE_H
#define _GUIPAGE_H

#include "scenenode.h"
#include "signalslot.h"

#include <map>
#include <list>
#include <vector>
#include <string>
#include <ostream>

class GUIWIDGET;
class GUICONTROL;
class GUILABEL;
class CONFIG;
class FONT;
class PATHMANAGER;
class ContentManager;

typedef std::map <std::string, Signal1<const std::string &>*> VSIGNALMAP;
typedef std::map <std::string, Slot1<const std::string &>*> VACTIONMAP;
typedef std::map <std::string, Slot0*> ACTIONMAP;

class GUIPAGE
{
public:
	GUIPAGE();

	~GUIPAGE();

	bool Load(
		const std::string & path,
		const std::string & texpath,
		const float screenhwratio,
		const FONT & font,
		const std::map <std::string, std::string> & languagemap,
		VSIGNALMAP vsignalmap,
		VACTIONMAP vactionmap,
		ACTIONMAP actionmap,
		SCENENODE & parentnode,
		ContentManager & content,
		std::ostream & error_output);

	void SetVisible(SCENENODE & parent, bool value);

	void SetAlpha(SCENENODE & parent, float value);

	/// execute game actions and update gui options
	void ProcessInput(
		float cursorx, float cursory,
		bool cursormoved, bool cursordown, bool cursorjustup,
		bool moveleft, bool moveright,
		bool moveup, bool movedown,
		bool select, bool cancel);

	/// tell all child widgets to do as update tick
	void Update(SCENENODE & parent, float dt);

	void SetLabelText(const std::map<std::string, std::string> & label_text);

	GUILABEL * GetLabel(const std::string & name);

	SCENENODE & GetNode(SCENENODE & parentnode);

private:
	std::map <std::string, GUILABEL *> labels;
	std::vector <GUICONTROL *> controls;			// active widgets (process input)
	std::vector <GUIWIDGET *> widgets;				// passive widgets
	GUICONTROL * default_control;					// default active control
	GUICONTROL * active_control;					// current active control
	keyed_container <SCENENODE>::handle s;
	std::string name;

	struct ControlFocusCb
	{
		GUIPAGE * page;
		GUICONTROL * control;
		Slot0 action;

		ControlFocusCb();
		ControlFocusCb(const ControlFocusCb & other);
		ControlFocusCb & operator=(const ControlFocusCb & other);
		void call();
	};
	struct WidgetCb
	{
		float value;
		Delegate1<void, float> set;
		Slot0 action;

		WidgetCb();
		WidgetCb(const WidgetCb & other);
		WidgetCb & operator=(const WidgetCb & other);
		void call();
	};
	struct ActionCb
	{
		std::string value;
		Signal1<const std::string &> signal;
		Slot0 action;

		ActionCb();
		ActionCb(const ActionCb & other);
		ActionCb & operator=(const ActionCb & other);
		void call();
	};
	std::vector<ControlFocusCb> control_focus;		// control focus callbacks
	std::vector<WidgetCb> widget_set;				// widget property callbacks
	std::vector<ActionCb> action_set;				// action value callbacks
	Signal1<const std::string &> tooltip;			// tooltip signal
	Signal0 onfocus, oncancel;						// page actions

	void Clear(SCENENODE & parentnode);

	void SetActiveWidget(GUICONTROL & widget);
};

#endif
