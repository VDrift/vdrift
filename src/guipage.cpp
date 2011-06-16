#include "guipage.h"
#include "config.h"
#include "texturemanager.h"
#include "textureinfo.h"
#include "widget.h"
#include "widget_image.h"
#include "widget_multiimage.h"
#include "widget_label.h"
#include "widget_button.h"
#include "widget_toggle.h"
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
	//std::clog << "Page Destructor: " << name << std::endl;
}

bool GUIPAGE::Load(
	const std::string & path,
	const std::string & texpath,
	const std::string & datapath,
	const std::string & texsize,
	const float screenhwratio,
	const CONFIG & controlsconfig,
	const FONT & font,
	const std::map <std::string, std::string> & languagemap,
	std::map <std::string, GUIOPTION> & optionmap,
	SCENENODE & parentnode,
	TEXTUREMANAGER & textures,
	MODELMANAGER & models,
	std::ostream & error_output)
{
	TEXTUREINFO texinfo;
	texinfo.mipmap = false;
	texinfo.repeatu = false;
	texinfo.repeatv = false;
	texinfo.size = texsize;

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

	// draw order
	float z = 100;

	//generate background
	{
		float y(0.5);
		float x(0.5);
		float yscale(1.0);
		float xscale(1.0);
		std::string background;
		std::tr1::shared_ptr<TEXTURE> texture;
		if (!pagefile.GetParam("", "background", background)) return false;
		if (!textures.Load(texpath, background, texinfo, texture)) return false;
		WIDGET_IMAGE * bg_widget = NewWidget<WIDGET_IMAGE>();
		bg_widget->SetupDrawable(sref, texture, x, y, xscale, yscale, z-1);
	}

	// tooltip widget
	{
		std::string text;
		float y(0.95);
		float x(0.5);
		float yscale(0.03);
		float xscale(yscale * screenhwratio);
		float r(1), g(1), b(1);
		tooltip_widget = NewWidget<WIDGET_LABEL>();
		tooltip_widget->SetupDrawable(sref, font, text, x, y, xscale, yscale, r, g, b, z);
	}

	// widget map for hooks
	std::map <std::string, WIDGET *> namemap;

	// load widgets
	for (CONFIG::const_iterator section = pagefile.begin(); section != pagefile.end(); ++section)
	{
		if (section->first.empty()) continue;

		std::string type;
		if (!pagefile.GetParam(section, "type", type, error_output)) return false;

		std::string text, desc;
		pagefile.GetParam(section, "text", text);
		pagefile.GetParam(section, "tip", desc);

		std::map<std::string, std::string>::const_iterator li;
		if ((li = languagemap.find(text)) != languagemap.end()) text = li->second;
		if ((li = languagemap.find(desc)) != languagemap.end()) desc = li->second;

		if (type == "image")
		{
			float w, h;
			std::vector<float> xy(2);

			if (!pagefile.GetParam(section, "center", xy, error_output)) return false;
			if (!pagefile.GetParam(section, "width", w, error_output)) return false;
			if (!pagefile.GetParam(section, "height", h, error_output)) return false;

			std::string texname;
			std::tr1::shared_ptr<TEXTURE> texture;
			if (!pagefile.GetParam(section, "filename", texname, error_output)) return false;
			if (!textures.Load(texpath, texname, texinfo, texture)) return false;

			WIDGET_IMAGE * new_widget = NewWidget<WIDGET_IMAGE>();
			new_widget->SetupDrawable(sref, texture, xy[0], xy[1], w, h, z);
		}
		else if (type == "button")
		{
			float fontsize(0), h(0), w(0);
			std::vector<float> xy(2);
			std::vector<float> color(3, 1.0);
			std::string action;
			bool cancel;

			if (!pagefile.GetParam(section, "center", xy, error_output)) return false;
			if (!pagefile.GetParam(section, "fontsize", fontsize, error_output)) return false;
			if (!pagefile.GetParam(section, "color", color, error_output)) return false;
			if (!pagefile.GetParam(section, "action", action, error_output)) return false;
			if (!pagefile.GetParam(section, "cancel", cancel, error_output)) return false;
			pagefile.GetParam(section, "height", h);
			pagefile.GetParam(section, "width", w);

			std::tr1::shared_ptr<TEXTURE> texture_up, texture_down, texture_sel;
			if (!textures.Load(texpath, "widgets/btn_up_unsel.png", texinfo, texture_sel)) return false;
			if (!textures.Load(texpath, "widgets/btn_down.png", texinfo, texture_down)) return false;
			if (!textures.Load(texpath, "widgets/btn_up.png", texinfo, texture_up)) return false;

			float fontscaley = fontsize;
			float fontscalex = fontsize * screenhwratio;

			WIDGET_BUTTON * new_widget = NewWidget<WIDGET_BUTTON>();
			new_widget->SetupDrawable(sref, texture_up, texture_down, texture_sel,
					font, text, xy[0], xy[1], fontscalex, fontscaley,
					color[0], color[1], color[2], h, w, z);
			new_widget->SetAction(action);
			new_widget->SetDescription(desc);
			new_widget->SetCancel(cancel);
		}
		else if (type == "label")
		{
			float fontsize;
			std::vector<float> xy(2);
			std::vector<float> color(3, 1.0);

			if (!pagefile.GetParam(section, "center", xy, error_output)) return false;
			if (!pagefile.GetParam(section, "fontsize", fontsize, error_output)) return false;
			pagefile.GetParam(section, "color", color);

			float fontscaley = fontsize;
			float fontscalex = fontsize * screenhwratio;

			WIDGET_LABEL * new_widget = NewWidget<WIDGET_LABEL>();
			new_widget->SetupDrawable(sref, font, text, xy[0], xy[1], fontscalex, fontscaley, color[0], color[1], color[2], z);

			std::string name;
			if (pagefile.GetParam(section, "name", name))
			{
				label_widgets[name] = *new_widget;
			}
		}
		else if (type == "toggle")
		{
			std::vector<float> xy(2);
			std::string setting;
			std::string valuetype;
			float spacing(0.3);
			float fontsize;

			if (!pagefile.GetParam(section, "center", xy, error_output)) return false;
			if (!pagefile.GetParam(section, "setting", setting, error_output)) return false;
			if (!pagefile.GetParam(section, "fontsize", fontsize, error_output)) return false;
			if (!pagefile.GetParam(section, "values", valuetype, error_output)) return false;
			pagefile.GetParam(section, "spacing", spacing);

			if (valuetype == "options")
			{
				text = optionmap[setting].GetText();
				desc = optionmap[setting].GetDescription();
			}
			else if (valuetype == "manual")
			{
				std::string truestr, falsestr;
				if (!pagefile.GetParam(section, "true", truestr, error_output)) return false;
				if (!pagefile.GetParam(section, "false", falsestr, error_output)) return false;
				text = truestr;
			}

			float fontscaley = fontsize;
			float fontscalex = fontsize * screenhwratio;
			xy[0] -= spacing * 0.5;

			//generate label
			{
				float textwidth = font.GetWidth(text) * fontscalex;
				float x = xy[0] + textwidth * 0.5;
				float y = xy[1];
				float r(1), g(1), b(1);
				WIDGET_LABEL * new_widget = NewWidget<WIDGET_LABEL>();
				new_widget->SetupDrawable(sref, font, text, x, y, fontscalex, fontscaley, r, g, b, z);
			}

			//generate toggle
			{
				std::tr1::shared_ptr<TEXTURE> up, down, upsel, downsel, trans;
				if (!textures.Load(texpath, "widgets/tog_off_up_unsel.png", texinfo, up)) return false;
				if (!textures.Load(texpath, "widgets/tog_on_up_unsel.png", texinfo, down)) return false;
				if (!textures.Load(texpath, "widgets/tog_off_up.png", texinfo, upsel)) return false;
				if (!textures.Load(texpath, "widgets/tog_on_up.png", texinfo, downsel)) return false;
				if (!textures.Load(texpath, "widgets/tog_off_down.png", texinfo, trans)) return false;

				float h = fontsize * 0.75;
				float w = h * screenhwratio;
				float x = xy[0] - w;
				float y = xy[1];

				WIDGET_TOGGLE * new_widget = NewWidget<WIDGET_TOGGLE>();
				new_widget->SetupDrawable(sref, up, down, upsel, downsel, trans, x, y, w, h, z);
				new_widget->SetDescription(desc);
				new_widget->SetSetting(setting);
				new_widget->UpdateOptions(sref, false, optionmap, error_output);
			}
		}
		else if (type == "stringwheel" || type == "intwheel" || type == "floatwheel")
		{
			std::string setting;
			std::string values;
			std::string action;
			std::vector<float> xy(2);
			float spacing(0.3);
			float fontsize;

			if (!pagefile.GetParam(section, "setting", setting, error_output)) return false;
			if (!pagefile.GetParam(section, "values", values, error_output)) return false;
			if (!pagefile.GetParam(section, "center", xy, error_output)) return false;
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
			if (!textures.Load(texpath, "widgets/wheel_up_l.png", texinfo, up_left)) return false;
			if (!textures.Load(texpath, "widgets/wheel_down_l.png", texinfo, down_left)) return false;
			if (!textures.Load(texpath, "widgets/wheel_up_r.png", texinfo, up_right)) return false;
			if (!textures.Load(texpath, "widgets/wheel_down_r.png", texinfo, down_right)) return false;

			float fontscaley = fontsize;
			float fontscalex = fontsize * screenhwratio;
			xy[0] -= spacing * 0.5;

			WIDGET_STRINGWHEEL * new_widget = NewWidget<WIDGET_STRINGWHEEL>();
			new_widget->SetupDrawable(sref, text+":", up_left, down_left, up_right, down_right,
					font, fontscalex, fontscaley, xy[0], xy[1], z);
			new_widget->SetDescription(desc);
			new_widget->SetSetting(setting);
			new_widget->UpdateOptions(sref, false, optionmap, error_output);
			new_widget->SetAction(action);
		}
		else if (type == "intintwheel")
		{
			std::string setting1, setting2;
			std::string values;
			std::string hook;
			std::vector<float> xy(2);
			float spacing(0.3);
			float fontsize;

			if (!pagefile.GetParam(section, "setting1", setting1, error_output)) return false;
			if (!pagefile.GetParam(section, "setting2", setting2, error_output)) return false;
			if (!pagefile.GetParam(section, "values", values, error_output)) return false;
			if (!pagefile.GetParam(section, "center", xy, error_output)) return false;
			if (!pagefile.GetParam(section, "fontsize", fontsize, error_output)) return false;
			pagefile.GetParam(section, "spacing", spacing);
			pagefile.GetParam(section, "hook", hook);

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
			if (!textures.Load(texpath, "widgets/wheel_up_l.png", texinfo, up_left)) return false;
			if (!textures.Load(texpath, "widgets/wheel_down_l.png", texinfo, down_left)) return false;
			if (!textures.Load(texpath, "widgets/wheel_up_r.png", texinfo, up_right)) return false;
			if (!textures.Load(texpath, "widgets/wheel_down_r.png", texinfo, down_right)) return false;

			float fontscaley = fontsize;
			float fontscalex = fontsize * screenhwratio;
			xy[0] -= spacing * 0.5;

			WIDGET_DOUBLESTRINGWHEEL * new_widget = NewWidget<WIDGET_DOUBLESTRINGWHEEL>();
			new_widget->SetupDrawable(sref, text+":", up_left, down_left, up_right, down_right,
					font, fontscalex, fontscaley, xy[0], xy[1], z);
			new_widget->SetDescription(desc);
			new_widget->SetSetting(setting1, setting2);

			const std::list <std::pair<std::string,std::string> > & valuelist1 = optionmap[setting1].GetValueList();
			const std::list <std::pair<std::string,std::string> > & valuelist2 = optionmap[setting2].GetValueList();
			new_widget->SetValueList(valuelist1, valuelist2);
			new_widget->UpdateOptions(sref, false, optionmap, error_output);
		}
		else if (type == "multi-image")
		{
			std::vector<float> xy(2, 0.0);
			float width, height;
			std::string setting, values, prefix, postfix;

			if (!pagefile.GetParam(section, "center", xy, error_output)) return false;
			if (!pagefile.GetParam(section, "width", width, error_output)) return false;
			if (!pagefile.GetParam(section, "height", height, error_output)) return false;
			if (!pagefile.GetParam(section, "setting", setting, error_output)) return false;
			if (!pagefile.GetParam(section, "values", values, error_output)) return false;
			if (!pagefile.GetParam(section, "prefix", prefix, error_output)) return false;
			if (!pagefile.GetParam(section, "postfix", postfix, error_output)) return false;

			WIDGET_MULTIIMAGE * new_widget = NewWidget<WIDGET_MULTIIMAGE>();
			new_widget->SetupDrawable(sref, textures, texsize, prefix, postfix, xy[0], xy[1], width, height, error_output, z);
		}
		else if (type == "colorpicker")
		{
			std::vector<float> xy(2);
			float width, height;
			std::string setting, name;
			std::tr1::shared_ptr<TEXTURE> cursor, hue, sat, bg;

			if (!pagefile.GetParam(section, "center", xy, error_output)) return false;
			if (!pagefile.GetParam(section, "width", width, error_output)) return false;
			if (!pagefile.GetParam(section, "height", height, error_output)) return false;
			if (!pagefile.GetParam(section, "setting", setting, error_output)) return false;

			if (!textures.Load(texpath, "widgets/color_cursor.png", texinfo, cursor)) return false;
			if (!textures.Load(texpath, "widgets/color_hue.png", texinfo, hue)) return false;
			if (!textures.Load(texpath, "widgets/color_saturation.png", texinfo, sat)) return false;
			if (!textures.Load(texpath, "widgets/color_value.png", texinfo, bg)) return false;

			float x = xy[0] - width / 2;
			float y = xy[1] - height / 2;

			WIDGET_COLORPICKER * new_widget = NewWidget<WIDGET_COLORPICKER>();
			new_widget->SetupDrawable(sref, cursor, hue, sat, bg, x, y, width, height, setting, error_output, z);
		}
		else if (type == "slider")
		{
			std::vector<float> xy(2);
			float min(0), max(1);
			float fontsize;
			bool percentage(false);
			std::string name, setting, values;

			if (!pagefile.GetParam(section, "name", name, error_output)) return false;
			if (!pagefile.GetParam(section, "center", xy, error_output)) return false;
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
				WIDGET_LABEL * new_widget = NewWidget<WIDGET_LABEL>();
				new_widget->SetupDrawable(sref, font, text, x, y, fontscalex, fontscaley, r, g, b, z);
			}

			float h(0.0), w(0.0);
			pagefile.GetParam(section, "width", w);
			pagefile.GetParam(section, "height", h);
			if (h == 0.0) h = fontsize;
			if (w < h * screenhwratio) w = h * screenhwratio;
			xy[0] = xy[0] - w * 4.0 / 2 - fontscalex; // slider offset

			std::vector<float> color(3, 1.0);
			pagefile.GetParam(section, "color", color);

			std::tr1::shared_ptr<TEXTURE> cursor, wedge;
			if (!textures.Load(texpath, "widgets/sld_cursor.png", texinfo, cursor)) return false;
			if (!textures.Load(texpath, "widgets/sld_wedge.png", texinfo, wedge)) return false;

			WIDGET_SLIDER * new_widget = NewWidget<WIDGET_SLIDER>();
			new_widget->SetupDrawable(sref, wedge, cursor, xy[0], xy[1], w, h, min, max,
					percentage, setting, font, fontscalex, fontscaley, error_output, z);
			new_widget->SetName(name);
			new_widget->SetDescription(desc);
			new_widget->SetColor(sref, color[0], color[1], color[2]);
		}
		else if (type == "spinningcar")
		{
			std::vector<float> centerxy(3), carpos(3);
			std::string setting, values, name, prefix, postfix;

			if (!pagefile.GetParam(section, "center", centerxy, error_output)) return false;
			if (!pagefile.GetParam(section, "carpos", carpos, error_output)) return false;
			if (!pagefile.GetParam(section, "setting", setting, error_output)) return false;
			if (!pagefile.GetParam(section, "values", values, error_output)) return false;

			WIDGET_SPINNINGCAR * new_widget = NewWidget<WIDGET_SPINNINGCAR>();
			new_widget->SetupDrawable(sref, textures, models, texsize, datapath, centerxy[0], centerxy[1],
				MATHVECTOR<float, 3>(carpos[0], carpos[1], carpos[2]), error_output, z+10);
		}
		else if (type == "controlgrab")
		{
			std::vector<float> xy(2);
			float fontsize;
			std::string setting;
			bool analog(false);
			bool only_one(false);

			if (!pagefile.GetParam(section, "center", xy, error_output)) return false;
			if (!pagefile.GetParam(section, "fontsize", fontsize, error_output)) return false;
			if (!pagefile.GetParam(section, "setting", setting, error_output)) return false;
			pagefile.GetParam(section, "analog", analog);
			pagefile.GetParam(section, "only_one", only_one);

			std::vector <std::tr1::shared_ptr<TEXTURE> > control(WIDGET_CONTROLGRAB::END);
			if (!textures.Load(texpath, "widgets/controls/add.png", texinfo, control[WIDGET_CONTROLGRAB::ADD])) return false;
			if (!textures.Load(texpath, "widgets/controls/add_sel.png", texinfo, control[WIDGET_CONTROLGRAB::ADDSEL])) return false;
			if (!textures.Load(texpath, "widgets/controls/joy_axis.png", texinfo, control[WIDGET_CONTROLGRAB::JOYAXIS])) return false;
			if (!textures.Load(texpath, "widgets/controls/joy_axis_sel.png", texinfo, control[WIDGET_CONTROLGRAB::JOYAXISSEL])) return false;
			if (!textures.Load(texpath, "widgets/controls/joy_btn.png", texinfo, control[WIDGET_CONTROLGRAB::JOYBTN])) return false;
			if (!textures.Load(texpath, "widgets/controls/joy_btn_sel.png", texinfo, control[WIDGET_CONTROLGRAB::JOYBTNSEL])) return false;
			if (!textures.Load(texpath, "widgets/controls/key.png", texinfo, control[WIDGET_CONTROLGRAB::KEY])) return false;
			if (!textures.Load(texpath, "widgets/controls/key_sel.png", texinfo, control[WIDGET_CONTROLGRAB::KEYSEL])) return false;
			if (!textures.Load(texpath, "widgets/controls/mouse.png", texinfo, control[WIDGET_CONTROLGRAB::MOUSE])) return false;
			if (!textures.Load(texpath, "widgets/controls/mouse_sel.png", texinfo, control[WIDGET_CONTROLGRAB::MOUSESEL])) return false;

			float fontscaley = fontsize;
			float fontscalex = fontsize * screenhwratio;

			WIDGET_CONTROLGRAB * new_widget = NewWidget<WIDGET_CONTROLGRAB>();
			new_widget->SetupDrawable(sref, controlsconfig, setting, control, font,
					text, xy[0], xy[1], fontscalex, fontscaley, analog, only_one, z);
			controlgrabs.push_back(new_widget);
		}
		else if (type != "disabled")
		{
			error_output << path << ": unknown " << section->first << " type: " << type << ", ignoring" << std::endl;
		}

		// process hooks
		std::string widget_name;
		if (pagefile.GetParam(section, "name", widget_name))
		{
			widgets.back().Get()->SetName(widget_name);

			std::map <std::string, WIDGET *>::iterator widget = namemap.find(widget_name);
			if (widget != namemap.end())
			{
				error_output << widget_name << " defined twice." << std::endl;
				return false;
			}
			namemap[widget_name] = widgets.back().Get();
		}

		std::vector<std::string> hooks;
		pagefile.GetParam(section, "hook", hooks);
		for (std::vector<std::string>::iterator n = hooks.begin(); n != hooks.end(); ++n)
		{
			std::map <std::string, WIDGET *>::iterator hookee = namemap.find(*n);
			if (hookee != namemap.end())
			{
				widgets.back().Get()->AddHook(hookee->second); // last widget added
			}
			else
			{
				error_output << path << ": unknown hook reference to " << *n << std::endl;
			}
		}
	}

	return true;
}

