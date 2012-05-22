#ifndef _GUIPAGE_H
#define _GUIPAGE_H

#include "scenenode.h"
#include "signalslot.h"

#include <map>
#include <list>
#include <vector>
#include <string>
#include <ostream>

class GUILABEL;
class GUIBUTTON;
class GUICONTROLGRAB;
class GUIOPTION;
class GUIWIDGET;
class GUICONTROL;
class CONFIG;
class FONT;
class PATHMANAGER;
class ContentManager;

class GUIPAGE
{
public:
	GUIPAGE();

	~GUIPAGE();

	bool Load(
		const std::string & path,
		const std::string & texpath,
		const PATHMANAGER & pathmanager,
		const float screenhwratio,
		const CONFIG & controlsconfig,
		const FONT & fonts,
		const std::map <std::string, std::string> & languagemap,
		std::map <std::string, GUIOPTION> & optionmap,
		std::map <std::string, Slot0*> actionmap,
		SCENENODE & parentnode,
		ContentManager & content,
		std::ostream & error_output);

	void SetVisible(SCENENODE & parent, bool value);

	void SetAlpha(SCENENODE & parent, float value);

	/// update controlgrab widgets
	void UpdateControls(SCENENODE & parent, const CONFIG & controls, const FONT & font);

	/// execute game actions and update gui options
	void ProcessInput(
		SCENENODE & parent,
		float cursorx, float cursory,
		bool cursordown, bool cursorjustup,
		bool moveleft, bool moveright,
		bool moveup, bool movedown,
		bool select, bool cancel,
		float screenhwratio);

	/// tell all child widgets to do as update tick
	void Update(SCENENODE & parent, float dt);

	GUILABEL * GetLabel(const std::string & name);

	GUIBUTTON * GetButton(const std::string & name);

	SCENENODE & GetNode(SCENENODE & parentnode);

private:
	static const float inactive_alpha = 0.6;
	std::map <std::string, GUILABEL *> labels;
	std::map <std::string, GUIBUTTON *> buttons;
	std::vector <GUICONTROLGRAB *> controlgrabs;	// input edit widgets
	std::vector <GUICONTROL *> awidgets;			// active widgets (process input)
	std::vector <GUIWIDGET *> pwidgets;				// passive widgets
	GUICONTROL * default_widget;					// default active widget
	GUICONTROL * active_widget;						// current active widget
	GUICONTROL * active_widget_next;				// next active widget
	GUILABEL * tooltip_widget;						// tooltip, hardcoded
	keyed_container <SCENENODE>::handle s;
	std::string name;
	bool dialog;

	struct WIDGETCB
	{
		GUIPAGE * page;
		GUICONTROL * widget;
		Slot0 action;

		WIDGETCB();
		WIDGETCB(const WIDGETCB & other);
		WIDGETCB & operator=(const WIDGETCB & other);
		void call();
	};
	std::vector<WIDGETCB> widget_activate;			// active widget actions
	Signal0 oncancel;								// page cancel action

	void Clear(SCENENODE & parentnode);

	void SetActiveWidget(GUICONTROL & widget);
};

inline GUILABEL * GUIPAGE::GetLabel(const std::string & name)
{
	std::map <std::string, GUILABEL*>::const_iterator i = labels.find(name);
	if (i != labels.end())
		return i->second;
	return 0;
}

inline GUIBUTTON * GUIPAGE::GetButton(const std::string & name)
{
	std::map <std::string, GUIBUTTON*>::const_iterator i = buttons.find(name);
	if (i != buttons.end())
		return i->second;
	return 0;
}

inline SCENENODE & GUIPAGE::GetNode(SCENENODE & parentnode)
{
	return parentnode.GetNode(s);
}

#endif
