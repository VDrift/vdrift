/************************************************************************/
/*                                                                      */
/* This file is part of VDrift.                                         */
/*                                                                      */
/* VDrift is free software: you can redistribute it and/or modify       */
/* it under the terms of the GNU General Public License as published by */
/* the Free Software Foundation, either version 3 of the License, or    */
/* (at your option) any later version.                                  */
/*                                                                      */
/* VDrift is distributed in the hope that it will be useful,            */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of       */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the        */
/* GNU General Public License for more details.                         */
/*                                                                      */
/* You should have received a copy of the GNU General Public License    */
/* along with VDrift.  If not, see <http://www.gnu.org/licenses/>.      */
/*                                                                      */
/************************************************************************/

#include "gui.h"
#include "guilabel.h"
#include "cfg/config.h"
#include <sstream>

static const std::string null;

GUI::GUI() :
	m_cursorx(0),
	m_cursory(0),
	animation_counter(0),
	animation_count_start(0),
	ingame(false)
{
	active_page = last_active_page = pages.end();
}

const std::string & GUI::GetActivePageName() const
{
	if (active_page == pages.end()) return null;
	return active_page->first;
}

const std::string & GUI::GetLastPageName() const
{
	if (last_active_page == pages.end()) return null;
	return last_active_page->first;
}

SCENENODE & GUI::GetNode()
{
	return node;
}

GUIPAGE & GUI::GetPage(const std::string & name)
{
	assert(pages.find(name) != pages.end());
	return pages[name];
}

bool GUI::Active() const
{
	return (active_page != pages.end());
}

bool GUI::GetInGame() const
{
	return ingame;
}

void GUI::SetInGame(bool value)
{
	ingame = value;
}

void GUI::Unload()
{
    // clear out maps
    pages.clear();
    options.clear();
    page_activate.clear();

    // clear out the scenegraph
    node.Clear();

    // reset variables
    animation_counter = 0;
    animation_count_start = 0;
    active_page = last_active_page = pages.end();

    // some things we don't want to reset incase we're in the middle of a reload;
    // for example we don't want to reset ingame
}

bool GUI::Load(
	const std::list <std::string> & pagelist,
	const std::map<std::string, std::list <std::pair <std::string, std::string> > > & valuelists,
	const std::string & datapath,
	const std::string & optionsfile,
	const std::string & skinname,
	const std::string & language,
	const std::string & texsize,
	const float screenhwratio,
	std::map <std::string, Slot0*> actionmap,
	ContentManager & content,
	std::ostream & info_output,
	std::ostream & error_output)
{
	Unload();

	// serious mess here, needs cleanup
	const std::string fonttexpath = "skins/" + skinname + "/fonts";
	const std::string texpath = "skins/" + skinname + "/textures";
	const std::string skinpath = datapath + "/skins/" + skinname;
	const std::string fontpath = skinpath + "/fonts";
	const std::string langpath = skinpath + "/languages";
	const std::string menupath = skinpath + "/menus";

	// setup language
	lang.Init(langpath);
	lang.Set(language, error_output);

	// load font (hardcoded for now)
	const std::string fontcp = lang.GetCodePageId(language);
	const std::string fonttex = "robotobold_" + fontcp + "_sdf.png";
	const std::string fontinfo = fontpath + "/robotobold_" + fontcp + ".txt";
	if (!font.Load(fontinfo, fonttexpath, fonttex, content, error_output))
		return false;

	// load options
	if (!LoadOptions(optionsfile, valuelists, error_output))
		return false;

	VSIGNALMAP vsignalmap;
	VACTIONMAP vactionmap;
	RegisterOptions(vsignalmap, vactionmap, actionmap);

	// init pages
	size_t pagecount = 0;
	for (std::list <std::string>::const_iterator i = pagelist.begin(); i != pagelist.end(); ++i)
	{
		pages.insert(std::make_pair(*i, GUIPAGE()));
		pagecount++;
	}

	// register pages
	page_activate.reserve(pagecount);
	for (PAGEMAP::iterator i = pages.begin(); i != pages.end(); ++i)
	{
		page_activate.push_back(PAGECB());
		page_activate.back().gui = this;
		page_activate.back().page = i->first;
		actionmap[i->first] = &page_activate.back().action;
	}

	// load pages
	for (PAGEMAP::iterator i = pages.begin(); i != pages.end(); ++i)
	{
		const std::string pagepath = menupath + "/" + i->first;
		if (!i->second.Load(
			pagepath, texpath, screenhwratio, lang, font,
			vsignalmap, vactionmap, actionmap,
			node, content, error_output))
		{
			error_output << "Error loading GUI page: " << pagepath << std::endl;
			return false;
		}
	}

	if (pages.find("Main") == pages.end())
	{
		error_output << "Couldn't find GUI Main menu in: " << menupath << std::endl;
		return false;
	}

	info_output << "Loaded GUI successfully" << std::endl;

	// start out with everything invisible
	Deactivate();

	return true;
}