void GUIPAGE::UpdateControls(SCENENODE & parentnode, const CONFIG & controls, const FONT & font)
{
	assert(s.valid());
	SCENENODE & sref = GetNode(parentnode);
	for (std::list <WIDGET_CONTROLGRAB *>::iterator i = controlgrabs.begin(); i != controlgrabs.end(); ++i)
	{
		(*i)->LoadControls(sref, controls, font);
	}
}

void GUIPAGE::SetVisible(SCENENODE & parent, const bool newvis)
{
	SCENENODE & sref = GetNode(parent);
	for (std::list <DERIVED <WIDGET> >::iterator i = widgets.begin(); i != widgets.end(); ++i)
	{
		(*i)->SetVisible(sref, newvis);
	}
}

void GUIPAGE::SetAlpha(SCENENODE & parent, const float newalpha)
{
	SCENENODE & sref = parent.GetNode(s);
	for (std::list <DERIVED <WIDGET> >::iterator i = widgets.begin(); i != widgets.end(); ++i)
	{
		(*i)->SetAlpha(sref, newalpha);
	}
}

///tell all child widgets to update to/from the option map
void GUIPAGE::UpdateOptions(SCENENODE & parent, bool save_to, std::map<std::string, GUIOPTION> & optionmap, std::ostream & error_output)
{
	SCENENODE & sref = parent.GetNode(s);
	for (std::list <DERIVED <WIDGET> >::iterator i = widgets.begin(); i != widgets.end(); ++i)
	{
		(*i)->UpdateOptions(sref, save_to, optionmap, error_output);
	}
}

