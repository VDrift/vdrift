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

class SCENENODE;
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
	SCENENODE * s;
	
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
	
	void Clear()
	{
		controlgrabs.clear();
		tooltip_widget = NULL;
		fontmap = NULL;
		dialog = false;
		widgets.clear();
		if (s)
			s->Clear();
		s = NULL;
	}
	
public:
	GUIPAGE() : tooltip_widget(NULL),fontmap(NULL),s(NULL),dialog(false) {}
	
	bool Load(const std::string & path, const std::string & texpath, const std::string & datapath,
		  CONFIGFILE & controlsconfig, SCENENODE * s, std::map<std::string, TEXTURE_GL> & textures,
    		  std::map <std::string, FONT> & fonts, std::map<std::string, GUIOPTION> & optionmap,
		  float screenhwratio, const std::string & texsize, std::ostream & error_output, bool reloadcontrolsonly=false);
	
	void SetVisible(const bool newvis)
	{
		for (std::list <DERIVED <WIDGET> >::iterator i = widgets.begin(); i != widgets.end(); i++)
		{
			(*i)->SetVisible(newvis);
		}
	}
	
	void SetAlpha(const float newalpha)
	{
		for (std::list <DERIVED <WIDGET> >::iterator i = widgets.begin(); i != widgets.end(); i++)
		{
			(*i)->SetAlpha(newalpha);
		}
	}
	
	///tell all child widgets to update to/from the option map
	void UpdateOptions(bool save_to_options, std::map<std::string, GUIOPTION> & optionmap, std::ostream & error_output)
	{
		for (std::list <DERIVED <WIDGET> >::iterator i = widgets.begin(); i != widgets.end(); i++)
		{
			(*i)->UpdateOptions(save_to_options, optionmap, error_output);
		}
	}
	
	///returns a list of actions that were generated
	std::list <std::pair <std::string, bool> > ProcessInput(bool movedown, bool moveup, float cursorx, float cursory,
			bool cursordown, bool cursorjustup, float screenhwratio);
			
	///tell all child widgets to do as update tick
	void Update(float dt)
	{
		for (std::list <DERIVED <WIDGET> >::iterator i = widgets.begin(); i != widgets.end(); i++)
		{
			(*i)->Update(dt);
		}
	}
	
	reseatable_reference <WIDGET_LABEL> GetLabel(const std::string & label_widget_name)
	{
		return label_widgets[label_widget_name];
	}
};

#endif
