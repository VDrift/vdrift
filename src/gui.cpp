#include "gui.h"
#include "config.h"
#include "widget_controlgrab.h"

#include <sstream>

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
	std::ostream & error_output)
{
	// load language font
	CONFIG languageconfig;
	languageconfig.Load(datapath + "/" + languagedir + "/" + language + ".lng");
	
	std::string fontinfo, fonttex;
	if (!languageconfig.GetParam("font", "info", fontinfo, error_output)) return false;
	if (!languageconfig.GetParam("font", "texture", fonttex, error_output)) return false;
	
	std::string fontinfopath = datapath + "/" + languagedir + "/" + fontinfo;
	if (!font.Load(fontinfopath, languagedir, fonttex, texsize, textures, error_output)) return false;
	
	// load language map
	CONFIG::const_iterator section;
	std::map<std::string, std::string> languagemap;
	if (languageconfig.GetSection("strings", section))
	{
		languagemap = section->second;
	}
	//languageconfig.DebugPrint(std::clog);
	
	// controlgrab description strings translation (ugly hack)
	for (int i = 0; i < WIDGET_CONTROLGRAB::END_STR; ++i)
	{
		std::string & str = WIDGET_CONTROLGRAB::Str[i];
		std::map<std::string, std::string>::const_iterator li;
		if ((li = languagemap.find(str)) != languagemap.end()) str = li->second;
	}
	
	// load options
	if (!LoadOptions(optionsfile, valuelists, languagemap, error_output)) return false;
	
	bool foundmain = false;
	
	CONFIG controlsconfig;
	controlsconfig.Load(carcontrolsfile); //pre-load controls file so that each individual widget doesn't have to reload it
	
	for (std::list <std::string>::const_iterator i = pagelist.begin(); i != pagelist.end(); ++i)
	{
		const std::string & pagename = *i;
		
		if (pagename == "SConscript") continue;
		
		if (!pages[pagename].Load(
			menupath + "/" + pagename, texpath, datapath, texsize,
			screenhwratio, carcontrolsfile, font, languagemap, optionmap,
			node, textures, models, error_output))
		{
			error_output << "Error loading GUI page: " << menupath << "/" << *i << std::endl;
			return false;
		}
		
		if (pagename == "Main")
		{
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

void GUI::UpdateControls(const std::string & pagename, const CONFIG & controlfile)
{
	assert(pages.find(pagename) != pages.end());
	pages[pagename].UpdateControls(node, controlfile, font);
}

void GUI::DeactivateAll()
{
	for (std::map<std::string, GUIPAGE>::iterator i = pages.begin(); i != pages.end(); ++i)
	{
		i->second.SetVisible(node, false);
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
		actions = active_page->second.ProcessInput(
			node,
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
			{
				ActivatePage(newpage, 0.25, error_output, save_options);
			}
		}
		else
		{
			gameactions.push_back(actionname);
			if (i->second)
			{
				//std::cout << "Processing input" << std::endl;
				active_page->second.UpdateOptions(node, true, optionmap, error_output);
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
	if (animation_counter < 0) animation_counter = 0;
	
	if (active_page != pages.end())
	{
		//ease curve: 3*p^2-2*p^3
		float p = 1.0-animation_counter/animation_count_start;
		active_page->second.SetAlpha(node, 3*p*p-2*p*p*p);
	}
	
	if (last_active_page != pages.end())
	{
		if (animation_counter > 0)
		{
			float p = animation_counter/animation_count_start;
			last_active_page->second.SetAlpha(node, 3*p*p-2*p*p*p);
		}
		else
		{
			last_active_page->second.SetVisible(node, false);
			last_active_page = pages.end();
		}
	}
	
	if (active_page != pages.end())
	{
		active_page->second.Update(node, dt);
	}
	
	if (last_active_page != pages.end())
	{
		last_active_page->second.Update(node, dt);
	}
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
				//error_output << "External option \"" << i->first << "\" has no internal GUI counterpart" << std::endl;
			}
			else //if (i->first.find("controledit.") != 0)
			{
				//std::cout << i->first << std::endl;
				if (!option->second.SetCurrentValue(i->second))
				{
					option->second.SetToFirstValue();
					error_output << "Error setting GUI option \""
								<< option->first << "\" to GAME value \"" 
								<< i->second << "\" use first option \"" 
								<< option->second.GetCurrentStorageValue() 
								<< "\"" << std::endl;
				}
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
	{
		UpdateOptions(error_output);
	}
	
	if (syncme_from_external)
	{
		//UpdateOptions(error_output);
		if (last_active_page != pages.end())
		{
			last_active_page->second.UpdateOptions(node, false, optionmap, error_output);
		}
		
		//std::cout << "About to update" << std::endl;
		if (active_page != pages.end())
		{
			active_page->second.UpdateOptions(node, false, optionmap, error_output);
		}
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
	if (active_page == pages.find(pagename))
	{
		animation_counter = animation_count_start = activation_time; // fade it back in at least
		return;
	}
	
	if (last_active_page != pages.end())
	{
		last_active_page->second.SetVisible(node, false);
	}
	
	bool ok = save_options;
	syncme = true;
	syncme_from_external = !ok;
	if (!ok) control_load = true;
	
	//save options from widgets to internal optionmap array, which will then later be saved to the game options via SyncOptions
	if (active_page != pages.end() && ok)
	{
		active_page->second.UpdateOptions(node, true, optionmap, error_output);
	}
	
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
	active_page->second.SetVisible(node, true);
	//std::cout << "Moving to page " << active_page->first << std::endl;
	//active_page->second.page.UpdateOptions(false, optionmap, error_output);
	animation_counter = animation_count_start = activation_time;
	
	//std::cout << "Done activating page" << std::endl;
}

bool GUI::LoadOptions(
	const std::string & optionfile,
	const std::map<std::string, std::list <std::pair <std::string, std::string> > > & valuelists,
	const std::map<std::string, std::string> & languagemap,
	std::ostream & error_output)
{
	CONFIG opt;
	if (!opt.Load(optionfile))
	{
		error_output << "Can't find options file: " << optionfile << std::endl;
		return "File loading";
	}
	
	//opt.DebugPrint(error_output);
	for (CONFIG::const_iterator i = opt.begin(); i != opt.end(); ++i)
	{
		if (i->first.empty()) continue;
		
		std::string cat, name, defaultval, values, text, desc, type;
		if (!opt.GetParam(i, "cat", cat, error_output)) return false;
		if (!opt.GetParam(i, "name", name, error_output)) return false;
		if (!opt.GetParam(i, "default", defaultval, error_output)) return false;
		if (!opt.GetParam(i, "values", values, error_output)) return false;
		if (!opt.GetParam(i, "title", text, error_output)) return false;
		if (!opt.GetParam(i, "desc", desc, error_output)) return false;
		if (!opt.GetParam(i, "type", type, error_output)) return false;
		
		std::map<std::string, std::string>::const_iterator li;
		if ((li = languagemap.find(text)) != languagemap.end()) text = li->second;
		if ((li = languagemap.find(desc)) != languagemap.end()) desc = li->second;
		
		float min(0),max(1);
		bool percentage(true);
		opt.GetParam(i, "min",min);
		opt.GetParam(i, "max",max);
		opt.GetParam(i, "percentage",percentage);
		
		std::string optionname = cat+"."+name;
		GUIOPTION & option = optionmap[optionname];
		
		option.SetInfo(text, desc, type);
		option.SetMinMaxPercentage(min, max, percentage);
		
		//different ways to populate the options
		if (values == "list")
		{
			int valuenum;
			if (!opt.GetParam(i, "num_vals", valuenum, error_output)) return false;
			
			for (int n = 0; n < valuenum; n++)
			{
				std::stringstream tstr;
				tstr.width(2);
				tstr.fill('0');
				tstr << n;
				
				std::string displaystr, storestr;
				if (!opt.GetParam(i, "opt"+tstr.str(), displaystr, error_output)) return false;
				if (!opt.GetParam(i, "val"+tstr.str(), storestr, error_output)) return false;
				if ((li = languagemap.find(displaystr)) != languagemap.end()) displaystr = li->second;
				
				option.Insert(storestr, displaystr);
			}
		}
		else if (values == "bool")
		{
			std::string truestr, falsestr;
			if (!opt.GetParam(i, "true", truestr, error_output)) return false;
			if (!opt.GetParam(i, "false", falsestr, error_output)) return false;
			
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
				error_output << "Can't find value type \"" << values << "\" in list of GAME values" << std::endl;
				return "GAME valuelist";
			}
			else
			{
				//std::cout << "Populating values:" << std::endl;
				for (std::list <std::pair<std::string,std::string> >::const_iterator n = vlist->second.begin(); n != vlist->second.end(); n++)
				{
					//std::cout << "\t" << n->second << std::endl;
					option.Insert(n->first, n->second);
				}
			}
		}
		
		option.SetCurrentValue(defaultval);
	}
	
	UpdateOptions(error_output);
	
	return "";
}

void GUI::UpdateOptions(std::ostream & error_output)
{
	bool save_to = false;
	for (std::map<std::string, GUIPAGE>::iterator i = pages.begin(); i != pages.end(); ++i)
	{
		i->second.UpdateOptions(node, save_to, optionmap, error_output);
	}
}

bool GUI::SetLabelText(const std::string & pagename, const std::string & labelname, const std::string & text)
{
	if (pages.find(pagename) == pages.end())
		return false;
	
	SCENENODE & pagenode = GetPageNode(pagename);
	reseatable_reference <WIDGET_LABEL> label = GetPage(pagename).GetLabel(labelname);
	if (!label)
		return false;
	
	label.get().SetText(pagenode, text);
	
	return true;
}
