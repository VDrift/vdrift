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
class TEXTUREMANAGER;
class MODELMANAGER;
class CONFIG;
class FONT;

class GUIPAGE
{
public:
	GUIPAGE();

	~GUIPAGE();

	bool Load(
		const std::string & path,
		const std::string & texpath,
		const std::string & datapath,
		const std::string & texsize,
		const float screenhwratio,
		const CONFIG & controlsconfig,
		const FONT & fonts,
		const std::map <std::string, std::string> & languagemap,
		std::map <std::string, GUIOPTION> & optionmap,
		SCENENODE & parentnode,
		TEXTUREMANAGER & textures,
		MODELMANAGER & models,
		std::ostream & error_output);
  
	void SetVisible(SCENENODE & parent, const bool newvis);

	void SetAlpha(SCENENODE & parent, const float newalpha);

	///tell all child widgets to update to/from the option map
	void UpdateOptions(SCENENODE & parent, bool save_to, std::map<std::string, GUIOPTION> & optionmap, std::ostream & error_output);

	///update controlgrab widgets
	void UpdateControls(SCENENODE & parentnode, const CONFIG & controls, const FONT & font);

	///returns a list of actions that were generated
	std::list <std::pair <std::string, bool> > ProcessInput(
		SCENENODE & parent,
		bool movedown, bool moveup,
		float cursorx, float cursory,
		bool cursordown, bool cursorjustup,
		float screenhwratio);

	///tell all child widgets to do as update tick
	void Update(SCENENODE & parent, float dt);

	reseatable_reference <WIDGET_LABEL> GetLabel(const std::string & label_widget_name)
	{
		return label_widgets[label_widget_name];
	}
	
	reseatable_reference <WIDGET_BUTTON> GetButton(const std::string & widget_name)
	{
		return button_widgets[widget_name];
	}

	SCENENODE & GetNode(SCENENODE & parentnode)
	{
		return parentnode.GetNode(s);
	}

private:
	std::list <DERIVED <WIDGET> > widgets;
	std::map <std::string, reseatable_reference <WIDGET_LABEL> > label_widgets;
	std::map <std::string, reseatable_reference <WIDGET_BUTTON> > button_widgets;
	std::list <WIDGET_CONTROLGRAB *> controlgrabs;
	WIDGET_LABEL * tooltip_widget;
	keyed_container <SCENENODE>::handle s;
	std::string name;
	bool dialog;

	///hides some of the ugliness behind this method
	template <typename T>
	T * NewWidget()
	{
		widgets.push_back(DERIVED <WIDGET> (new T()));
		return (T*) widgets.back().Get();
	}

	void Clear(SCENENODE & parentnode);
};

#endif
