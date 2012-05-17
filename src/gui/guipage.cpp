#include "gui/guipage.h"
#include "gui/guioption.h"
#include "config.h"
#include "contentmanager.h"
#include "textureinfo.h"
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

#include <fstream>
#include <sstream>

GUIPAGE::GUIPAGE() :
	tooltip_widget(0),
	dialog(false)
{
	// ctor
}

GUIPAGE::~GUIPAGE()
{
	for (std::vector <GUIWIDGET *>::iterator i = widgets.begin(); i != widgets.end(); ++i)
	{
		delete *i;
	}
}

static void RegisterActions(
	const std::map<std::string, Slot0*> actionmap,
	const std::string & actionstr,
	Signal0 & signal)
{
	std::stringstream st(actionstr);
	while (st.good())
	{
		std::string action;
		st >> action;
		std::map<std::string, Slot0*>::const_iterator it = actionmap.find(action);
		if (it != actionmap.end())
			it->second->connect(signal);
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
	std::map <std::string, Slot0*> & actionmap,
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
		widgets.push_back(tooltip_widget);
	}

	// load widgets
	for (CONFIG::const_iterator section = pagefile.begin(); section != pagefile.end(); ++section)
	{
		if (section->first.empty()) continue;

		// required
		std::string type;
		std::vector<float> xy(2);
		if (!pagefile.GetParam(section, "type", type, error_output)) return false;
		if (!pagefile.GetParam(section, "center", xy, error_output)) return false;

		// optional
		float fontsize(0.03), w(0), h(0), z(0);
		std::vector<float> rgb(3, 1.0);
		std::string salign, text, desc;
		int align(0);
		pagefile.GetParam(section, "fontsize", fontsize);
		pagefile.GetParam(section, "width", w);
		pagefile.GetParam(section, "height", h);
		pagefile.GetParam(section, "layer", z);
		pagefile.GetParam(section, "color", rgb);
		pagefile.GetParam(section, "align", salign);
		pagefile.GetParam(section, "text", text);
		pagefile.GetParam(section, "tip", desc);

		float scaley = fontsize;
		float scalex = fontsize * screenhwratio;
		z = z + z0;

		if (salign == "right") align = 1;
		else if (salign == "left") align = -1;

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
			widgets.push_back(new_widget);
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
			widgets.push_back(new_widget);
		}
		else if (type == "label")
		{
			GUILABEL * new_widget = new GUILABEL();
			new_widget->SetupDrawable(
				sref, font, align, scalex, scaley,
				xy[0], xy[1], w, h, z,
				rgb[0], rgb[1], rgb[2]);
			new_widget->SetText(sref, text);
			widgets.push_back(new_widget);

			std::string name;
			if (pagefile.GetParam(section, "name", name))
			{
				labels[name] = new_widget;
			}
		}
		else if (type == "button")
		{
			std::string action;
			bool enabled = true;
			if (!pagefile.GetParam(section, "action", action, error_output)) return false;
			pagefile.GetParam(section, "enabled", enabled);

			GUIBUTTON * new_widget = new GUIBUTTON();
			new_widget->SetupDrawable(
				sref, font, align, scalex, scaley,
				xy[0], xy[1], w, h, z,
				rgb[0], rgb[1], rgb[2]);
			new_widget->SetText(sref, text);
			new_widget->SetDescription(desc);
			new_widget->SetEnabled(sref, enabled);
			widgets.push_back(new_widget);

			RegisterActions(actionmap, action, new_widget->signal_action);

			std::string name;
			if (pagefile.GetParam(section, "name", name))
			{
				buttons[name] = new_widget;
			}
		}
		else if (type == "stringwheel" || type == "intwheel" || type == "floatwheel")
		{
			std::string setting, action;
			if (!pagefile.GetParam(section, "setting", setting, error_output)) return false;
			pagefile.GetParam(section, "action", action);

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
			widgets.push_back(new_widget);

			RegisterActions(actionmap, action, new_widget->signal_action);
		}
		else if (type == "colorpicker")
		{
			std::string setting, action;
			std::tr1::shared_ptr<TEXTURE> cursor, hue, sat, bg;
			if (!content.load(texpath, "widgets/color_cursor.png", texinfo, cursor)) return false;
			if (!content.load(texpath, "widgets/color_hue.png", texinfo, hue)) return false;
			if (!content.load(texpath, "widgets/color_saturation.png", texinfo, sat)) return false;
			if (!content.load(texpath, "widgets/color_value.png", texinfo, bg)) return false;
			if (!pagefile.GetParam(section, "setting", setting, error_output)) return false;
			pagefile.GetParam(section, "action", action);

			float x = xy[0] - w / 2;
			float y = xy[1] - h / 2;
			GUICOLORPICKER * new_widget = new GUICOLORPICKER();
			new_widget->SetupDrawable(
				sref, cursor, hue, sat, bg, x, y, w, h,
				optionmap, setting, error_output, z);
			widgets.push_back(new_widget);

			RegisterActions(actionmap, action, new_widget->signal_action);
		}
		else if (type == "slider")
		{
			bool fill(false);
			std::string setting;
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
			widgets.push_back(new_widget);
		}
		else if (type == "controlgrab")
		{
			std::string setting;
			bool analog(false);
			bool once(false);
			if (!pagefile.GetParam(section, "setting", setting, error_output)) return false;
			pagefile.GetParam(section, "analog", analog);
			pagefile.GetParam(section, "only_one", once);

			GUICONTROLGRAB * new_widget = new GUICONTROLGRAB();
			new_widget->SetupDrawable(
				sref, setting, controlsconfig, font,
				scalex, scaley, xy[0], xy[1], z,
				analog, once);
			controlgrabs.push_back(new_widget);
			widgets.push_back(new_widget);

			std::map<std::string, GUIOPTION>::iterator i = optionmap.find("controledit.string");
			if (i != optionmap.end())
				i->second.set_val.connect(new_widget->signal_control);

			RegisterActions(actionmap, "UpdateControl", new_widget->signal_action);
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
			widgets.push_back(new_widget);
		}
		else if (type != "disabled")
		{
			error_output << path << ": unknown " << section->first << " type: " << type << ", ignoring" << std::endl;
		}
	}

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
	for (std::vector <GUIWIDGET *>::iterator i = widgets.begin(); i != widgets.end(); ++i)
	{
		(*i)->SetVisible(sref, value);
	}
}

