#ifndef _GUIPAGE_H
#define _GUIPAGE_H

#include "derived.h"
#include "widget.h"
#include "font.h"
#include "guioption.h"
#include "scenenode.h"
#include "reseatable_reference.h"

#include <map>
#include <list>
#include <string>
#include <iostream>

class WIDGET_LABEL;
class WIDGET_CONTROLGRAB;
class MODELMANAGER;
class CONFIG;

class GUIPAGE
{
public:
	GUIPAGE();
	
	bool Load(
		const std::string & path,
		const std::string & texpath,
		const std::string & datapath,
		const std::string & texsize,
		const float screenhwratio,
		const CONFIG & controlsconfig,
		const std::map <std::string, FONT> & fonts,
		std::map <std::string, GUIOPTION> & optionmap,
		SCENENODE & parentnode,
		TEXTUREMANAGER & textures,
		MODELMANAGER & models,
		std::ostream & error_output,
		bool reloadcontrolsonly = false);
  	
	void SetVisible(SCENENODE & parent, const bool newvis);
	
	void SetAlpha(SCENENODE & parent, const float newalpha);
	
	///tell all child widgets to update to/from the option map
	void UpdateOptions(SCENENODE & parent, bool save_to, std::map<std::string, GUIOPTION> & optionmap, std::ostream & error_output);
	
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
	
	SCENENODE & GetNode(SCENENODE & parentnode)
	{
		return parentnode.GetNode(s);
	}
	
private:
	std::list <DERIVED <WIDGET> > widgets;
	std::map <std::string, reseatable_reference <WIDGET_LABEL> > label_widgets;
	std::list <WIDGET_CONTROLGRAB *> controlgrabs;
	WIDGET_LABEL * tooltip_widget;
	const std::map <std::string, FONT> * fontmap;
	keyed_container <SCENENODE>::handle s;
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
