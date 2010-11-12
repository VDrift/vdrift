#ifndef _GUI_H
#define _GUI_H

#include "guipage.h"
#include "texture.h"
#include "font.h"
#include "guioption.h"

#include <map>
#include <string>
#include <iostream>
#include <list>
#include <sstream>

class GUI
{
public:
	GUI();
	
	GUIPAGE & GetPage(const std::string & pagename)
	{
		assert(pages.find(pagename) != pages.end());
		return pages[pagename].page;
	}
	
	std::string GetActivePageName()
	{
		if (active_page == pages.end()) return "";
		else return active_page->first;
	}
	
	std::string GetLastPageName()
	{
		if (last_active_page == pages.end()) return "";
		else return last_active_page->first;
	}
	
	std::map<std::string, GUIOPTION> & GetOptionMap() {return optionmap;}
	
	SCENENODE & GetNode() {return node;}
	
	SCENENODE & GetPageNode(const std::string & pagename)
	{
		assert(pages.find(pagename) != pages.end());
		return node.GetNode(pages[pagename].node);
	}
	
	bool Load(
		const std::list <std::string> & pagelist,
		const std::map<std::string, std::list <std::pair <std::string, std::string> > > & valuelists, 
		const std::string & optionsfile,
		const std::string & carcontrolsfile,
		const std::string & menupath,
		const std::string & texpath, 
		const std::string & datapath,
		const std::string & texsize,
		const float screenhwratio,
		const std::map <std::string, FONT> & fonts,
		TEXTUREMANAGER & textures,
		MODELMANAGER & models,
		std::ostream & info_output,
		std::ostream & error_output);
	
	void DeactivateAll();
	
	/// ensure the last last active page is invisible, update options from the last page and start fading it out, and load options into the new page and fade it in
	void ActivatePage(
		const std::string & pagename,
		float activation_time,
		std::ostream & error_output,
		bool save_options = false);
	
	bool OptionsNeedSync() const {return syncme;}
	
	bool ControlsNeedLoading()
	{
		if (control_load)
		{
			control_load = false;
			return true;
		}
		else
			return false;
	}
	
	void SetControlsNeedLoading(bool newcl) {control_load = newcl;}
	
	///movedown and moveup are true when the user has navigated up or down with the keyboard
	/// or joystick, while the cursor variables are set for mouse navigation.
	/// returns a list of actions for processing by the game.
	std::list <std::string> ProcessInput(
		bool movedown, bool moveup,
		float cursorx, float cursory,
		bool cursordown, bool cursorjustup,
		float screenhwratio,
		std::ostream & error_output);
	
	void Update(float dt);
	
	bool Active() const
	{
		return (active_page != pages.end());
	}
	
	///if settings_are_newer is true, then this function will revise its internal options
	/// to match the settings passed in.  otherwise, it'll operate the other way around
	void SyncOptions(
		const bool external_settings_are_newer,
		std::map <std::string, std::string> & external_options,
		std::ostream & error_output);

	void SetInGame ( bool value )
	{
		ingame = value;
	}
	
	void ReplaceOptionValues(
		const std::string & optionname,
		const std::list <std::pair <std::string, std::string> > & newvalues,
		std::ostream & error_output);

private:
	struct PAGEINFO
	{
		GUIPAGE page;
		keyed_container <SCENENODE>::handle node;
	};
	
	std::map<std::string, PAGEINFO> pages;
	std::map<std::string, PAGEINFO>::iterator active_page;
	std::map<std::string, PAGEINFO>::iterator last_active_page;
	std::map<std::string, GUIOPTION> optionmap;
	SCENENODE node;
	float animation_counter;
	float animation_count_start;
	bool syncme; ///<true if a sync is needed
	bool syncme_from_external;
	bool control_load;
	bool ingame;
	
	bool LoadPage(
		const std::string & pagename,
		const std::string & path,
		const std::string & texpath,
		const std::string & datapath,
		const std::string & texsize,
		const float screenhwratio,
		const CONFIG & carcontrolsfile,
		const std::map <std::string, FONT> & fonts,
		std::map<std::string, GUIOPTION> & optionmap,
		SCENENODE & scenenode,
		TEXTUREMANAGER & textures,
		MODELMANAGER & models,
		std::ostream & error_output);
	
	///returns a string showing where the error occurred, or an empty string if no error
	std::string LoadOptions(
		const std::string & optionfile,
		const std::map<std::string, std::list <std::pair <std::string, std::string> > > & valuelists,
		std::ostream & error_output);
	
	///send options from the optionmap to the widgets
	void UpdateOptions(std::ostream & error_output);
};

#endif