void GUIPAGE::SetAlpha(SCENENODE & parent, float value)
{
	SCENENODE & sref = parent.GetNode(s);
	for (std::vector <GUIWIDGET *>::iterator i = widgets.begin(); i != widgets.end(); ++i)
	{
		(*i)->SetAlpha(sref, value);
	}
}

void GUIPAGE::ProcessInput(
	SCENENODE & parent,
	bool movedown, bool moveup,
	float cursorx, float cursory,
	bool cursordown, bool cursorjustup,
	float screenhwratio)
{
	SCENENODE & sref = parent.GetNode(s);
	std::string tooltip;

	for (std::vector <GUIWIDGET *>::iterator i = widgets.begin(); i != widgets.end(); ++i)
	{
		GUIWIDGET & w = **i;

		bool focus = w.ProcessInput(
			sref, cursorx, cursory,
			cursordown, cursorjustup);

		if (focus)
			tooltip = w.GetDescription();
	}

	assert(tooltip_widget);
	if (tooltip != tooltip_widget->GetText())
	{
		tooltip_widget->SetText(sref, tooltip);
	}
}

void GUIPAGE::Update(SCENENODE & parent, float dt)
{
	SCENENODE & sref = parent.GetNode(s);
	for (std::vector <GUIWIDGET *>::iterator i = widgets.begin(); i != widgets.end(); ++i)
	{
		(*i)->Update(sref, dt);
	}
}

void GUIPAGE::Clear(SCENENODE & parentnode)
{
	controlgrabs.clear();
	tooltip_widget = 0;
	dialog = false;

	for (std::vector <GUIWIDGET *>::iterator i = widgets.begin(); i != widgets.end(); ++i)
	{
		delete *i;
	}
	widgets.clear();

	if (s.valid())
	{
		SCENENODE & sref = parentnode.GetNode(s);
		sref.Clear();
	}
	s.invalidate();
}
