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
private:
	struct PAGEINFO
	{
		GUIPAGE page;
		keyed_container <SCENENODE>::handle node;
	};
	
	std::map<std::string, PAGEINFO> pages;
	std::map<std::string, PAGEINFO>::iterator active_page;
	std::map<std::string, PAGEINFO>::iterator last_active_page;
	std::map<std::string, TEXTURE> textures;
	std::map<std::string, GUIOPTION> optionmap;
	SCENENODE node;
	float animation_counter;
	float animation_count_start;
	bool syncme; ///<true if a sync is needed
	bool syncme_from_external; ///<true if an OK button was pressed
	bool control_load;
	bool ingame;
	
	bool LoadPage(
		const std::string & pagename,
		const std::string & path,
		const std::string & texpath,
		const std::string & datapath,
		CONFIGFILE & carcontrolsfile,
		SCENENODE & scenenode,
		std::map <std::string, FONT> & fonts,
		std::map<std::string, GUIOPTION> & optionmap,
		float screenhwratio,
		const std::string & texsize,
		MANAGER<TEXTURE, TEXTUREINFO> & texturemanager,
		std::ostream & error_output)
	{
		PAGEINFO & p = pages[pagename];
		if (!p.node.valid())
		{
			p.node = scenenode.AddNode();
		}
		SCENENODE & pnoderef = scenenode.GetNode(p.node);
		if (!p.page.Load(
				path+"/"+pagename, texpath, datapath,
				carcontrolsfile, pnoderef, fonts,
				optionmap, screenhwratio,
				texsize, texturemanager,
				error_output))
		{
			return false;
		}
		else
		{
			return true;
		}
		
	}
	
	///returns a string showing where the error occurred, or an empty string if no error
	std::string LoadOptions(const std::string & optionfile, const std::map<std::string, std::list <std::pair <std::string, std::string> > > & valuelists, std::ostream & error_output);
	
	///send options from the optionmap to the widgets
	void UpdateOptions(std::ostream & error_output)
	{
		for (std::map<std::string, PAGEINFO>::iterator i = pages.begin(); i != pages.end(); ++i)
		{
			i->second.page.UpdateOptions(node.GetNode(i->second.node), false, optionmap, error_output);
		}
	}