void GUI::Deactivate()
{
	for (std::map<std::string, GUIPAGE>::iterator i = pages.begin(); i != pages.end(); ++i)
	{
		i->second.SetVisible(node, false);
	}
	active_page = pages.end();
}

void GUI::ProcessInput(
	float cursorx, float cursory,
	bool cursordown, bool cursorjustup,
	bool moveleft, bool moveright,
	bool moveup, bool movedown,
	bool select, bool cancel)
{
	if (active_page == pages.end())
		return;

	bool cursormoved = m_cursorx != cursorx || m_cursory != cursory;
	m_cursorx = cursorx;
	m_cursory = cursory;

	active_page->second.ProcessInput(
		cursorx, cursory, cursormoved,
		cursordown, cursorjustup,
		moveleft, moveright,
		moveup, movedown,
		select, cancel);
}

void GUI::Update(float dt)
{
	bool fadein = animation_counter > 0;

	animation_counter -= dt;
	if (animation_counter < 0)
		animation_counter = 0;

	if (active_page != pages.end())
	{
		active_page->second.Update(node, dt);
		if (fadein)
		{
			// ease curve: 3*p^2-2*p^3
			float p = 1.0 - animation_counter / animation_count_start;
			float alpha = 3 * p * p - 2 * p * p * p;
			active_page->second.SetAlpha(node, alpha);
			//std::cout << "fade in: " << alpha << std::endl;
		}
	}

	if (last_active_page != pages.end())
	{
		last_active_page->second.Update(node, dt);
		if (fadein)
		{
			float p = animation_counter / animation_count_start;
			float alpha = 3 * p * p - 2 * p * p * p;
			last_active_page->second.SetAlpha(node, alpha);
			//std::cout << "fade out: " << alpha << std::endl;
		}
		else
		{
			last_active_page->second.SetVisible(node, false);
		}
	}
}

void GUI::GetOptions(std::map <std::string, std::string> & options) const
{
	for (std::map <std::string, std::string>::iterator i = options.begin(); i != options.end(); ++i)
	{
		OPTIONMAP::const_iterator option = this->options.find(i->first);
		if (option != this->options.end())
			i->second = option->second.GetCurrentStorageValue();
	}
}

void GUI::SetOptions(const std::map <std::string, std::string> & options)
{
	for (std::map <std::string, std::string>::const_iterator i = options.begin(); i != options.end(); ++i)
	{
		OPTIONMAP::iterator option = this->options.find(i->first);
		if (option != this->options.end())
			option->second.SetCurrentValue(i->second);
	}
}

void GUI::SetOptionValues(
	const std::string & optionname,
	const std::string & curvalue,
	std::list <std::pair <std::string, std::string> > & newvalues,
	std::ostream & error_output)
{
	OPTIONMAP::iterator op = options.find(optionname);
	if (op == options.end())
	{
		error_output << "Can't find option named " << optionname << " when replacing optionmap values" << std::endl;
		return;
	}
	op->second.SetValues(curvalue, newvalues);
}

void GUI::ActivatePage(
	const std::string & pagename,
	float activation_time,
	std::ostream & error_output)
{
	if (!ActivatePage(pagename, activation_time))
		error_output << "Gui page not found: " << pagename << std::endl;
}

bool GUI::ActivatePage(
	const std::string & pagename,
	float activation_time)
{
	// check whether a page is faded in
	if (animation_counter > 0)
		return true;

	// check whether page already active
	if (active_page != pages.end() && active_page->first == pagename)
		return true;

	// get new page
	PAGEMAP::iterator next_active_page;
	if (ingame && pagename == "Main")
		next_active_page = pages.find("InGameMain");
	else
		next_active_page = pages.find(pagename);

	if (next_active_page == pages.end())
		return false;

	// activate new page
	last_active_page = active_page;
	active_page = next_active_page;
	active_page->second.SetVisible(node, true);

	// reset animation counter
	animation_counter = animation_count_start = activation_time;
	//std::cout << "activate: " << active_page->first << std::endl;

	return true;
}

