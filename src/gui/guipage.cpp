#include "gui/guipage.h"
#include "gui/guioption.h"
#include "gui/guiwidget.h"
#include "gui/guiimage.h"
#include "gui/guimultiimage.h"
#include "gui/guilabel.h"
#include "gui/guibutton.h"
#include "gui/guistringwheel.h"
#include "gui/guislider.h"
#include "gui/guispinningcar.h"
#include "gui/guicontrolgrab.h"
#include "gui/guicolorpicker.h"
#include "contentmanager.h"
#include "textureinfo.h"
#include "config.h"

#include <fstream>
#include <sstream>

GUIPAGE::GUIPAGE() :
	default_widget(0),
	active_widget(0),
	active_widget_next(0),
	tooltip_widget(0),
	dialog(false)
{
	// ctor
}

GUIPAGE::~GUIPAGE()
{
	for (std::vector <GUIWIDGET *>::iterator i = pwidgets.begin(); i != pwidgets.end(); ++i)
	{
		delete *i;
	}
	for (std::vector <GUICONTROL *>::iterator i = awidgets.begin(); i != awidgets.end(); ++i)
	{
		delete *i;
	}
}

bool GUIPAGE::Load(
	const std::string & path,
	const std::string & texpath,
	const PATHMANAGER & pathmanager,
	const float screenhwratio,
	const CONFIG & controlsconfig,
	const FONT & font,
	const std::map <std::string, std::string> & languagemap,
	std::map <std::string, GUIOPTION> & optionmap,
	std::map <std::string, Slot0*> actionmap,
	SCENENODE & parentnode,
	ContentManager & content,
	std::ostream & error_output)
{
	TEXTUREINFO texinfo;
	texinfo.mipmap = false;
	texinfo.repeatu = false;
	texinfo.repeatv = false;

	assert(!s.valid());

	Clear(parentnode);

	s = parentnode.AddNode();
	SCENENODE & sref = GetNode(parentnode);

	//error_output << "Loading " << path << std::endl;

	CONFIG pagefile;
	if (!pagefile.Load(path))
	{
		error_output << "Couldn't find GUI page file: " << path << std::endl;
		return false;
	}

	if (!pagefile.GetParam("", "name", name)) return false;
	if (!pagefile.GetParam("", "dialog", dialog)) return false;

	// set oncancel event handler
	std::string actionstr;
	if (pagefile.GetParam("", "oncancel", actionstr))
	{
		GUICONTROL::SetActions(actionmap, actionstr, oncancel);
	}

	// draw order offset
	float z0 = 100;

	// tooltip widget
	{
		float yscale(0.03), xscale(yscale * screenhwratio);
		float y(0.95), x(0.5), w(1.0), h(yscale);
		float r(1), g(1), b(1);
		int align(0);
		tooltip_widget = new GUILABEL();
		tooltip_widget->SetupDrawable(
			sref, font, align, xscale, yscale,
			x, y, w, h, z0, r, g, b);
		pwidgets.push_back(tooltip_widget);
	}

	// load widgets
	GUICONTROL * selected_widget = 0;
	std::vector<std::string> awidgetsid;
	for (CONFIG::const_iterator section = pagefile.begin(); section != pagefile.end(); ++section)
	{
		if (section->first.empty()) continue;

		// required
		std::string type;
		std::vector<float> xy(2);
		if (!pagefile.GetParam(section, "type", type, error_output)) return false;
		if (!pagefile.GetParam(section, "center", xy, error_output)) return false;

		// optional
		bool selected(false);
		float fontsize(0.03), w(0), h(0), z(0);
		std::vector<float> rgb(3, 1.0);
		std::string alignstr, text, desc;
		int align(0);
		pagefile.GetParam(section, "selected", selected);
		pagefile.GetParam(section, "fontsize", fontsize);
		pagefile.GetParam(section, "width", w);
		pagefile.GetParam(section, "height", h);
		pagefile.GetParam(section, "layer", z);
		pagefile.GetParam(section, "color", rgb);
		pagefile.GetParam(section, "align", alignstr);
		pagefile.GetParam(section, "text", text);
		pagefile.GetParam(section, "tip", desc);

		float scaley = fontsize;
		float scalex = fontsize * screenhwratio;
		z = z + z0;

		if (alignstr == "right") align = 1;
		else if (alignstr == "left") align = -1;

		if (w == 0) w = fontsize;
		if (h == 0) h = fontsize;

		std::map<std::string, std::string>::const_iterator li;
		if ((li = languagemap.find(text)) != languagemap.end()) text = li->second;
		if ((li = languagemap.find(desc)) != languagemap.end()) desc = li->second;

		if (type == "image")
		{
			std::string texname;
			std::tr1::shared_ptr<TEXTURE> texture;
			if (!pagefile.GetParam(section, "filename", texname, error_output)) return false;
			if (!content.load(texpath, texname, texinfo, texture)) return false;

			GUIIMAGE * new_widget = new GUIIMAGE();
			new_widget->SetupDrawable(sref, texture, xy[0], xy[1], w, h, z);
			pwidgets.push_back(new_widget);
		}
		else if (type == "multi-image")
		{
			std::string setting, prefix, postfix;
			if (!pagefile.GetParam(section, "setting", setting, error_output)) return false;
			if (!pagefile.GetParam(section, "prefix", prefix, error_output)) return false;
			if (!pagefile.GetParam(section, "postfix", postfix, error_output)) return false;

			GUIMULTIIMAGE * new_widget = new GUIMULTIIMAGE();
			new_widget->SetupDrawable(
				sref, content, optionmap, setting, prefix, postfix,
				xy[0], xy[1], w, h, error_output, z);
			pwidgets.push_back(new_widget);
		}
		else if (type == "label")
		{
			GUILABEL * new_widget = new GUILABEL();
			new_widget->SetupDrawable(
				sref, font, align, scalex, scaley,
				xy[0], xy[1], w, h, z,
				rgb[0], rgb[1], rgb[2]);
			new_widget->SetText(text);
			pwidgets.push_back(new_widget);

			std::string name;
			if (pagefile.GetParam(section, "name", name))
				labels[name] = new_widget;
		}
		else if (type == "button")
		{
			bool enabled = true;
			pagefile.GetParam(section, "enabled", enabled);

			GUIBUTTON * new_widget = new GUIBUTTON();
			new_widget->SetupDrawable(
				sref, font, align, scalex, scaley,
				xy[0], xy[1], w, h, z,
				rgb[0], rgb[1], rgb[2]);
			new_widget->SetText(text);
			new_widget->SetDescription(desc);
			new_widget->SetEnabled(sref, enabled);
			awidgets.push_back(new_widget);
			awidgetsid.push_back(section->first);

			if (selected)
				selected_widget = new_widget;

			std::string name;
			if (pagefile.GetParam(section, "name", name))
				buttons[name] = new_widget;
		}
		else if (type == "stringwheel" || type == "intwheel" || type == "floatwheel")
		{
			std::string setting;
			pagefile.GetParam(section, "setting", setting);

			std::map <std::string, GUIOPTION>::const_iterator opt = optionmap.find(setting);
			if (opt == optionmap.end())
			{
				error_output << path << ": widget option " << setting << " not found." << std::endl;
				return false;
			}
			desc = opt->second.GetDescription();

			std::tr1::shared_ptr<TEXTURE> bgtex;
			if (!content.load(texpath, "white.png", texinfo, bgtex)) return false;

			GUISTRINGWHEEL * new_widget = new GUISTRINGWHEEL();
			new_widget->SetupDrawable(
				sref, bgtex, optionmap, setting,
				font, scalex, scaley,
				xy[0], xy[1], w, h, z,
				error_output);
			new_widget->SetDescription(desc);
			awidgets.push_back(new_widget);
			awidgetsid.push_back(section->first);

			if (selected)
				selected_widget = new_widget;
		}
		else if (type == "colorpicker")
		{
			std::string setting;
			if (!pagefile.GetParam(section, "setting", setting, error_output)) return false;

			std::tr1::shared_ptr<TEXTURE> cursor, hue, sat, bg;
			if (!content.load(texpath, "widgets/color_cursor.png", texinfo, cursor)) return false;
			if (!content.load(texpath, "widgets/color_hue.png", texinfo, hue)) return false;
			if (!content.load(texpath, "widgets/color_saturation.png", texinfo, sat)) return false;
			if (!content.load(texpath, "widgets/color_value.png", texinfo, bg)) return false;

			float x = xy[0] - w / 2;
			float y = xy[1] - h / 2;
			GUICOLORPICKER * new_widget = new GUICOLORPICKER();
			new_widget->SetupDrawable(
				sref, cursor, hue, sat, bg, x, y, w, h,
				optionmap, setting, error_output, z);
			awidgets.push_back(new_widget);
			awidgetsid.push_back(section->first);

			if (selected)
				selected_widget = new_widget;
		}
		else if (type == "slider")
		{
			std::string setting;
			bool fill = false;
			if (!pagefile.GetParam(section, "setting", setting, error_output)) return false;
			pagefile.GetParam(section, "fill", fill);

			std::map <std::string, GUIOPTION>::const_iterator opt = optionmap.find(setting);
			if (opt == optionmap.end())
			{
				error_output << path << ": widget option " << setting << " not found." << std::endl;
				return false;
			}
			float min = opt->second.GetMin();
			float max = opt->second.GetMax();
			bool percent = opt->second.GetPercentage();
			desc = opt->second.GetDescription();

			std::tr1::shared_ptr<TEXTURE> bgtex, bartex;
			if (!content.load(texpath, "white.png", texinfo, bgtex)) return false;
			if (!content.load(texpath, "white.png", texinfo, bartex)) return false;

			GUISLIDER * new_widget = new GUISLIDER();
			new_widget->SetupDrawable(
				sref, bgtex, bartex,
				optionmap, setting,
				font, scalex, scaley,
				xy[0], xy[1], w, h, z,
				min, max, percent, fill,
				error_output);
			new_widget->SetDescription(desc);
			new_widget->SetColor(sref, rgb[0], rgb[1], rgb[2]);
			awidgets.push_back(new_widget);
			awidgetsid.push_back(section->first);

			if (selected)
				selected_widget = new_widget;
		}
		else if (type == "controlgrab")
		{
			std::string setting;
			bool analog = false;
			bool once = false;
			if (!pagefile.GetParam(section, "setting", setting, error_output)) return false;
			pagefile.GetParam(section, "analog", analog);
			pagefile.GetParam(section, "only_one", once);

			GUICONTROLGRAB * new_widget = new GUICONTROLGRAB();
			new_widget->SetupDrawable(
				sref, setting, controlsconfig, font,
				scalex, scaley, xy[0], xy[1], z,
				analog, once);
			controlgrabs.push_back(new_widget);
			awidgets.push_back(new_widget);
			awidgetsid.push_back(section->first);

			if (selected)
				selected_widget = new_widget;

			std::map<std::string, GUIOPTION>::iterator i = optionmap.find("controledit.string");
			if (i != optionmap.end())
				i->second.set_val.connect(new_widget->signal_control);
		}
		else if (type == "spinningcar")
		{
			std::vector<float> carpos(3);
			std::string setting;
			if (!pagefile.GetParam(section, "carpos", carpos, error_output)) return false;
			if (!pagefile.GetParam(section, "setting", setting, error_output)) return false;

			GUISPINNINGCAR * new_widget = new GUISPINNINGCAR();
			new_widget->SetupDrawable(
				sref, content, pathmanager, optionmap,
				xy[0], xy[1], MATHVECTOR<float, 3>(carpos[0], carpos[1], carpos[2]),
				setting, error_output, z + 10);
			pwidgets.push_back(new_widget);
		}
		else if (type != "disabled")
		{
			error_output << path << ": unknown " << section->first << " type: " << type << ", ignoring" << std::endl;
		}
	}

	// register widget actions, extra pass to avoid reallocations
	widget_activate.reserve(awidgets.size());
	for (size_t i = 0; i < awidgets.size(); ++i)
	{
		widget_activate.push_back(WIDGETCB());
		widget_activate.back().page = this;
		widget_activate.back().widget = awidgets[i];
		actionmap[awidgetsid[i]] = &widget_activate.back().action;
	}

	// set widget actions
	for (size_t i = 0; i < awidgets.size(); ++i)
	{
		awidgets[i]->RegisterActions(actionmap, awidgetsid[i], pagefile);
	}

	// set active widget
	if (!selected_widget && !awidgets.empty())
		selected_widget = awidgets[0];

	default_widget = active_widget = selected_widget;

	if (active_widget)
		tooltip_widget->SetText(active_widget->GetDescription());

	return true;
}

