#ifndef _GUIPAGE_H
#define _GUIPAGE_H

#include <map>
#include <list>
#include <string>
#include <iostream>

#include "derived.h"
#include "widget.h"
#include "texture.h"
#include "font.h"
#include "guioption.h"
#include "scenegraph.h"
#include "configfile.h"
#include "reseatable_reference.h"

class WIDGET_LABEL;
class WIDGET_CONTROLGRAB;

class GUIPAGE
{
private:
	std::list <DERIVED <WIDGET> > widgets;
	std::map <std::string, reseatable_reference <WIDGET_LABEL> > label_widgets;
	std::list <WIDGET_CONTROLGRAB *> controlgrabs;
	WIDGET_LABEL * tooltip_widget;
	std::map <std::string, FONT> * fontmap;
	keyed_container <SCENENODE>::handle s;
	
	bool dialog;
	
	///hides some of the ugliness behind this method
	template <typename T>
	T * NewWidget()
	{
		widgets.push_back(DERIVED <WIDGET> (new T()));
		return (T*) widgets.back().Get();
	}
	
	bool EnsureTextureIsLoaded(const std::string & texname, const std::string & texpath, std::map<std::string, TEXTURE_GL> & textures, const std::string & texsize, std::ostream & error_output)
	{
		if (!textures[texname].Loaded())
		{
			TEXTUREINFO texinfo(texpath + "/" + texname);
			texinfo.SetMipMap(false);
			texinfo.SetRepeat(false, false);
			if (!textures[texname].Load(texinfo, error_output, texsize)) return false;
		}
		
		return true;
	}
	
	void Clear(SCENENODE & parentnode)
	{
		controlgrabs.clear();
		tooltip_widget = NULL;
		fontmap = NULL;
		dialog = false;
		widgets.clear();
		if (s.valid())
		{
			SCENENODE & sref = parentnode.GetNode(s);
			sref.Clear();
		}
		s.invalidate();
	}
	
public:
	GUIPAGE() : tooltip_widget(NULL),fontmap(NULL),dialog(false) {}
	
	SCENENODE & GetNode(SCENENODE & parentnode)
	{
		return parentnode.GetNode(s);
	}
	
	bool Load(const std::string & path, const std::string & texpath, const std::string & datapath,
		  CONFIGFILE & controlsconfig, SCENENODE & parentnode, std::map<std::string, TEXTURE_GL> & textures,
    		  std::map <std::string, FONT> & fonts, std::map<std::string, GUIOPTION> & optionmap,
		  float screenhwratio, const std::string & texsize, std::ostream & error_output, bool reloadcontrolsonly=false);
		  
	void SetVisible(SCENENODE & parent, const bool newvis)
	{
		SCENENODE & sref = GetNode(parent);
		for (std::list <DERIVED <WIDGET> >::iterator i = widgets.begin(); i != widgets.end(); ++i)
		{
			(*i)->SetVisible(sref, newvis);
		}
	}
	
	void SetAlpha(SCENENODE & parent, const float newalpha)
	{
		SCENENODE & sref = parent.GetNode(s);
		for (std::list <DERIVED <WIDGET> >::iterator i = widgets.begin(); i != widgets.end(); ++i)
		{
			(*i)->SetAlpha(sref, newalpha);
		}
	}
	
	///tell all child widgets to update to/from the option map
	void UpdateOptions(SCENENODE & parent, bool save_to_options, std::map<std::string, GUIOPTION> & optionmap, std::ostream & error_output)
	{
		SCENENODE & sref = parent.GetNode(s);
		for (std::list <DERIVED <WIDGET> >::iterator i = widgets.begin(); i != widgets.end(); ++i)
		{
			(*i)->UpdateOptions(sref, save_to_options, optionmap, error_output);
		}
	}
	
	///returns a list of actions that were generated
	std::list <std::pair <std::string, bool> > ProcessInput(SCENENODE & parent, bool movedown, bool moveup, float cursorx, float cursory,
			bool cursordown, bool cursorjustup, float screenhwratio);
			
	///tell all child widgets to do as update tick
	void Update(SCENENODE & parent, float dt)
	{
		SCENENODE & sref = parent.GetNode(s);
		for (std::list <DERIVED <WIDGET> >::iterator i = widgets.begin(); i != widgets.end(); ++i)
		{
			(*i)->Update(sref, dt);
		}
	}
	
	reseatable_reference <WIDGET_LABEL> GetLabel(const std::string & label_widget_name)
	{
		return label_widgets[label_widget_name];
	}
};

#endif