std::list <std::pair <std::string, bool> > GUIPAGE::ProcessInput(
	SCENENODE & parent, bool movedown, bool moveup, float cursorx, float cursory,
	bool cursordown, bool cursorjustup, float screenhwratio)
{
	assert(tooltip_widget);

	SCENENODE & sref = parent.GetNode(s);

	std::list <std::pair <std::string, bool> > actions;
	std::string tooltip;

	for (std::list <DERIVED <WIDGET> >::iterator i = widgets.begin(); i != widgets.end(); ++i)
	{
		WIDGET * w = i->Get();

		bool mouseover = w->ProcessInput(sref, cursorx, cursory, cursordown, cursorjustup);
		if (mouseover)
		{
			tooltip = w->GetDescription();
		}

		std::string action = w->GetAction();
		bool cancel = w->GetCancel();
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

///tell all child widgets to do as update tick
void GUIPAGE::Update(SCENENODE & parent, float dt)
{
	SCENENODE & sref = parent.GetNode(s);
	for (std::list <DERIVED <WIDGET> >::iterator i = widgets.begin(); i != widgets.end(); ++i)
	{
		(*i)->Update(sref, dt);
	}
}

void GUIPAGE::Clear(SCENENODE & parentnode)
{
	controlgrabs.clear();
	tooltip_widget = 0;
	dialog = false;
	widgets.clear();
	if (s.valid())
	{
		SCENENODE & sref = parentnode.GetNode(s);
		sref.Clear();
	}
	s.invalidate();
}