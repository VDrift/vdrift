#include "gui.h"
#include "configfile.h"

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
			if (!LoadPage(
					*i,
					menupath,
					texpath,
					datapath,
					texsize,
					screenhwratio,
					controlsconfig,
					fonts,
					optionmap,
					node,
					textures,
					models,
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
	const CONFIGFILE & carcontrolsfile,
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

string GUI::LoadOptions(const std::string & optionfile, const std::map<std::string, std::list <std::pair <std::string, std::string> > > & valuelists, std::ostream & error_output)
{
	CONFIGFILE o;
	
	if (!o.Load(optionfile))
	{
		error_output << "Can't find options file: " << optionfile << endl;
		return "File loading";
	}
	
	std::list <std::string> sectionlist;
	o.GetSectionList(sectionlist);
	for (std::list <std::string>::iterator i = sectionlist.begin(); i != sectionlist.end(); ++i)
	{
		if (!i->empty())
		{
			string cat, name, defaultval, values, text, desc, type;
			if (!o.GetParam(*i+".cat", cat)) return *i+".cat";
			if (!o.GetParam(*i+".name", name)) return *i+".name";
			if (!o.GetParam(*i+".default", defaultval)) return *i+".default";
			if (!o.GetParam(*i+".values", values)) return *i+".values";
			if (!o.GetParam(*i+".title", text)) return *i+".title";
			if (!o.GetParam(*i+".desc", desc)) return *i+".desc";
			if (!o.GetParam(*i+".type", type)) return *i+".type";
			
			float min(0),max(1);
			bool percentage(true);
			o.GetParam(*i+".min",min);
			o.GetParam(*i+".max",max);
			o.GetParam(*i+".percentage",percentage);
			
			string optionname = cat+"."+name;
			
			optionmap[optionname].SetInfo(text, desc, type);
			optionmap[optionname].SetMinMaxPercentage(min, max, percentage);
			
			//different ways to populate the options
			if (values == "list")
			{
				int valuenum;
				if (!o.GetParam(*i+".num_vals", valuenum)) return *i+".num_vals";
				
				for (int n = 0; n < valuenum; n++)
				{
					stringstream tstr;
					tstr.width(2);
					tstr.fill('0');
					tstr << n;
					
					string displaystr, storestr;
					if (!o.GetParam(*i+".opt"+tstr.str(), displaystr)) return *i+".opt"+tstr.str();
					if (!o.GetParam(*i+".val"+tstr.str(), storestr)) return *i+".val"+tstr.str();
					
					optionmap[optionname].Insert(storestr, displaystr);
				}
			}
			else if (values == "bool")
			{
				string truestr, falsestr;
				if (!o.GetParam(*i+".true", truestr)) return *i+".true";
				if (!o.GetParam(*i+".false", falsestr)) return *i+".false";
					
				optionmap[optionname].Insert("true", truestr);
				optionmap[optionname].Insert("false", falsestr);
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
						optionmap[optionname].Insert(n->first, n->second);
					}
				}
			}
			
			optionmap[optionname].SetCurrentValue(defaultval);
		}
	}
	
	UpdateOptions(error_output);
	
	return "";
}
