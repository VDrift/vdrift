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

static bool LoadOptions(
	const Config & cfg,
	const GuiLanguage & lang,
	Gui::OptionMap & options,
	std::ostream & /*error_output*/)
{
	for (auto i = cfg.begin(); i != cfg.end(); ++i)
	{
		std::string type, desc;
		if (i->first.empty() || !(cfg.get(i, "type", type) && cfg.get(i, "desc", desc)))
			continue;

		float min(0), max(1);
		bool percent(false);
		cfg.get(i, "min", min);
		cfg.get(i, "max", max);
		cfg.get(i, "percent", percent);

		options[i->first].SetInfo(lang(desc), type, min, max, percent);
	}
	return true;
}

static bool LoadOptionValues(
	const Config & cfg,
	const GuiLanguage & lang,
	const std::map<std::string, GuiOption::List> & valuelists,
	Gui::OptionMap & options,
	std::ostream & error_output)
{
	for (auto & op : options)
	{
		const std::string & name = op.first;
		GuiOption & option = op.second;

		Config::const_iterator ci;
		if (!cfg.get(name, ci, error_output)) return false;

		std::string defaultval, values;
		if (!cfg.get(ci, "default", defaultval, error_output)) return false;
		if (!cfg.get(ci, "values", values, error_output)) return false;

		// different ways to populate the options
		GuiOption::List opvals;
		if (values == "list")
		{
			int valuenum;
			if (!cfg.get(ci, "num_vals", valuenum, error_output)) return false;

			for (int n = 0; n < valuenum; n++)
			{
				std::ostringstream ns;
				ns.width(2);
				ns.fill('0');
				ns << n;

				std::string displaystr, storestr;
				if (!cfg.get(ci, "opt"+ns.str(), displaystr, error_output)) return false;
				if (!cfg.get(ci, "val"+ns.str(), storestr, error_output)) return false;
				opvals.push_back(std::make_pair(storestr, lang(displaystr)));
			}
		}
		else if (values == "bool")
		{
			std::string truestr, falsestr;
			if (!cfg.get(ci, "true", truestr, error_output)) return false;
			if (!cfg.get(ci, "false", falsestr, error_output)) return false;
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
			auto vlist = valuelists.find(values);
			if (vlist == valuelists.end())
			{
				error_output << "Can't find value type \"" << values << "\" in list of GAME values" << std::endl;
				return false;
			}
			for (const auto & val : vlist->second)
			{
				opvals.push_back(std::make_pair(val.first, lang(val.second)));
			}
		}
		option.SetValues(defaultval, opvals);
	}
	return true;
}

Gui::Gui() :
	m_cursorx(0),
	m_cursory(0),
	animation_counter(0),
	animation_count_start(0),
	next_animation_count_start(0),
	ingame(false)
{
	last_active_page = pages.end();
	active_page = pages.end();
	next_active_page = pages.end();
}

const std::string & Gui::GetActivePageName() const
{
	if (active_page == pages.end()) return null;
	return active_page->first;
}

const std::string & Gui::GetLastPageName() const
{
	if (last_active_page == pages.end()) return null;
	return last_active_page->first;
}

std::pair<SceneNode*, SceneNode*> Gui::GetNodes()
{
	SceneNode *n1 = NULL, *n2 = NULL;
	if (active_page != pages.end())
		n1 = &active_page->second.GetNode();
	if (animation_counter > 0 && last_active_page != pages.end())
		n2 = &last_active_page->second.GetNode();
	return std::make_pair(n1, n2);
}

bool Gui::GetInGame() const
{
	return ingame;
}

void Gui::SetInGame(bool value)
{
	ingame = value;
}

void Gui::Unload()
{
	// clear out maps
	pages.clear();
	options.clear();

	// reset variables
	animation_counter = 0;
	animation_count_start = 0;
	next_animation_count_start = 0;
	last_active_page = pages.end();
	active_page = pages.end();
	next_active_page = pages.end();

	// some things we don't want to reset incase we're in the middle of a reload;
	// for example we don't want to reset ingame
}

