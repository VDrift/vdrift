#include "gui/gui.h"
#include "gui/guicontrolgrab.h"
#include "config.h"
#include "pathmanager.h"

#include <sstream>

GUI::GUI() :
	animation_counter(0),
	animation_count_start(0),
	ingame(false)
{
	active_page = last_active_page = pages.end();
}

std::string GUI::GetActivePageName()
{
	if (active_page == pages.end()) return "";
	return active_page->first;
}

std::string GUI::GetLastPageName()
{
	if (last_active_page == pages.end()) return "";
	return last_active_page->first;
}

SCENENODE & GUI::GetNode()
{
	return node;
}

SCENENODE & GUI::GetPageNode(const std::string & name)
{
	assert(pages.find(name) != pages.end());
	return pages[name].GetNode(node);
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

void GUI::SetInGame(bool value)
{
	ingame = value;
}

void GUI::Unload()
{
    // clear out maps
    pages.clear();
    optionmap.clear();
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
	const std::string & optionsfile,
	const std::string & carcontrolsfile,
	const std::string & menupath,
	const std::string & languagedir,
	const std::string & language,
	const std::string & texpath,
	const PATHMANAGER & pathmanager,
	const std::string & texsize,
	const float screenhwratio,
	const std::map <std::string, FONT> & fonts,
	std::map <std::string, Slot0*> & actionmap,
	ContentManager & content,
	std::ostream & info_output,
	std::ostream & error_output)
{
	Unload();

	std::string datapath = pathmanager.GetDataPath();

	// load language font
	CONFIG languageconfig;
	languageconfig.Load(datapath + "/" + languagedir + "/" + language + ".lng");

	std::string fontinfo, fonttex;
	if (!languageconfig.GetParam("font", "info", fontinfo, error_output)) return false;
	if (!languageconfig.GetParam("font", "texture", fonttex, error_output)) return false;

	std::string fontinfopath = datapath + "/" + languagedir + "/" + fontinfo;
	if (!font.Load(fontinfopath, languagedir, fonttex, content, error_output)) return false;

	// load language map
	CONFIG::const_iterator section;
	std::map<std::string, std::string> languagemap;
	if (languageconfig.GetSection("strings", section))
	{
		languagemap = section->second;
	}
	//languageconfig.DebugPrint(std::clog);

	// controlgrab description strings translation (ugly hack)
	for (int i = 0; i < GUICONTROLGRAB::END_STR; ++i)
	{
		std::string & str = GUICONTROLGRAB::Str[i];
		std::map<std::string, std::string>::const_iterator li;
		if ((li = languagemap.find(str)) != languagemap.end()) str = li->second;
	}

	// load options
	if (!LoadOptions(optionsfile, valuelists, languagemap, error_output)) return false;

	// pre-load controls file so that each individual widget doesn't have to reload it
	CONFIG controlsconfig;
	controlsconfig.Load(carcontrolsfile);

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
			pagepath, texpath, pathmanager,
			screenhwratio, carcontrolsfile, font,
			languagemap, optionmap, actionmap,
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

void GUI::UpdateControls(const std::string & pagename, const CONFIG & controlfile)
{
	assert(pages.find(pagename) != pages.end());
	pages[pagename].UpdateControls(node, controlfile, font);
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
	bool movedown, bool moveup,
	float cursorx, float cursory,
	bool cursordown, bool cursorjustup,
	float screenhwratio)
{
	if (active_page != pages.end())
	{
		active_page->second.ProcessInput(
			node, movedown, moveup,
			cursorx, cursory,
			cursordown, cursorjustup,
			screenhwratio);
	}
}

void GUI::Update(float dt)
{
	animation_counter -= dt;
	if (animation_counter < 0) animation_counter = 0;

	if (active_page != pages.end())
	{
		//ease curve: 3*p^2-2*p^3
		float p = 1.0 - animation_counter / animation_count_start;
		float alpha = 3 * p * p - 2 * p * p * p;
		active_page->second.SetAlpha(node, alpha);
		active_page->second.Update(node, dt);
	}

	if (last_active_page != pages.end())
	{
		if (animation_counter > 0)
		{
			float p = animation_counter / animation_count_start;
			float alpha = 3 * p * p - 2 * p * p * p;
			last_active_page->second.SetAlpha(node, alpha);
			last_active_page->second.Update(node, dt);
		}
		else
		{
			last_active_page->second.SetVisible(node, false);
			last_active_page = pages.end();
		}
	}
}

void GUI::GetOptions(std::map <std::string, std::string> & options) const
{
	for (std::map <std::string, std::string>::iterator i = options.begin(); i != options.end(); ++i)
	{
		std::map<std::string, GUIOPTION>::const_iterator option = optionmap.find(i->first);
		if (option != optionmap.end())
			i->second = option->second.GetCurrentStorageValue();
	}
}

void GUI::SetOptions(const std::map <std::string, std::string> & options)
{
	for (std::map <std::string, std::string>::const_iterator i = options.begin(); i != options.end(); ++i)
	{
		std::map<std::string, GUIOPTION>::iterator option = optionmap.find(i->first);
		if (option != optionmap.end())
			option->second.SetCurrentValue(i->second);
	}
}

void GUI::ReplaceOptionValues(
	const std::string & optionname,
	const std::list <std::pair <std::string, std::string> > & newvalues,
	std::ostream & error_output)
{
	std::map<std::string, GUIOPTION>::iterator op = optionmap.find(optionname);
	if (op == optionmap.end())
	{
		error_output << "Can't find option named " << optionname << " when replacing optionmap values" << std::endl;
	}
	else
	{
		op->second.ReplaceValues(newvalues);
	}
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
	animation_counter = animation_count_start = activation_time;

	if (active_page != pages.end() && active_page->first == pagename)
		return true;

	PAGEMAP::iterator next_active_page;
	if (ingame && pagename == "Main")
		next_active_page = pages.find("InGameMain");
	else
		next_active_page = pages.find(pagename);

	if (next_active_page == pages.end())
		return false;

	last_active_page = active_page;
	active_page = next_active_page;
	active_page->second.SetVisible(node, true);
	return true;
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
		return false;
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

		std::string optionname = cat + "." + name;
		GUIOPTION & option = optionmap[optionname];

		option.SetInfo(text, desc, type);
		option.SetMinMaxPercentage(min, max, percentage);

		// different ways to populate the options
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

	return true;
}

bool GUI::SetLabelText(const std::string & pagename, const std::string & labelname, const std::string & text)
{
	if (pages.find(pagename) == pages.end())
		return false;

	SCENENODE & pagenode = GetPageNode(pagename);
	GUILABEL * label = GetPage(pagename).GetLabel(labelname);
	if (!label)
		return false;

	label->SetText(pagenode, text);

	return true;
}

bool GUI::GetLabelText(const std::string & pagename, const std::string & labelname, std::string & text_output)
{
	if (pages.find(pagename) == pages.end())
		return false;

	GUILABEL * label = GetPage(pagename).GetLabel(labelname);
	if (!label)
		return false;

	text_output = label->GetText();

	return true;
}

bool GUI::SetButtonEnabled(const std::string & pagename, const std::string & buttonname, bool enable)
{
	if (pages.find(pagename) == pages.end())
		return false;

	SCENENODE & pagenode = GetPageNode(pagename);
	GUIBUTTON * button = GetPage(pagename).GetButton(buttonname);
	if (!button)
		return false;

	button->SetEnabled(pagenode, enable);

	return true;
}

std::string GUI::GetOptionValue(const std::string & name) const
{
	std::map<std::string, GUIOPTION>::const_iterator it = optionmap.find(name);
	if (it != optionmap.end())
	{
		return it->second.GetCurrentStorageValue();
	}
	return std::string();
}

void GUI::SetOptionValue(const std::string & name, const std::string & value)
{
	std::map<std::string, GUIOPTION>::iterator it = optionmap.find(name);
	if (it != optionmap.end())
	{
		it->second.SetCurrentValue(value);
	}
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