void GUIPAGE::UpdateControls(SCENENODE & parentnode, const CONFIG & controls, const FONT & font)
{
	assert(s.valid());
	SCENENODE & sref = GetNode(parentnode);
	for (std::vector <GUICONTROLGRAB *>::iterator i = controlgrabs.begin(); i != controlgrabs.end(); ++i)
	{
		(*i)->LoadControls(sref, controls, font);
	}
}

void GUIPAGE::SetVisible(SCENENODE & parent, bool value)
{
	SCENENODE & sref = GetNode(parent);
	for (std::vector <GUIWIDGET *>::iterator i = pwidgets.begin(); i != pwidgets.end(); ++i)
	{
		(*i)->SetVisible(sref, value);
	}
	for (std::vector <GUICONTROL *>::iterator i = awidgets.begin(); i != awidgets.end(); ++i)
	{
		(*i)->SetVisible(sref, value);
	}

	// reset active widget
	if (!value && default_widget)
		SetActiveWidget(*default_widget);
}

void GUIPAGE::SetAlpha(SCENENODE & parent, float value)
{
	SCENENODE & sref = parent.GetNode(s);
	for (std::vector <GUIWIDGET *>::iterator i = pwidgets.begin(); i != pwidgets.end(); ++i)
	{
		(*i)->SetAlpha(sref, value);
	}
	for (std::vector <GUICONTROL *>::iterator i = awidgets.begin(); i != awidgets.end(); ++i)
	{
		(*i)->SetAlpha(sref, value * inactive_alpha);
	}

	// set active widget
	if (active_widget)
		active_widget->SetAlpha(sref, value);
}

