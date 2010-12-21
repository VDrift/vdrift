#ifndef _GUI_H
#define _GUI_H

#include "guipage.h"
#include "guioption.h"
#include "font.h"

#include <map>
#include <list>
#include <string>
#include <iostream>

class GUI
{
public:
	GUI();
	
	GUIPAGE & GetPage(const std::string & pagename)
	{
		assert(pages.find(pagename) != pages.end());
		return pages[pagename];
	}
	
	std::string GetActivePageName()
	{
		if (active_page == pages.end()) return "";
		return active_page->first;
	}
	
	std::string GetLastPageName()
	{
		if (last_active_page == pages.end()) return "";
		return last_active_page->first;
	}
	
	std::map<std::string, GUIOPTION> & GetOptionMap() {return optionmap;}
	
	SCENENODE & GetNode() {return node;}
	
	SCENENODE & GetPageNode(const std::string & pagename)
	{
		assert(pages.find(pagename) != pages.end());
		return pages[pagename].GetNode(node);
	}
	
	bool Load(
		const std::list <std::string> & pagelist,
		const std::map<std::string, std::list <std::pair <std::string, std::string> > > & valuelists, 
		const std::string & optionsfile,
		const std::string & carcontrolsfile,
		const std::string & menupath,
		const std::string & languagedir,
		const std::string & language,
		const std::string & texpath,
		const std::string & datapath,
		const std::string & texsize,
		const float screenhwratio,
		const std::map <std::string, FONT> & fonts,
		TEXTUREMANAGER & textures,
		MODELMANAGER & models,
		std::ostream & info_output,
		std::ostream & error_output);
	
	void UpdateControls(const std::string & pagename, const CONFIG & controlfile);
	
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
	
	bool Active() const {return (active_page != pages.end());}
	
	void SetInGame(bool value) {ingame = value;}
	
	///if settings_are_newer is true, then this function will revise its internal options
	/// to match the settings passed in.  otherwise, it'll operate the other way around
	void SyncOptions(
		const bool external_settings_are_newer,
		std::map <std::string, std::string> & external_options,
		std::ostream & error_output);

	void ReplaceOptionValues(
		const std::string & optionname,
		const std::list <std::pair <std::string, std::string> > & newvalues,
		std::ostream & error_output);
	
	/// returns false if the specified page/label does not exist
	bool SetLabelText(const std::string & page, const std::string & label, const std::string & text);

private:
	std::map<std::string, GUIPAGE> pages;
	std::map<std::string, GUIPAGE>::iterator active_page;
	std::map<std::string, GUIPAGE>::iterator last_active_page;
	std::map<std::string, GUIOPTION> optionmap;
	SCENENODE node;
	FONT font;
	float animation_counter;
	float animation_count_start;
	bool syncme; ///<true if a sync is needed
	bool syncme_from_external;
	bool control_load;
	bool ingame;
	
	///returns a string showing where the error occurred, or an empty string if no error
	bool LoadOptions(
		const std::string & optionfile,
		const std::map<std::string, std::list <std::pair <std::string, std::string> > > & valuelists,
		const std::map<std::string, std::string> & languagemap,
		std::ostream & error_output);
	
	///send options from the optionmap to the widgets
	void UpdateOptions(std::ostream & error_output);
};

#endif