public:
	GUI() : animation_counter(0),animation_count_start(0),syncme(false),syncme_from_external(false),control_load(false),ingame(false) {active_page = last_active_page = pages.end();}
	
	GUIPAGE & GetPage(const std::string & pagename)
	{
		assert(pages.find(pagename) != pages.end());
		return pages[pagename].page;
	}
	
	std::string GetActivePageName() {if (active_page == pages.end()) return ""; else return active_page->first;}
	std::string GetLastPageName() {if (last_active_page == pages.end()) return ""; else return last_active_page->first;}
	
	std::map<std::string, GUIOPTION> & GetOptionMap() {return optionmap;}
	
	SCENENODE & GetNode() {return node;}
	
	SCENENODE & GetPageNode(const std::string & pagename)
	{
		assert(pages.find(pagename) != pages.end());
		return node.GetNode(pages[pagename].node);
	}
	
	bool Load(
		const std::list <std::string> & pagelist,
		const std::map<std::string, 
		std::list <std::pair <std::string, std::string> > > & valuelists, 
		const std::string & optionsfile,
		const std::string & carcontrolsfile,
  		const std::string & menupath,
  		const std::string & texpath, 
		const std::string & datapath,
		std::map <std::string, FONT> & fonts, 
		float screenhwratio,
		const std::string & texsize,
		MANAGER<TEXTURE, TEXTUREINFO> & textures,
		std::ostream & info_output,
		std::ostream & error_output)
	{
		std::string optionresult(LoadOptions(optionsfile, valuelists, error_output));
		if (!optionresult.empty())
		{
			error_output << "Options file loading failed here: " << optionresult << std::endl;
			return false;
		}
		
		bool foundmain = false;
		
		CONFIGFILE controlsconfig;
		controlsconfig.Load(carcontrolsfile); //pre-load controls file so that each individual widget doesn't have to reload it
		
		for (std::list <std::string>::const_iterator i = pagelist.begin(); i != pagelist.end(); ++i)
		{
			if (*i != "SConscript")
			{
				if (!LoadPage(*i, menupath, texpath, datapath, controlsconfig, node, fonts, optionmap, screenhwratio, texsize, textures, error_output))
				{
					error_output << "Error loading GUI page: " << menupath << "/" << *i << std::endl;
					return false;
				}
				if (*i == "Main")
					foundmain = true;
			}
		}
		
		if (!foundmain)
		{
			error_output << "Couldn't find GUI Main menu in: " << menupath << std::endl;
			return false;
		}
		
		info_output << "Loaded GUI successfully" << std::endl;
		
		//start out with everything invisible
		DeactivateAll();
		
		return true;
	}
	
	void DeactivateAll()
	{
		for (std::map<std::string, PAGEINFO>::iterator i = pages.begin(); i != pages.end(); ++i)
		{
			i->second.page.SetVisible(node.GetNode(i->second.node), false);
		}
		
		active_page = pages.end();
	}
	
	///ensure the last last active page is invisible, update options from the last page and start fading it out, and load options into the new page and fade it in
	void ActivatePage(const std::string & pagename, float activation_time, std::ostream & error_output, bool save_options=false)
	{
		if (last_active_page != pages.end())
			last_active_page->second.page.SetVisible(node.GetNode(last_active_page->second.node), false);
		
		bool ok = save_options;
		syncme = true;
		syncme_from_external = !ok;
		if (!ok)
			control_load = true;
		
		//save options from widgets to internal optionmap array, which will then later be saved to the game options via SyncOptions
		if (active_page != pages.end() && ok)
			active_page->second.page.UpdateOptions(node.GetNode(active_page->second.node), true, optionmap, error_output);
		
		//purge controledit parameters from the optionmap
		optionmap.erase("controledit.analog.deadzone");
		optionmap.erase("controledit.analog.gain");
		optionmap.erase("controledit.analog.exponent");
		optionmap.erase("controledit.button.held_once");
		optionmap.erase("controledit.button.up_down");
		
		//if (active_page != pages.end()) active_page->second.page.UpdateOptions(ok, optionmap, error_output);
		
		last_active_page = active_page;
		active_page = pages.find(pagename);
		assert(active_page != pages.end());
		active_page->second.page.SetVisible(node.GetNode(active_page->second.node), true);
		//std::cout << "Moving to page " << active_page->first << std::endl;
		//active_page->second.page.UpdateOptions(false, optionmap, error_output);
		animation_counter = animation_count_start = activation_time;
		
		//std::cout << "Done activating page" << std::endl;
	}
	
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
	std::list <std::string> ProcessInput(bool movedown, bool moveup, float cursorx, float cursory,
					     bool cursordown, bool cursorjustup, float screenhwratio, std::ostream & error_output)
	{
		std::list <std::pair <std::string, bool> > actions;
		
		if (active_page != pages.end())
		{
			actions = active_page->second.page.ProcessInput(node.GetNode(active_page->second.node),
					movedown, moveup, cursorx, cursory,
					cursordown, cursorjustup, screenhwratio);
		}
		
		std::list <std::string> gameactions;
		
		std::string newpage;
		bool save_options = false;
		
		//process resulting actions
		for (std::list <std::pair <std::string, bool> >::iterator i = actions.begin(); i != actions.end(); ++i)
		{
			std::string actionname = i->first;
			
			//if the action is the same as a page name, just switch to that page
			//decide which main page to use
			if (actionname == "Main")
			{
				if (ingame)
				{
					actionname = "InGameMenu";
				}
			}
			
			if (pages.find(actionname) != pages.end())
			{
				newpage = actionname;
				save_options = i->second;
				if (!newpage.empty())
					ActivatePage(newpage, 0.25, error_output, save_options);
			}
			else
			{
				gameactions.push_back(actionname);
				if (i->second)
				{
					//std::cout << "Processing input" << std::endl;
					active_page->second.page.UpdateOptions(node.GetNode(active_page->second.node), true, optionmap, error_output);
					//std::cout << "Done processing options" << std::endl;
					syncme = true;
				}
			}
		}
		
		return gameactions;
	}
	
	void Update(float dt)
	{
		animation_counter -= dt;
		if (animation_counter < 0)
			animation_counter = 0;
		
		if (active_page != pages.end())
		{
			//ease curve: 3*p^2-2*p^3
			float p = 1.0-animation_counter/animation_count_start;
			active_page->second.page.SetAlpha(node.GetNode(active_page->second.node), 3*p*p-2*p*p*p);
		}
		
		if (last_active_page != pages.end())
		{
			if (animation_counter > 0)
			{
				float p = animation_counter/animation_count_start;
				last_active_page->second.page.SetAlpha(node.GetNode(last_active_page->second.node), 3*p*p-2*p*p*p);
			}
			else
			{
				last_active_page->second.page.SetVisible(node.GetNode(last_active_page->second.node), false);
				last_active_page = pages.end();
			}
		}
		
		if (active_page != pages.end())
			active_page->second.page.Update(node.GetNode(active_page->second.node), dt);
		if (last_active_page != pages.end())
			last_active_page->second.page.Update(node.GetNode(last_active_page->second.node), dt);
	}
	
	bool Active() const
	{
		return (active_page != pages.end());
	}
	
	///if settings_are_newer is true, then this function will revise its internal options
	/// to match the settings passed in.  otherwise, it'll operate the other way around
	void SyncOptions(const bool external_settings_are_newer, std::map <std::string, std::string> & external_options, std::ostream & error_output)
	{
		//std::cout << "Syncing options: " << external_settings_are_newer << ", " << syncme_from_external << std::endl;
		
		for (std::map <std::string, std::string>::iterator i = external_options.begin(); i != external_options.end(); ++i)
		{
			if (external_settings_are_newer || syncme_from_external)
			{
				std::map<std::string, GUIOPTION>::iterator option = optionmap.find(i->first);
				if (option == optionmap.end())
				{
					//error_output << "External option \"" << i->first << "\" has no internal GUI counterpart" << std::endl;
				}
				else //if (i->first.find("controledit.") != 0)
				{
					//std::cout << i->first << std::endl;
					//if (option->first == "game.car_paint") error_output << "Setting GUI option \"" << option->first << "\" to GAME value \"" << i->second << "\"" << std::endl;
					if (!option->second.SetCurrentValue(i->second))
						error_output << "Error setting GUI option \"" << option->first << "\" to GAME value \"" << i->second << "\"" << std::endl;
				}
				//else std::cout << "Ignoring controledit value: " << i->first << std::endl;
			}
			else
			{
				std::map<std::string, GUIOPTION>::iterator option = optionmap.find(i->first);
				if (option == optionmap.end())
				{
					//error_output << "External option \"" << i->first << "\" has no internal GUI counterpart" << std::endl;
				}
				else
				{
					//if (option->first == "game.car_paint") error_output << "Setting GAME option \"" << i->first << "\" to GUI value \"" << option->second.GetCurrentStorageValue() << "\"" << std::endl;
					i->second = option->second.GetCurrentStorageValue();
				}
			}
		}
		
		if (external_settings_are_newer)
			UpdateOptions(error_output);
		
		if (syncme_from_external)
		{
			//UpdateOptions(error_output);
			if (last_active_page != pages.end())
				last_active_page->second.page.UpdateOptions(node.GetNode(last_active_page->second.node), false, optionmap, error_output);
			//std::cout << "About to update" << std::endl;
			if (active_page != pages.end())
				active_page->second.page.UpdateOptions(node.GetNode(active_page->second.node), false, optionmap, error_output);
			//std::cout << "Done updating" << std::endl;
		}
		
		syncme = false;
		
		//std::cout << "Done syncing options" << std::endl;
	}

	void SetInGame ( bool value )
	{
		ingame = value;
	}
	
	void ReplaceOptionMapValues(const std::string & optionname, std::list <std::pair <std::string, std::string> > & newvalues, std::ostream & error_output)
	{
		//std::cout << "Replacing option map values" << std::endl;
		
		if (optionmap.find(optionname) == optionmap.end())
		{
			error_output << "Can't find option named " << optionname << " when replacing optionmap values" << std::endl;
		}
		else
		{
			optionmap[optionname].ReplaceValues(newvalues);
			UpdateOptions(error_output);
		}
		
		//std::cout << "Done replacing option map values" << std::endl;
	}
	
};

#endif