void GUIPAGE::ProcessInput(
	SCENENODE & parent,
	float cursorx, float cursory,
	bool cursordown, bool cursorjustup,
	bool moveleft, bool moveright,
	bool moveup, bool movedown,
	bool select, bool cancel,
	float screenhwratio)
{
	if (cancel)
	{
		oncancel();
		return;
	}

	// set active widget
	SCENENODE & sref = parent.GetNode(s);
	for (std::vector <GUICONTROL *>::iterator i = awidgets.begin(); i != awidgets.end(); ++i)
	{
		if ((**i).ProcessInput(sref, cursorx, cursory, cursordown, cursorjustup))
		{
			SetActiveWidget(**i);
			break;
		}
	}

	// process events
	if (active_widget)
	{
		if (select)
			active_widget->OnSelect();
		else if (moveleft)
			active_widget->OnMoveLeft();
		else if (moveright)
			active_widget->OnMoveRight();
		else if (moveup)
			active_widget->OnMoveUp();
		else if (movedown)
			active_widget->OnMoveDown();
	}
}

void GUIPAGE::Update(SCENENODE & parent, float dt)
{
	SCENENODE & sref = parent.GetNode(s);
	for (std::vector <GUIWIDGET *>::iterator i = pwidgets.begin(); i != pwidgets.end(); ++i)
	{
		(*i)->Update(sref, dt);
	}
	for (std::vector <GUICONTROL *>::iterator i = awidgets.begin(); i != awidgets.end(); ++i)
	{
		(*i)->Update(sref, dt);
	}
}

