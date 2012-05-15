#ifndef _GUIPAGE_H
#define _GUIPAGE_H

#include "scenenode.h"

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
		SCENENODE & parentnode,
		ContentManager & content,
		std::ostream & error_output);

	void SetVisible(SCENENODE & parent, bool newvis);

	void SetAlpha(SCENENODE & parent, float newalpha);

	/// tell all child widgets to update to/from the option map
	void UpdateOptions(SCENENODE & parent, bool save_to, std::map<std::string, GUIOPTION> & optionmap, std::ostream & error_output);

	/// update controlgrab widgets
	void UpdateControls(SCENENODE & parentnode, const CONFIG & controls, const FONT & font);

	/// return a list of actions
	std::list <std::pair <std::string, bool> > ProcessInput(
		SCENENODE & parent,
		std::map<std::string, GUIOPTION> & optionmap,
		bool movedown, bool moveup,
		float cursorx, float cursory,
		bool cursordown, bool cursorjustup,
		float screenhwratio);

	/// tell all child widgets to do as update tick
	void Update(SCENENODE & parent, float dt);

	GUILABEL * GetLabel(const std::string & name);

	GUIBUTTON * GetButton(const std::string & name);

	SCENENODE & GetNode(SCENENODE & parentnode);

private:
	std::map <std::string, GUILABEL *> labels;
	std::map <std::string, GUIBUTTON *> buttons;
	std::vector <GUICONTROLGRAB *> controlgrabs;
	std::vector <GUIWIDGET *> widgets;
	GUILABEL * tooltip_widget;
	keyed_container <SCENENODE>::handle s;
	std::string name;
	bool dialog;

	void Clear(SCENENODE & parentnode);
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
