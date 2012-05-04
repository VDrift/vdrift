#include "guipage.h"
#include "guioption.h"
#include "config.h"
#include "contentmanager.h"
#include "textureinfo.h"
#include "widget.h"
#include "widget_image.h"
#include "widget_multiimage.h"
#include "widget_label.h"
#include "widget_button.h"
#include "widget_stringwheel.h"
#include "widget_doublestringwheel.h"
#include "widget_slider.h"
#include "widget_spinningcar.h"
#include "widget_controlgrab.h"
#include "widget_colorpicker.h"

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
	for (std::vector <WIDGET *>::iterator i = widgets.begin(); i != widgets.end(); ++i)
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
		std::string text;
		float y(0.95);
		float x(0.5);
		float yscale(0.03);
		float xscale(yscale * screenhwratio);
		float r(1), g(1), b(1);
		tooltip_widget = new WIDGET_LABEL();
		tooltip_widget->SetupDrawable(sref, font, text, x, y, xscale, yscale, r, g, b, z0);
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
		float w(0), h(0), z(0);
		std::string text, desc;
		pagefile.GetParam(section, "width", w);
		pagefile.GetParam(section, "height", h);
		pagefile.GetParam(section, "layer", z);
		pagefile.GetParam(section, "text", text);
		pagefile.GetParam(section, "tip", desc);
		z += z0;

		std::map<std::string, std::string>::const_iterator li;
		if ((li = languagemap.find(text)) != languagemap.end()) text = li->second;
		if ((li = languagemap.find(desc)) != languagemap.end()) desc = li->second;

		if (type == "image")
		{
			std::string texname;
			std::tr1::shared_ptr<TEXTURE> texture;
			if (!pagefile.GetParam(section, "filename", texname, error_output)) return false;
			if (!content.load(texpath, texname, texinfo, texture)) return false;

			WIDGET_IMAGE * new_widget = new WIDGET_IMAGE();
			new_widget->SetupDrawable(sref, texture, xy[0], xy[1], w, h, z);
			widgets.push_back(new_widget);
		}
		else if (type == "button")
		{
			float fontsize(0);
			std::vector<float> color(3, 1.0);
			std::string action;
			bool cancel = false;
			bool enabled = true;

			if (!pagefile.GetParam(section, "fontsize", fontsize, error_output)) return false;
			if (!pagefile.GetParam(section, "color", color, error_output)) return false;
			if (!pagefile.GetParam(section, "action", action, error_output)) return false;
			if (!pagefile.GetParam(section, "cancel", cancel, error_output)) return false;
			pagefile.GetParam(section, "enabled", enabled);

			std::tr1::shared_ptr<TEXTURE> texture_up, texture_down, texture_sel;
			if (!content.load(texpath, "widgets/btn_up_unsel.png", texinfo, texture_sel)) return false;
			if (!content.load(texpath, "widgets/btn_down.png", texinfo, texture_down)) return false;
			if (!content.load(texpath, "widgets/btn_up.png", texinfo, texture_up)) return false;

			float fontscaley = fontsize;
			float fontscalex = fontsize * screenhwratio;

			WIDGET_BUTTON * new_widget = new WIDGET_BUTTON();
			new_widget->SetupDrawable(sref, texture_up, texture_down, texture_sel,
					font, text, xy[0], xy[1], fontscalex, fontscaley,
					color[0], color[1], color[2], h, w, z);
			new_widget->SetAction(action);
			new_widget->SetDescription(desc);
			new_widget->SetCancel(cancel);
			new_widget->SetEnabled(sref, enabled);
			widgets.push_back(new_widget);

			std::string name;
			if (pagefile.GetParam(section, "name", name))
			{
				button_widgets[name] = *new_widget;
			}
		}
		else if (type == "label")
		{
			float fontsize;
			std::vector<float> color(3, 1.0);

			if (!pagefile.GetParam(section, "fontsize", fontsize, error_output)) return false;
			pagefile.GetParam(section, "color", color);

			float fontscaley = fontsize;
			float fontscalex = fontsize * screenhwratio;

			WIDGET_LABEL * new_widget = new WIDGET_LABEL();
			new_widget->SetupDrawable(sref, font, text, xy[0], xy[1], fontscalex, fontscaley, color[0], color[1], color[2], z);
			widgets.push_back(new_widget);

			std::string name;
			if (pagefile.GetParam(section, "name", name))
			{
				label_widgets[name] = *new_widget;
			}
		}
		else if (type == "stringwheel" || type == "intwheel" || type == "floatwheel")
		{
			std::string setting;
			std::string values;
			std::string action;
			float spacing(0.3);
			float fontsize;

			if (!pagefile.GetParam(section, "setting", setting, error_output)) return false;
			if (!pagefile.GetParam(section, "values", values, error_output)) return false;
			if (!pagefile.GetParam(section, "fontsize", fontsize, error_output)) return false;
			pagefile.GetParam(section, "spacing", spacing);
			pagefile.GetParam(section, "action", action);

			if (values == "options")
			{
				text = optionmap[setting].GetText();
				desc = optionmap[setting].GetDescription();
			}
			else
			{
				error_output << "Widget " << section->first << ": unknown value type " << values << std::endl;
			}

			std::tr1::shared_ptr<TEXTURE> up_left, down_left, up_right, down_right;
			if (!content.load(texpath, "widgets/wheel_up_l.png", texinfo, up_left)) return false;
			if (!content.load(texpath, "widgets/wheel_down_l.png", texinfo, down_left)) return false;
			if (!content.load(texpath, "widgets/wheel_up_r.png", texinfo, up_right)) return false;
			if (!content.load(texpath, "widgets/wheel_down_r.png", texinfo, down_right)) return false;

			float fontscaley = fontsize;
			float fontscalex = fontsize * screenhwratio;
			xy[0] -= spacing * 0.5;

			WIDGET_STRINGWHEEL * new_widget = new WIDGET_STRINGWHEEL();
			new_widget->SetupDrawable(
				sref, optionmap, setting, text + ":",
				up_left, down_left, up_right, down_right,
				font, fontscalex, fontscaley, xy[0], xy[1], z);
			new_widget->SetDescription(desc);
			new_widget->SetAction(action);
			widgets.push_back(new_widget);
		}
		else if (type == "intintwheel")
		{
			std::string setting1, setting2, values;
			float spacing(0.3);
			float fontsize;

			if (!pagefile.GetParam(section, "setting1", setting1, error_output)) return false;
			if (!pagefile.GetParam(section, "setting2", setting2, error_output)) return false;
			if (!pagefile.GetParam(section, "values", values, error_output)) return false;
			if (!pagefile.GetParam(section, "fontsize", fontsize, error_output)) return false;
			pagefile.GetParam(section, "spacing", spacing);

			if (values == "options")
			{
				text = optionmap[setting1].GetText();
				desc = optionmap[setting1].GetDescription();
			}
			else
			{
				error_output << "Widget " << section->first << ": unknown value type " << values << std::endl;
			}

			std::tr1::shared_ptr<TEXTURE> up_left, down_left, up_right, down_right;
			if (!content.load(texpath, "widgets/wheel_up_l.png", texinfo, up_left)) return false;
			if (!content.load(texpath, "widgets/wheel_down_l.png", texinfo, down_left)) return false;
			if (!content.load(texpath, "widgets/wheel_up_r.png", texinfo, up_right)) return false;
			if (!content.load(texpath, "widgets/wheel_down_r.png", texinfo, down_right)) return false;

			float fontscaley = fontsize;
			float fontscalex = fontsize * screenhwratio;
			xy[0] -= spacing * 0.5;

			WIDGET_DOUBLESTRINGWHEEL * new_widget = new WIDGET_DOUBLESTRINGWHEEL();
			new_widget->SetupDrawable(
				sref, optionmap, setting1, setting2, text + ":",
				up_left, down_left, up_right, down_right,
				font, fontscalex, fontscaley, xy[0], xy[1], z);
			new_widget->SetDescription(desc);
			widgets.push_back(new_widget);
		}
		else if (type == "multi-image")
		{
			std::string setting, values, prefix, postfix;
			if (!pagefile.GetParam(section, "setting", setting, error_output)) return false;
			if (!pagefile.GetParam(section, "values", values, error_output)) return false;
			if (!pagefile.GetParam(section, "prefix", prefix, error_output)) return false;
			if (!pagefile.GetParam(section, "postfix", postfix, error_output)) return false;

			WIDGET_MULTIIMAGE * new_widget = new WIDGET_MULTIIMAGE();
			new_widget->SetupDrawable(
				sref, content, optionmap, setting, prefix, postfix,
				xy[0], xy[1], w, h, error_output, z);
			widgets.push_back(new_widget);
		}
		else if (type == "colorpicker")
		{
			std::string setting, action, name;
			std::tr1::shared_ptr<TEXTURE> cursor, hue, sat, bg;
			if (!content.load(texpath, "widgets/color_cursor.png", texinfo, cursor)) return false;
			if (!content.load(texpath, "widgets/color_hue.png", texinfo, hue)) return false;
			if (!content.load(texpath, "widgets/color_saturation.png", texinfo, sat)) return false;
			if (!content.load(texpath, "widgets/color_value.png", texinfo, bg)) return false;
			if (!pagefile.GetParam(section, "setting", setting, error_output)) return false;
			pagefile.GetParam(section, "action", action);

			float x = xy[0] - w / 2;
			float y = xy[1] - h / 2;
			WIDGET_COLORPICKER * new_widget = new WIDGET_COLORPICKER();
			new_widget->SetupDrawable(
				sref, cursor, hue, sat, bg, x, y, w, h,
				optionmap, setting, error_output, z);
			new_widget->SetAction(action);
			widgets.push_back(new_widget);
		}
		else if (type == "slider")
		{
			float min(0), max(1);
			float fontsize;
			bool percentage(false);
			std::string name, setting, values;

			if (!pagefile.GetParam(section, "name", name, error_output)) return false;
			if (!pagefile.GetParam(section, "setting", setting, error_output)) return false;
			if (!pagefile.GetParam(section, "values", values, error_output)) return false;
			if (!pagefile.GetParam(section, "fontsize", fontsize, error_output)) return false;

			if (values == "manual")
			{
				if (!pagefile.GetParam(section, "min", min, error_output)) return false;
				if (!pagefile.GetParam(section, "max", max, error_output)) return false;
			}
			else
			{
				text = optionmap[setting].GetText();
				desc = optionmap[setting].GetDescription();
				if (optionmap.find(setting) == optionmap.end())
				{
					error_output << path << ": slider widget option " << setting << " not found, assuming default min/max/percentage values" << std::endl;
				}
				else
				{
					min = optionmap[setting].GetMin();
					max = optionmap[setting].GetMax();
					percentage = optionmap[setting].GetPercentage();
				}
			}

			float spacing(0.3);
			pagefile.GetParam(section, "spacing", spacing);
			xy[0] -= spacing * 0.5;

			float fontscaley = fontsize;
			float fontscalex = fontsize * screenhwratio;

			//generate label
			{
				float textwidth = font.GetWidth(text) * fontscalex;
				float x = xy[0] + textwidth * 0.5;
				float y = xy[1];
				float r(1), g(1), b(1);
				WIDGET_LABEL * new_widget = new WIDGET_LABEL();
				new_widget->SetupDrawable(sref, font, text, x, y, fontscalex, fontscaley, r, g, b, z);
				widgets.push_back(new_widget);
			}

			if (h == 0.0) h = fontsize;
			if (w < h * screenhwratio) w = h * screenhwratio;
			xy[0] = xy[0] - w * 4.0 / 2 - fontscalex; // slider offset

			std::vector<float> color(3, 1.0);
			pagefile.GetParam(section, "color", color);

			std::tr1::shared_ptr<TEXTURE> cursor, wedge;
			if (!content.load(texpath, "widgets/sld_cursor.png", texinfo, cursor)) return false;
			if (!content.load(texpath, "widgets/sld_wedge.png", texinfo, wedge)) return false;

			WIDGET_SLIDER * new_widget = new WIDGET_SLIDER();
			new_widget->SetupDrawable(
				sref, wedge, cursor, xy[0], xy[1], w, h, min, max, percentage,
				optionmap, setting, font, fontscalex, fontscaley, error_output, z);
			new_widget->SetName(name);
			new_widget->SetDescription(desc);
			new_widget->SetColor(sref, color[0], color[1], color[2]);
			widgets.push_back(new_widget);
		}
		else if (type == "spinningcar")
		{
			std::vector<float> carpos(3);
			std::string setting, values, name, prefix, postfix;
			if (!pagefile.GetParam(section, "carpos", carpos, error_output)) return false;
			if (!pagefile.GetParam(section, "setting", setting, error_output)) return false;
			if (!pagefile.GetParam(section, "values", values, error_output)) return false;

			WIDGET_SPINNINGCAR * new_widget = new WIDGET_SPINNINGCAR();
			new_widget->SetupDrawable(sref, content, pathmanager, optionmap,
				xy[0], xy[1], MATHVECTOR<float, 3>(carpos[0], carpos[1], carpos[2]),
				setting, error_output, z + 10);
			widgets.push_back(new_widget);
		}
		else if (type == "controlgrab")
		{
			float fontsize;
			std::string setting;
			bool analog(false);
			bool only_one(false);

			if (!pagefile.GetParam(section, "fontsize", fontsize, error_output)) return false;
			if (!pagefile.GetParam(section, "setting", setting, error_output)) return false;
			pagefile.GetParam(section, "analog", analog);
			pagefile.GetParam(section, "only_one", only_one);

			std::vector <std::tr1::shared_ptr<TEXTURE> > control(WIDGET_CONTROLGRAB::END);
			if (!content.load(texpath, "widgets/controls/add.png", texinfo, control[WIDGET_CONTROLGRAB::ADD])) return false;
			if (!content.load(texpath, "widgets/controls/add_sel.png", texinfo, control[WIDGET_CONTROLGRAB::ADDSEL])) return false;
			if (!content.load(texpath, "widgets/controls/joy_axis.png", texinfo, control[WIDGET_CONTROLGRAB::JOYAXIS])) return false;
			if (!content.load(texpath, "widgets/controls/joy_axis_sel.png", texinfo, control[WIDGET_CONTROLGRAB::JOYAXISSEL])) return false;
			if (!content.load(texpath, "widgets/controls/joy_btn.png", texinfo, control[WIDGET_CONTROLGRAB::JOYBTN])) return false;
			if (!content.load(texpath, "widgets/controls/joy_btn_sel.png", texinfo, control[WIDGET_CONTROLGRAB::JOYBTNSEL])) return false;
			if (!content.load(texpath, "widgets/controls/key.png", texinfo, control[WIDGET_CONTROLGRAB::KEY])) return false;
			if (!content.load(texpath, "widgets/controls/key_sel.png", texinfo, control[WIDGET_CONTROLGRAB::KEYSEL])) return false;
			if (!content.load(texpath, "widgets/controls/mouse.png", texinfo, control[WIDGET_CONTROLGRAB::MOUSE])) return false;
			if (!content.load(texpath, "widgets/controls/mouse_sel.png", texinfo, control[WIDGET_CONTROLGRAB::MOUSESEL])) return false;

			float fontscaley = fontsize;
			float fontscalex = fontsize * screenhwratio;

			WIDGET_CONTROLGRAB * new_widget = new WIDGET_CONTROLGRAB();
			new_widget->SetupDrawable(sref, controlsconfig, setting, control, font,
					text, xy[0], xy[1], fontscalex, fontscaley, analog, only_one, z);
			controlgrabs.push_back(new_widget);
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
	for (std::vector <WIDGET_CONTROLGRAB *>::iterator i = controlgrabs.begin(); i != controlgrabs.end(); ++i)
	{
		(*i)->LoadControls(sref, controls, font);
	}
}

