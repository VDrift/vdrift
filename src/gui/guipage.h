#ifndef _GUIPAGE_H
#define _GUIPAGE_H

#include "derived.h"
#include "widget.h"
#include "scenenode.h"
#include "reseatable_reference.h"

#include <map>
#include <list>
#include <string>
#include <ostream>

class WIDGET_LABEL;
class WIDGET_BUTTON;
class WIDGET_CONTROLGRAB;
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

	WIDGET_LABEL * GetLabel(const std::string & name);

	WIDGET_BUTTON * GetButton(const std::string & name);

	SCENENODE & GetNode(SCENENODE & parentnode);

private:
	std::map <std::string, WIDGET_LABEL *> labels;
	std::map <std::string, WIDGET_BUTTON *> buttons;
	std::vector <WIDGET_CONTROLGRAB *> controlgrabs;
	std::vector <WIDGET *> widgets;
	WIDGET_LABEL * tooltip_widget;
	keyed_container <SCENENODE>::handle s;
	std::string name;
	bool dialog;

	void Clear(SCENENODE & parentnode);
};

inline WIDGET_LABEL * GUIPAGE::GetLabel(const std::string & name)
{
	std::map <std::string, WIDGET_LABEL*>::const_iterator i = labels.find(name);
	if (i != labels.end())
		return i->second;
	return 0;
}

inline WIDGET_BUTTON * GUIPAGE::GetButton(const std::string & name)
{
	std::map <std::string, WIDGET_BUTTON*>::const_iterator i = buttons.find(name);
	if (i != buttons.end())
		return i->second;
	return 0;
}

inline SCENENODE & GUIPAGE::GetNode(SCENENODE & parentnode)
{
	return parentnode.GetNode(s);
}

#endif