bool Gui::Load(
	const std::list <std::string> & pagelist,
	const std::map<std::string, GuiOption::List> & valuelists,
	const std::string & datapath,
	const std::string & optionsfile,
	const std::string & skinname,
	const std::string & language,
	const float screenhwratio,
	StrSignalMap vsignalmap,
	SlotMap actionmap,
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
	lang.Set(language, info_output, error_output);

	// load font (hardcoded for now)
	const std::string fontcp = lang.GetCodePage();
	const std::string fonttex = "robotobold_" + fontcp + "_sdf.png";
	const std::string fontinfo = fontpath + "/robotobold_" + fontcp + ".txt";
	if (!font.Load(fontinfo, fonttexpath, fonttex, content, error_output))
		return false;

	// load options
	Config opt;
	if (!opt.load(optionsfile))
	{
		error_output << "Failed to load options file: " << optionsfile << std::endl;
		return false;
	}
	if (!LoadOptions(opt, lang, options, error_output))
	{
		error_output << "Failed to load options." << std::endl;
		return false;
	}

	// register options
	StrVecSlotMap vnactionmap;
	IntSlotMap nactionmap;
	StrSlotMap vactionmap;
	RegisterOptions(vsignalmap, vnactionmap, vactionmap, nactionmap, actionmap);

	// register page activation callbacks
	vactionmap["gui.page"].bind<Gui, &Gui::ActivatePage>(this);
	actionmap["gui.page.prev"].bind<Gui, &Gui::ActivatePrevPage>(this);

	// init pages
	size_t pagecount = 0;
	for (const auto & page : pagelist)
	{
		pages.emplace(page, GuiPage());
		pagecount++;
	}

	// load pages
	for (auto & page : pages)
	{
		const std::string pagepath = menupath + "/" + page.first;
		if (!page.second.Load(
			pagepath, texpath, screenhwratio, lang, font,
			vsignalmap, vnactionmap, vactionmap, nactionmap, actionmap,
			content, error_output))
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

	// populate option values
	// has to happen after page loading to sync gui with options
	if (!LoadOptionValues(opt, lang, valuelists, options, error_output))
	{
		error_output << "Failed to load option values." << std::endl;
		return false;
	}

	info_output << "Loaded GUI successfully" << std::endl;

	// start out with everything invisible
	Deactivate();

	return true;
}

void Gui::Deactivate()
{
	for (auto & page : pages)
		page.second.SetVisible(false);

	last_active_page = pages.end();
	active_page = pages.end();
	next_active_page = pages.end();
}

void Gui::ProcessInput(
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

void Gui::Update(float dt)
{
	bool fade = animation_counter > 0 && animation_count_start > 0;

	animation_counter -= dt;
	if (animation_counter < 0)
		animation_counter = 0;

	if (active_page != pages.end())
	{
		active_page->second.Update(dt);
		if (fade)
		{
			// fade in new active page
			// ease curve: 3 * p^2 - 2 * p^3
			float p = 1 - animation_counter / animation_count_start;
			float alpha = 3 * p * p - 2 * p * p * p;
			active_page->second.SetAlpha(alpha);
			//dlog << active_page->first << " fade in: " << alpha << std::endl;
		}
	}

	if (fade && last_active_page != pages.end())
	{
		// fade out previous active page
		float p = animation_counter / animation_count_start;
		float alpha = 3 * p * p - 2 * p * p * p;
		last_active_page->second.SetAlpha(alpha);
		//dlog << last_active_page->first << " fade out: " << alpha << std::endl;
	}

	if (!fade && next_active_page != pages.end())
	{
		// deactivate previously active page after fading out
		if (animation_count_start > 0 && last_active_page != pages.end())
		{
			last_active_page->second.SetVisible(false);
			//dlog << last_active_page->first << " deactivate" << std::endl;
		}

		last_active_page = active_page;
		active_page = next_active_page;
		next_active_page = pages.end();

		// activate new page
		active_page->second.SetVisible(true);
		active_page->second.Update(dt);
		//dlog << active_page->first << " activate" << std::endl;

		if (next_animation_count_start > 0)
		{
			// start fading in new active page
			active_page->second.SetAlpha(0.0);
		}
		else if (last_active_page != pages.end())
		{
			// deactivate previously active page
			last_active_page->second.SetVisible(false);
			//dlog << last_active_page->first << " deactivate" << std::endl;
		}

		// reset animation counter
		animation_count_start = next_animation_count_start;
		animation_counter = next_animation_count_start;
	}
}

void Gui::GetOptions(std::map <std::string, std::string> & options) const
{
	for (auto & op : options)
	{
		auto option = this->options.find(op.first);
		if (option != this->options.end())
			op.second = option->second.GetCurrentStorageValue();
	}
}

void Gui::SetOptions(const std::map <std::string, std::string> & options)
{
	for (const auto & op : options)
	{
		auto option = this->options.find(op.first);
		if (option != this->options.end())
			option->second.SetCurrentValue(op.second);
	}
}

void Gui::SetOptionValues(
	const std::string & optionname,
	const std::string & curvalue,
	const GuiOption::List & newvalues,
	std::ostream & error_output)
{
	auto op = options.find(optionname);
	if (op == options.end())
	{
		error_output << "Can't find option named " << optionname << " when replacing optionmap values" << std::endl;
		return;
	}
	op->second.SetValues(curvalue, newvalues);
}

void Gui::ActivatePage(
	const std::string & pagename,
	float activation_time,
	std::ostream & error_output)
{
	if (!ActivatePage(pagename, activation_time))
		error_output << "Gui page not found: " << pagename << std::endl;
}

bool Gui::ActivatePage(
	const std::string & pagename,
	float /*activation_time*/)
{
	// check whether page is already active
	if (active_page != pages.end() && active_page->first == pagename)
		return true;

	// get new page
	if (ingame && pagename == "Main")
		next_active_page = pages.find("InGameMain");
	else
		next_active_page = pages.find(pagename);

	if (next_active_page == pages.end())
		return false;

	// FIXME: Fading animation disabled due to half-transparent gui elements flickering.
	next_animation_count_start = 0.0;//activation_time;

	return true;
}

void Gui::ActivatePage(const std::string & pagename)
{
	ActivatePage(pagename, 0.25);
}

void Gui::ActivatePrevPage()
{
	ActivatePage(GetLastPageName(), 0.25);
}

void Gui::RegisterOptions(
	StrSignalMap & vsignalmap,
	StrVecSlotMap & vnactionmap,
	StrSlotMap & vactionmap,
	IntSlotMap & nactionmap,
	SlotMap & actionmap)
{
	for (auto & n : options)
	{
		const std::string & name = n.first;
		GuiOption & op = n.second;
		if (op.IsFloat())
		{
			// option is a float
			vsignalmap[name + ".norm"] = &op.signal_valn;
			vactionmap[name + ".norm"].bind<GuiOption, &GuiOption::SetCurrentValueNorm>(&op);
		}
		else
		{
			// option is a list
			vsignalmap[name + ".str.update"] = &op.signal_update;
			vsignalmap[name + ".update"] = &op.signal_update;
			vnactionmap[name + ".str"].bind<GuiOption, &GuiOption::GetDisplayValues>(&op);
			vnactionmap[name].bind<GuiOption, &GuiOption::GetStorageValues>(&op);
		}
		vsignalmap[name + ".nth"] = &op.signal_nth;
		vsignalmap[name + ".str"] = &op.signal_str;
		vsignalmap[name] = &op.signal_val;
		vactionmap[name].bind<GuiOption, &GuiOption::SetCurrentValue>(&op);
		nactionmap[name + ".nth"].bind<GuiOption, &GuiOption::SetToNthValue>(&op);
		actionmap[name + ".next"].bind<GuiOption, &GuiOption::Increment>(&op);
		actionmap[name + ".prev"].bind<GuiOption, &GuiOption::Decrement>(&op);
	}
}

bool Gui::SetLabelText(const std::string & pagename, const std::string & labelname, const std::string & text)
{
	auto p = pages.find(pagename);
	if (p == pages.end())
		return false;

	GuiLabel * label = p->second.GetLabel(labelname);
	if (!label)
		return false;

	label->SetText(text);
	return true;
}

void Gui::SetLabelText(const std::string & pagename, const std::map<std::string, std::string> & label_text)
{
	auto p = pages.find(pagename);
	if (p != pages.end())
		p->second.SetLabelText(label_text);
}

void Gui::SetLabelText(const std::map<std::string, std::string> & label_text)
{
	for (auto & page : pages)
		page.second.SetLabelText(label_text);
}

const std::string & Gui::GetOptionValue(const std::string & name) const
{
	auto it = options.find(name);
	if (it != options.end())
		return it->second.GetCurrentStorageValue();
	return null;
}

void Gui::SetOptionValue(const std::string & name, const std::string & value)
{
	auto it = options.find(name);
	if (it != options.end())
		it->second.SetCurrentValue(value);
}

GuiOption & Gui::GetOption(const std::string & name)
{
	auto it = options.find(name);
	assert(it != options.end());
	return it->second;
}

const GuiLanguage & Gui::GetLanguageDict() const
{
	return lang;
}

const Font & Gui::GetFont() const
{
	return font;
}