bool GUI::LoadOptions(
	const std::string & optionfile,
	const std::map<std::string, std::list <std::pair <std::string, std::string> > > & valuelists,
	std::ostream & error_output)
{
	Config opt;
	if (!opt.load(optionfile))
	{
		error_output << "Can't find options file: " << optionfile << std::endl;
		return false;
	}

	//opt.DebugPrint(error_output);
	for (Config::const_iterator i = opt.begin(); i != opt.end(); ++i)
	{
		if (i->first.empty()) continue;

		std::string cat, name, defaultval, values, desc, type;
		if (!opt.get(i, "cat", cat, error_output)) return false;
		if (!opt.get(i, "name", name, error_output)) return false;
		if (!opt.get(i, "default", defaultval, error_output)) return false;
		if (!opt.get(i, "values", values, error_output)) return false;
		if (!opt.get(i, "desc", desc, error_output)) return false;
		if (!opt.get(i, "type", type, error_output)) return false;

		desc = lang(desc);

		float min(0), max(1);
		bool percent(false);
		opt.get(i, "min", min);
		opt.get(i, "max", max);
		opt.get(i, "percent", percent);

		std::string optionname = cat + "." + name;
		GUIOPTION & option = options[optionname];

		option.SetInfo(desc, type);
		option.SetMinMaxPercentage(min, max, percent);

		// different ways to populate the options
		std::list <std::pair<std::string, std::string> >  opvals;
		if (values == "list")
		{
			int valuenum;
			if (!opt.get(i, "num_vals", valuenum, error_output)) return false;

			for (int n = 0; n < valuenum; n++)
			{
				std::stringstream tstr;
				tstr.width(2);
				tstr.fill('0');
				tstr << n;

				std::string displaystr, storestr;
				if (!opt.get(i, "opt"+tstr.str(), displaystr, error_output)) return false;
				if (!opt.get(i, "val"+tstr.str(), storestr, error_output)) return false;
				opvals.push_back(std::make_pair(storestr, lang(displaystr)));
			}
		}
		else if (values == "bool")
		{
			std::string truestr, falsestr;
			if (!opt.get(i, "true", truestr, error_output)) return false;
			if (!opt.get(i, "false", falsestr, error_output)) return false;
			opvals.push_back(std::make_pair("true", lang(truestr)));
			opvals.push_back(std::make_pair("false", lang(falsestr)));
		}
		else if (values == "ip_valid")
		{

		}
		else if (values == "port_valid")
		{

		}
		else if (values == "int")
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
				return false;
			}
			for (std::list <std::pair<std::string,std::string> >::const_iterator n = vlist->second.begin(); n != vlist->second.end(); n++)
			{
				opvals.push_back(std::make_pair(n->first, lang(n->second)));
			}
		}
		option.SetValues(defaultval, opvals);
	}

	return true;
}

void GUI::RegisterOptions(VSIGNALMAP & vsignalmap, VACTIONMAP & vactionmap, ACTIONMAP & actionmap)
{
	for (OPTIONMAP::iterator i = options.begin(); i != options.end(); ++i)
	{
		vsignalmap[i->first + ".str"] = &i->second.signal_str;
		vsignalmap[i->first + ".norm"] = &i->second.signal_valn;
		vactionmap[i->first + ".norm"] = &i->second.set_valn;
		vsignalmap[i->first] = &i->second.signal_val;
		vactionmap[i->first] = &i->second.set_val;
		actionmap[i->first + ".next"] = &i->second.next_val;
		actionmap[i->first + ".prev"] = &i->second.prev_val;
	}
}

bool GUI::SetLabelText(const std::string & pagename, const std::string & labelname, const std::string & text)
{
	if (pages.find(pagename) == pages.end())
		return false;

	GUILABEL * label = GetPage(pagename).GetLabel(labelname);
	if (!label)
		return false;

	label->SetText(text);

	return true;
}

bool GUI::GetLabelText(const std::string & pagename, const std::string & labelname, std::string & text_output)
{

	PAGEMAP::iterator p = pages.find(pagename);
	if (p == pages.end())
		return false;

	GUILABEL * label = p->second.GetLabel(labelname);
	if (!label)
		return false;

	text_output = label->GetText();

	return true;
}

void GUI::SetLabelText(const std::string & pagename, const std::map<std::string, std::string> & label_text)
{
	PAGEMAP::iterator p = pages.find(pagename);
	if (p != pages.end())
		p->second.SetLabelText(label_text);
}

void GUI::SetLabelText(const std::map<std::string, std::string> & label_text)
{
	for (PAGEMAP::iterator p = pages.begin(); p != pages.end(); ++p)
	{
		p->second.SetLabelText(label_text);
	}
}

std::string GUI::GetOptionValue(const std::string & name) const
{
	OPTIONMAP::const_iterator it = options.find(name);
	if (it != options.end())
	{
		return it->second.GetCurrentStorageValue();
	}
	return std::string();
}

void GUI::SetOptionValue(const std::string & name, const std::string & value)
{
	OPTIONMAP::iterator it = options.find(name);
	if (it != options.end())
	{
		it->second.SetCurrentValue(value);
	}
}

GUIOPTION & GUI::GetOption(const std::string & name)
{
	OPTIONMAP::iterator it = options.find(name);
	assert(it != options.end());
	return it->second;
}

const GUILANGUAGE & GUI::GetLanguageDict() const
{
	return lang;
}

const FONT & GUI::GetFont() const
{
	return font;
}

GUI::PAGECB::PAGECB()
{
	action.call.bind<PAGECB, &PAGECB::call>(this);
	gui = 0;
}

GUI::PAGECB::PAGECB(const PAGECB & other)
{
	*this = other;
}

GUI::PAGECB & GUI::PAGECB::operator=(const PAGECB & other)
{
	action.call.bind<PAGECB, &PAGECB::call>(this);
	gui = other.gui;
	page = other.page;
	return *this;
}

void GUI::PAGECB::call()
{
	assert(gui && !page.empty());
	gui->ActivatePage(page, 0.25);
}