void GUIPAGE::SetVisible(SCENENODE & parent, const bool newvis)
{
	SCENENODE & sref = GetNode(parent);
	for (std::vector <WIDGET *>::iterator i = widgets.begin(); i != widgets.end(); ++i)
	{
		(*i)->SetVisible(sref, newvis);
	}
}

void GUIPAGE::SetAlpha(SCENENODE & parent, const float newalpha)
{
	SCENENODE & sref = parent.GetNode(s);
	for (std::vector <WIDGET *>::iterator i = widgets.begin(); i != widgets.end(); ++i)
	{
		(*i)->SetAlpha(sref, newalpha);
	}
}

void GUIPAGE::UpdateOptions(
	SCENENODE & parent,
	bool save_to,
	std::map<std::string, GUIOPTION> & optionmap,
	std::ostream & error_output)
{
	SCENENODE & sref = parent.GetNode(s);
	for (std::vector <WIDGET *>::iterator i = widgets.begin(); i != widgets.end(); ++i)
	{
		(*i)->UpdateOptions(sref, save_to, optionmap, error_output);
	}
}

std::list <std::pair <std::string, bool> > GUIPAGE::ProcessInput(
	SCENENODE & parent,
	std::map<std::string, GUIOPTION> & optionmap,
	bool movedown, bool moveup,
	float cursorx, float cursory,
	bool cursordown, bool cursorjustup,
	float screenhwratio)
{
	assert(tooltip_widget);

	SCENENODE & sref = parent.GetNode(s);

	std::list <std::pair <std::string, bool> > actions;
	std::string tooltip;

	for (std::vector <WIDGET *>::iterator i = widgets.begin(); i != widgets.end(); ++i)
	{
		WIDGET & w = **i;

		bool mouseover = w.ProcessInput(
			sref, optionmap,
			cursorx, cursory,
			cursordown, cursorjustup);

		if (mouseover)
		{
			tooltip = w.GetDescription();
		}

		std::string action = w.GetAction();
		bool cancel = w.GetCancel();
		if (!action.empty())
		{
			actions.push_back(std::pair <std::string, bool> (action, !cancel));
		}
	}

	if (tooltip != tooltip_widget->GetText())
	{
		tooltip_widget->SetText(sref, tooltip);
	}

	return actions;
}

void GUIPAGE::Update(SCENENODE & parent, float dt)
{
	SCENENODE & sref = parent.GetNode(s);
	for (std::vector <WIDGET *>::iterator i = widgets.begin(); i != widgets.end(); ++i)
	{
		(*i)->Update(sref, dt);
	}
}

void GUIPAGE::Clear(SCENENODE & parentnode)
{
	controlgrabs.clear();
	tooltip_widget = 0;
	dialog = false;

	for (std::vector <WIDGET *>::iterator i = widgets.begin(); i != widgets.end(); ++i)
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
