#include "gui.h"
#include "config.h"

#include <map>
using std::map;

#include <string>
using std::string;

#include <iostream>
using std::endl;

#include <list>
using std::list;

#include <sstream>
using std::stringstream;

GUI::GUI() : 
	animation_counter(0),
	animation_count_start(0),
	syncme(false),
	syncme_from_external(false),
	control_load(false),
	ingame(false)
{
	active_page = last_active_page = pages.end();
}

bool GUI::Load(
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
	std::ostream & error_output)
{
	std::string optionresult = LoadOptions(optionsfile, valuelists, error_output);
	if (!optionresult.empty())
	{
		error_output << "Options file loading failed here: " << optionresult << std::endl;
		return false;
	}
	
	bool foundmain = false;
	
	CONFIG controlsconfig;
	controlsconfig.Load(carcontrolsfile); //pre-load controls file so that each individual widget doesn't have to reload it
	
	for (std::list <std::string>::const_iterator i = pagelist.begin(); i != pagelist.end(); ++i)
	{
		if (*i != "SConscript")
		{
			if (!LoadPage(*i,
				menupath, texpath, datapath,
				texsize, screenhwratio,
				controlsconfig, fonts,
				optionmap, node,
				textures, models,
				error_output))
			{
				error_output << "Error loading GUI page: " << menupath << "/" << *i << std::endl;
				return false;
			}
			if (*i == "Main")
			{
				foundmain = true;
			}
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

void GUI::DeactivateAll()
{
	for (std::map<std::string, PAGEINFO>::iterator i = pages.begin(); i != pages.end(); ++i)
	{
		i->second.page.SetVisible(node.GetNode(i->second.node), false);
	}
	
	active_page = pages.end();
}

std::list <std::string> GUI::ProcessInput(
	bool movedown, bool moveup,
	float cursorx, float cursory,
	bool cursordown, bool cursorjustup,
	float screenhwratio,
	std::ostream & error_output)
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

void GUI::Update(float dt)
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

void GUI::SyncOptions(
	const bool external_settings_are_newer,
	std::map <std::string, std::string> & external_options,
	std::ostream & error_output)
{
	//std::cout << "Syncing options: " << external_settings_are_newer << ", " << syncme_from_external << std::endl;
	
	for (std::map <std::string, std::string>::iterator i = external_options.begin(); i != external_options.end(); ++i)
	{
		if (external_settings_are_newer || syncme_from_external)
		{
			std::map<std::string, GUIOPTION>::iterator option = optionmap.find(i->first);
			if (option == optionmap.end())
			{
				error_output << "External option \"" << i->first << "\" has no internal GUI counterpart" << std::endl;
			}
			else //if (i->first.find("controledit.") != 0)
			{
				//std::cout << i->first << std::endl;
				//if (option->first == "game.player_paint") error_output << "Setting GUI option \"" << option->first << "\" to GAME value \"" << i->second << "\"" << std::endl;
				//if (option->first == "game.player_color") error_output << "Setting GUI option \"" << option->first << "\" to GAME value \"" << i->second << "\"" << std::endl;
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

void GUI::ReplaceOptionValues(
	const std::string & optionname,
	const std::list <std::pair <std::string, std::string> > & newvalues,
	std::ostream & error_output)
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

void GUI::ActivatePage(
	const std::string & pagename,
	float activation_time,
	std::ostream & error_output,
	bool save_options)
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

bool GUI::LoadPage(
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
	std::ostream & error_output)
{
	PAGEINFO & p = pages[pagename];
	if (!p.node.valid())
	{
		p.node = scenenode.AddNode();
	}
	
	if (!p.page.Load(
			path+"/"+pagename,
			texpath,
			datapath,
			texsize,
			screenhwratio,
			carcontrolsfile,
			fonts,
			optionmap,
			scenenode.GetNode(p.node),
			textures,
			models,
			error_output))
	{
		return false;
	}
	else
	{
		return true;
	}
	
}

string GUI::LoadOptions(
	const std::string & optionfile,
	const std::map<std::string, std::list <std::pair <std::string, std::string> > > & valuelists,
	std::ostream & error_output)
{
	CONFIG opt;
	if (!opt.Load(optionfile))
	{
		error_output << "Can't find options file: " << optionfile << endl;
		return "File loading";
	}
	
	//opt.DebugPrint(error_output);
	
	for (CONFIG::const_iterator i = opt.begin(); i != opt.end(); ++i)
	{
		if (!i->first.empty())
		{
			string cat, name, defaultval, values, text, desc, type;
			if (!opt.GetParam(i, "cat", cat)) return i->first+".cat";
			if (!opt.GetParam(i, "name", name)) return i->first+".name";
			if (!opt.GetParam(i, "default", defaultval)) return i->first+".default";
			if (!opt.GetParam(i, "values", values)) return i->first+".values";
			if (!opt.GetParam(i, "title", text)) return i->first+".title";
			if (!opt.GetParam(i, "desc", desc)) return i->first+".desc";
			if (!opt.GetParam(i, "type", type)) return i->first+".type";
			
			float min(0),max(1);
			bool percentage(true);
			opt.GetParam(i, "min",min);
			opt.GetParam(i, "max",max);
			opt.GetParam(i, "percentage",percentage);
			
			string optionname = cat+"."+name;
			GUIOPTION & option = optionmap[optionname];
			
			option.SetInfo(text, desc, type);
			option.SetMinMaxPercentage(min, max, percentage);
			
			//different ways to populate the options
			if (values == "list")
			{
				int valuenum;
				if (!opt.GetParam(i, "num_vals", valuenum)) return i->first+".num_vals";
				
				for (int n = 0; n < valuenum; n++)
				{
					stringstream tstr;
					tstr.width(2);
					tstr.fill('0');
					tstr << n;
					
					string displaystr, storestr;
					if (!opt.GetParam(i, "opt"+tstr.str(), displaystr)) return i->first+".opt"+tstr.str();
					if (!opt.GetParam(i, "val"+tstr.str(), storestr)) return i->first+".val"+tstr.str();
					
					option.Insert(storestr, displaystr);
				}
			}
			else if (values == "bool")
			{
				string truestr, falsestr;
				if (!opt.GetParam(i, "true", truestr)) return i->first+".true";
				if (!opt.GetParam(i, "false", falsestr)) return i->first+".false";
				
				option.Insert("true", truestr);
				option.Insert("false", falsestr);
			}
			else if (values == "ip_valid")
			{
				
			}
			else if (values == "port_valid")
			{
				
			}
			else if (values == "float")
			{
				
			}
			else if (values == "string")
			{
				
			}
			else //assume it's "values", meaning the GAME populates the values
			{
				std::map<std::string, std::list <std::pair<std::string,std::string> > >::const_iterator vlist = valuelists.find(values);
				if (vlist == valuelists.end())
				{
					error_output << "Can't find value type \"" << values << "\" in list of GAME values" << endl;
					return "GAME valuelist";
				}
				else
				{
					//std::cout << "Populating values:" << endl;
					for (std::list <std::pair<std::string,std::string> >::const_iterator n = vlist->second.begin(); n != vlist->second.end(); n++)
					{
						//std::cout << "\t" << n->second << endl;
						option.Insert(n->first, n->second);
					}
				}
			}
			
			option.SetCurrentValue(defaultval);
		}
	}
	
	UpdateOptions(error_output);
	
	return "";
}

void GUI::UpdateOptions(std::ostream & error_output)
{
	bool save_to = false;
	for (std::map<std::string, PAGEINFO>::iterator i = pages.begin(); i != pages.end(); ++i)
	{
		i->second.page.UpdateOptions(node.GetNode(i->second.node), save_to, optionmap, error_output);
	}
}