void GUIPAGE::Clear(SCENENODE & parentnode)
{
	for (std::vector <GUIWIDGET *>::iterator i = pwidgets.begin(); i != pwidgets.end(); ++i)
	{
		delete *i;
	}
	for (std::vector <GUICONTROL *>::iterator i = awidgets.begin(); i != awidgets.end(); ++i)
	{
		delete *i;
	}

	pwidgets.clear();
	awidgets.clear();
	labels.clear();
	buttons.clear();
	controlgrabs.clear();
	tooltip_widget = 0;
	dialog = false;

	if (s.valid())
	{
		SCENENODE & sref = parentnode.GetNode(s);
		sref.Clear();
	}
	s.invalidate();
}

void GUIPAGE::SetActiveWidget(GUICONTROL & widget)
{
	if (active_widget != &widget)
	{
		active_widget = &widget;
		tooltip_widget->SetText(active_widget->GetDescription());
	}
}

GUIPAGE::WIDGETCB::WIDGETCB()
{
	action.call.bind<WIDGETCB, &WIDGETCB::call>(this);
	page = 0;
	widget = 0;
}

GUIPAGE::WIDGETCB::WIDGETCB(const WIDGETCB & other)
{
	*this = other;
}

GUIPAGE::WIDGETCB & GUIPAGE::WIDGETCB::operator=(const WIDGETCB & other)
{
	action.call.bind<WIDGETCB, &WIDGETCB::call>(this);
	page = other.page;
	widget = other.widget;
	return *this;
}

void GUIPAGE::WIDGETCB::call()
{
	assert(page && widget);
	page->SetActiveWidget(*widget);
}
