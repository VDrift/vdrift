#include "guipage.h"
#include "config.h"
#include "texturemanager.h"
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

#include <list>
#include <string>
#include <map>
#include <fstream>
#include <iostream>
#include <sstream>

GUIPAGE::GUIPAGE() : 
	tooltip_widget(0),
	fontmap(0),
	dialog(false)
{
	// ctor
}

bool GUIPAGE::Load(
	const std::string & path,
	const std::string & texpath,
	const std::string & datapath,
	const std::string & texsize,
	const float screenhwratio,
	const CONFIG & controlsconfig,
	const std::map <std::string, FONT> & fonts,
	std::map <std::string, GUIOPTION> & optionmap,
	SCENENODE & parentnode,
	TEXTUREMANAGER & textures,
	MODELMANAGER & models,
	std::ostream & error_output,
	bool reloadcontrolsonly)
{
	TEXTUREINFO texinfo;
	texinfo.mipmap = false;
	texinfo.repeatu = false;
	texinfo.repeatv = false;
	texinfo.size = texsize;
	
	if (reloadcontrolsonly)
		assert(s.valid());
	else
		assert(!s.valid());
	
	if (!reloadcontrolsonly)
	{
		Clear(parentnode);
	
		//std::cout << "GUI PAGE LOAD" << std::endl;
		
		s = parentnode.AddNode();
	
		fontmap = &fonts;
	}
	
	SCENENODE & sref = GetNode(parentnode);
	
	//error_output << "Loading " << path << std::endl;
	
	CONFIG pagefile;
	if (!pagefile.Load(path))
	{
		error_output << "Couldn't find GUI page file: " << path << std::endl;
		return false;
	}
	
	if (!pagefile.GetParam("", "dialog", dialog)) return false;
	
	std::string background;
	if (!pagefile.GetParam("", "background", background)) return false;
	
	if (!reloadcontrolsonly)
	{
		//generate background
		WIDGET_IMAGE * bg_widget = NewWidget<WIDGET_IMAGE>();
		std::tr1::shared_ptr<TEXTURE> texture;
		if (!textures.Load(texpath+"/"+background, texinfo, texture)) return false;
		bg_widget->SetupDrawable(sref, texture, 0.5, 0.5, 1.0, 1.0, -1);
	}
	
	//remove existing controlgrabs
	if (reloadcontrolsonly)
	{
		//first, unload all of the assets from the widgets we're about to delete
		for (std::list <WIDGET_CONTROLGRAB *>::iterator n = controlgrabs.begin(); n != controlgrabs.end(); n++)
		{
			sref.Delete((*n)->GetNode());
		}
		
		std::list <std::list <DERIVED <WIDGET> >::iterator> todel;
		
		for (std::list <DERIVED <WIDGET> >::iterator i = widgets.begin(); i != widgets.end(); ++i)
		{
			for (std::list <WIDGET_CONTROLGRAB *>::iterator n = controlgrabs.begin(); n != controlgrabs.end(); ++n)
			{
				if (i->Get() == *n)
				{
					todel.push_back(i);
				}
			}
		}
		
		for (std::list <std::list <DERIVED <WIDGET> >::iterator>::iterator i = todel.begin(); i != todel.end(); ++i)
		{
			widgets.erase(*i);
		}
		
		controlgrabs.clear();
	}
	
	// widget map for hooks
	std::map <std::string, WIDGET *> namemap;
	
	// load widgets
	for (CONFIG::const_iterator section = pagefile.begin(); section != pagefile.end(); ++section)
	{
		if (section->first.empty()) continue;
		
		std::string wtype;
		if (!pagefile.GetParam(section, "type", wtype, error_output)) return false;
		//std::cout << section->first << ": " << wtype << std::endl;
		
		if (!reloadcontrolsonly)
		{
			if (wtype == "image")
			{
				float w, h;
				std::vector<float> xy(2);
				if (!pagefile.GetParam(section, "center", xy, error_output)) return false;
				if (!pagefile.GetParam(section, "width", w, error_output)) return false;
				if (!pagefile.GetParam(section, "height", h, error_output)) return false;
				
				std::string texname;
				if (!pagefile.GetParam(section, "filename", texname, error_output)) return false;
				std::tr1::shared_ptr<TEXTURE> texture;
				if (!textures.Load(texpath+"/"+texname, texinfo, texture)) return false;
				
				WIDGET_IMAGE * new_widget = NewWidget<WIDGET_IMAGE>();
				new_widget->SetupDrawable(sref, texture, xy[0], xy[1], w, h);
			}
			else if (wtype == "button")
			{
				int fontsize;
				std::vector<float> xy(2);
				std::vector<float> color(3, 1.0);
				std::string text;
				std::string action;
				std::string description;
				bool cancel;
				if (!pagefile.GetParam(section, "center", xy, error_output)) return false;
				if (!pagefile.GetParam(section, "text", text, error_output)) return false;
				if (!pagefile.GetParam(section, "fontsize", fontsize, error_output)) return false;
				if (!pagefile.GetParam(section, "color", color, error_output)) return false;
				if (!pagefile.GetParam(section, "action", action, error_output)) return false;
				if (!pagefile.GetParam(section, "cancel", cancel, error_output)) return false;
				pagefile.GetParam(section, "tip", description);
				
				std::tr1::shared_ptr<TEXTURE> texture_up, texture_down, texture_sel;
				if (!textures.Load(texpath+"/widgets/btn_up_unsel.png", texinfo, texture_sel)) return false;
				if (!textures.Load(texpath+"/widgets/btn_down.png", texinfo, texture_down)) return false;
				if (!textures.Load(texpath+"/widgets/btn_up.png", texinfo, texture_up)) return false;
				float fontscaley = ((((float) fontsize - 7.0f) * 0.25f) + 1.0f)*0.25;
				float fontscalex = fontscaley*screenhwratio;
				
				const FONT * font = &fonts.find("futuresans")->second;
				WIDGET_BUTTON * new_widget = NewWidget<WIDGET_BUTTON>();
				new_widget->SetupDrawable(
						sref, texture_up, texture_down, texture_sel,
						font, text, xy[0], xy[1], fontscalex, fontscaley,
						color[0], color[1], color[2]);
				new_widget->SetAction(action);
				new_widget->SetDescription(description);
				new_widget->SetCancel(cancel);
			}
			else if (wtype == "label")
			{
				int fontsize;
				std::string text;
				std::vector<float> xy(2);
				std::vector<float> color(3, 1.0);
				if (!pagefile.GetParam(section, "center", xy, error_output)) return false;
				if (!pagefile.GetParam(section, "text", text))
				{
					//no text?
				}
				if (!pagefile.GetParam(section, "fontsize", fontsize, error_output)) return false;
				if (!pagefile.GetParam(section, "color", color))
				{
					color[0] = color[1] = color[2] = 1.0;
				}
				float fontscaley = ((((float) fontsize - 7.0f) * 0.25f) + 1.0f)*0.25;
				float fontscalex = fontscaley*screenhwratio;
				const FONT * font = &fonts.find("futuresans")->second;
				WIDGET_LABEL * new_widget = NewWidget<WIDGET_LABEL>();
				new_widget->SetupDrawable(sref, font, text, xy[0],xy[1], fontscalex,fontscaley, color[0],color[1],color[2], 2);
				
				std::string name;
				if (pagefile.GetParam(section, "name", name))
				{
					label_widgets[name] = *new_widget;
				}
			}
			else if (wtype == "toggle")
			{
				std::vector<float> xy(2);
				std::string setting;
				float spacing(0.3);
				int fontsize;
				std::string valuetype;
				if (!pagefile.GetParam(section, "center", xy, error_output)) return false;
				if (!pagefile.GetParam(section, "setting", setting, error_output)) return false;
				if (!pagefile.GetParam(section, "fontsize", fontsize, error_output)) return false;
				if (!pagefile.GetParam(section, "values", valuetype, error_output)) return false;
				pagefile.GetParam(section, "spacing", spacing);
				
				std::string title = "<invalid>";
				std::string description = "<invalid>";
				
				if (valuetype == "options")
				{
					title = optionmap[setting].GetText();
					description = optionmap[setting].GetDescription();
				}
				else if (valuetype == "manual")
				{
					std::string truestr, falsestr;
					if (!pagefile.GetParam(section, "tip", description, error_output)) return false;
					if (!pagefile.GetParam(section, "true", truestr, error_output)) return false;
					if (!pagefile.GetParam(section, "false", falsestr, error_output)) return false;
					title = truestr;
				}
				
				//if (!pagefile.GetParam(section, "action", action)) return false;
				
				xy[0] -= spacing*.5;
				
				//generate label
				{
					float fontscaley = ((((float) fontsize - 7.0f) * 0.25f) + 1.0f)*0.25;
					float fontscalex = fontscaley*screenhwratio;
					const FONT * font = &fonts.find("futuresans")->second;
					WIDGET_LABEL * new_widget = NewWidget<WIDGET_LABEL>();
					float fw = new_widget->GetWidth(font, title, fontscalex);
					new_widget->SetupDrawable(sref, font, title, xy[0]+fw*0.5,xy[1]+(0.02*0.25*4.0/3.0), fontscalex,fontscaley, 1,1,1, 2);
				}
				
				//generate toggle
				{
					std::tr1::shared_ptr<TEXTURE> up, down, upsel, downsel, trans;
					if (!textures.Load(texpath+"/widgets/tog_off_up_unsel.png", texinfo, up)) return false;
					if (!textures.Load(texpath+"/widgets/tog_on_up_unsel.png", texinfo, down)) return false;
					if (!textures.Load(texpath+"/widgets/tog_off_up.png", texinfo, upsel)) return false;
					if (!textures.Load(texpath+"/widgets/tog_on_up.png", texinfo, downsel)) return false;
					if (!textures.Load(texpath+"/widgets/tog_off_down.png", texinfo, trans)) return false;
					
					float h = 0.025;
					float w = h*screenhwratio;
					WIDGET_TOGGLE * new_widget = NewWidget<WIDGET_TOGGLE>();
					new_widget->SetupDrawable(sref, up, down,upsel, downsel, trans, xy[0]-0.02, xy[1], w, h);
					//new_widget->SetAction(action);
					new_widget->SetDescription(description);
					new_widget->SetSetting(setting);
					new_widget->UpdateOptions(sref, false, optionmap, error_output);
				}
			}
			else if (wtype == "stringwheel" || wtype == "intwheel" || wtype == "floatwheel")
			{
				std::string setting;
				std::string values;
				std::string action;
				std::vector<float> xy(2);
				float spacing(0.3);
				int fontsize;
				
				if (!pagefile.GetParam(section, "setting", setting, error_output)) return false;
				if (!pagefile.GetParam(section, "values", values, error_output)) return false;
				if (!pagefile.GetParam(section, "center", xy, error_output)) return false;
				if (!pagefile.GetParam(section, "fontsize", fontsize, error_output)) return false;
				pagefile.GetParam(section, "spacing", spacing);
				pagefile.GetParam(section, "action", action);
				
				xy[0] -= spacing*.5;
				
				std::string title = "<invalid>";
				std::string description = "<invalid>";
				
				if (values == "options")
				{
					title = optionmap[setting].GetText();
					description = optionmap[setting].GetDescription();
				}
				else
				{
					error_output << "Widget " << section->first << ": unknown value type " << values << std::endl;
				}
				
				std::tr1::shared_ptr<TEXTURE> up_left, down_left, up_right, down_right;
				if (!textures.Load(texpath+"/widgets/wheel_up_l.png", texinfo, up_left)) return false;
				if (!textures.Load(texpath+"/widgets/wheel_down_l.png", texinfo, down_left)) return false;
				if (!textures.Load(texpath+"/widgets/wheel_up_r.png", texinfo, up_right)) return false;
				if (!textures.Load(texpath+"/widgets/wheel_down_r.png", texinfo, down_right)) return false;
				
				float fontscaley = ((((float) fontsize - 7.0f) * 0.25f) + 1.0f) * 0.25;
				float fontscalex = fontscaley * screenhwratio;
				const FONT * font = &fonts.find("futuresans")->second;
				WIDGET_STRINGWHEEL * new_widget = NewWidget<WIDGET_STRINGWHEEL>();
				new_widget->SetupDrawable(
						sref, title+":", up_left, down_left, up_right, down_right,
						font, fontscalex, fontscaley, xy[0], xy[1]);
				new_widget->SetDescription(description);
				new_widget->SetSetting(setting);
				new_widget->UpdateOptions(sref, false, optionmap, error_output);
				new_widget->SetAction(action);
			}
			else if (wtype == "intintwheel")
			{
				std::string setting1, setting2;
				std::string values;
				std::string hook;
				std::vector<float> xy(2);
				float spacing(0.3);
				int fontsize;
				
				if (!pagefile.GetParam(section, "setting1", setting1, error_output)) return false;
				if (!pagefile.GetParam(section, "setting2", setting2, error_output)) return false;
				if (!pagefile.GetParam(section, "values", values, error_output)) return false;
				if (!pagefile.GetParam(section, "center", xy, error_output)) return false;
				if (!pagefile.GetParam(section, "fontsize", fontsize, error_output)) return false;
				pagefile.GetParam(section, "spacing", spacing);
				pagefile.GetParam(section, "hook", hook);
				
				xy[0] -= spacing*.5;
				
				std::string title = "<invalid>";
				std::string description = "<invalid>";
				
				if (values == "options")
				{
					title = optionmap[setting1].GetText();
					description = optionmap[setting1].GetDescription();
				}
				else
				{
					error_output << "Widget " << section->first << ": unknown value type " << values << std::endl;
				}
				
				std::tr1::shared_ptr<TEXTURE> up_left, down_left, up_right, down_right;
				if (!textures.Load(texpath+"/widgets/wheel_up_l.png", texinfo, up_left)) return false;
				if (!textures.Load(texpath+"/widgets/wheel_down_l.png", texinfo, down_left)) return false;
				if (!textures.Load(texpath+"/widgets/wheel_up_r.png", texinfo, up_right)) return false;
				if (!textures.Load(texpath+"/widgets/wheel_down_r.png", texinfo, down_right)) return false;
				
				float fontscaley = ((((float) fontsize - 7.0f) * 0.25f) + 1.0f) * 0.25;
				float fontscalex = fontscaley * screenhwratio;
				const FONT * font = &fonts.find("futuresans")->second;
				WIDGET_DOUBLESTRINGWHEEL * new_widget = NewWidget<WIDGET_DOUBLESTRINGWHEEL>();
				new_widget->SetupDrawable(
						sref, title+":", up_left, down_left, up_right, down_right,
						font, fontscalex, fontscaley, xy[0], xy[1]);
				new_widget->SetDescription(description);
				new_widget->SetSetting(setting1, setting2);
				
				const std::list <std::pair<std::string,std::string> > & valuelist1 = optionmap[setting1].GetValueList();
				const std::list <std::pair<std::string,std::string> > & valuelist2 = optionmap[setting2].GetValueList();
				new_widget->SetValueList(valuelist1, valuelist2);
				new_widget->UpdateOptions(sref, false, optionmap, error_output);
			}
			else if (wtype == "multi-image")
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
				new_widget->SetupDrawable(sref, textures, texsize, prefix, postfix, xy[0],xy[1], width, height, error_output, 102);
			}
			else if (wtype == "colorpicker")
			{
				std::vector<float> xy(2);
				float width, height;
				std::string setting, name;
				std::tr1::shared_ptr<TEXTURE> cursor, hue, satval, bg;
				
				if (!pagefile.GetParam(section, "center", xy, error_output)) return false;
				if (!pagefile.GetParam(section, "width", width, error_output)) return false;
				if (!pagefile.GetParam(section, "height", height, error_output)) return false;
				if (!pagefile.GetParam(section, "setting", setting, error_output)) return false;
				
				if (!textures.Load(texpath+"/widgets/color_cursor.png", texinfo, cursor)) return false;
				if (!textures.Load(texpath+"/widgets/color_hue.png", texinfo, hue)) return false;
				if (!textures.Load(texpath+"/widgets/color_saturation.png", texinfo, satval)) return false;
				if (!textures.Load(texpath+"/widgets/color_value.png", texinfo, bg)) return false;
				
				WIDGET_COLORPICKER * new_widget = NewWidget<WIDGET_COLORPICKER>();
				new_widget->SetupDrawable(sref, cursor, hue, satval, bg,
					xy[0]-width/2, xy[1]-height/2, width, height, setting, error_output, 102);
			}
			else if (wtype == "slider")
			{
				std::vector<float> xy(2);
				float min(0), max(1);
				bool percentage(false);
				std::string name, setting, values;
				int fontsize;
				
				if (!pagefile.GetParam(section, "name", name, error_output)) return false;
				if (!pagefile.GetParam(section, "center", xy, error_output)) return false;
				if (!pagefile.GetParam(section, "setting", setting, error_output)) return false;
				if (!pagefile.GetParam(section, "values", values, error_output)) return false;
				if (!pagefile.GetParam(section, "fontsize", fontsize, error_output)) return false;
				
				std::string title = "<invalid>";
				std::string description = "<invalid>";
				
				if (values == "manual")
				{
					if (!pagefile.GetParam(section, "tip", description, error_output)) return false;
					if (!pagefile.GetParam(section, "text", title, error_output)) return false;
				}
				else
				{
					title = optionmap[setting].GetText();
					description = optionmap[setting].GetDescription();
				}
				
				if (values == "manual")
				{
					if (!pagefile.GetParam(section, "min", min, error_output)) return false;
					if (!pagefile.GetParam(section, "max", max, error_output)) return false;
				}
				else
				{
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
				
				float h = 0;
				float w = 0;
				float spacing(0.3);
				pagefile.GetParam(section, "width", w);
				pagefile.GetParam(section, "height", h);
				pagefile.GetParam(section, "spacing", spacing);
				if (h < 0.01) h = 0.05;
				if (w < h * screenhwratio) w = h * screenhwratio;
				xy[0] -= spacing*.5;
				
				//generate label
				float fw = 0;
				{
					float fontscaley = ((((float) fontsize - 7.0f) * 0.25f) + 1.0f)*0.25;
					float fontscalex = fontscaley*screenhwratio;
					const FONT * font = &fonts.find("futuresans")->second;
					WIDGET_LABEL * new_widget = NewWidget<WIDGET_LABEL>();
					fw = new_widget->GetWidth(font, title, fontscalex);
					new_widget->SetupDrawable(sref, font, title, xy[0]+fw*0.5,xy[1]+(0.02*0.25*4.0/3.0), fontscalex,fontscaley, 1,1,1, 2);
				}
				
				//xy[0] += fw+w*4.0*0.5+0.01;
				xy[0] -= w*4.0*0.4;//w*4.0*0.5;
				
				//font settings for % display
				float fontscaley = ((((float) fontsize - 7.0f) * 0.25f) + 1.0f)*0.25;
				float fontscalex = fontscaley*screenhwratio;
				const FONT * font = &fonts.find("lcd")->second;
				
				std::tr1::shared_ptr<TEXTURE> cursor, wedge;
				if (!textures.Load(texpath+"/widgets/sld_cursor.png", texinfo, cursor)) return false;
				if (!textures.Load(texpath+"/widgets/sld_wedge.png", texinfo, wedge)) return false;
				
				WIDGET_SLIDER * new_widget = NewWidget<WIDGET_SLIDER>();
				new_widget->SetupDrawable(sref, wedge, cursor, xy[0], xy[1], w, h, min, max, percentage, setting,
     					font, fontscalex, fontscaley, error_output, 102);
				new_widget->SetName(name);
				new_widget->SetDescription(description);
				
				std::vector<float> color(3, 1.0);
				pagefile.GetParam(section, "color", color);
				new_widget->SetColor(sref, color[0], color[1], color[2]);
			}
			else if (wtype == "spinningcar")
			{
				std::vector<float> centerxy(3), carpos(3);
				std::string setting, values, name, prefix, postfix;
				
				if (!pagefile.GetParam(section, "center", centerxy, error_output)) return false;
				if (!pagefile.GetParam(section, "carpos", carpos, error_output)) return false;
				if (!pagefile.GetParam(section, "setting", setting, error_output)) return false;
				if (!pagefile.GetParam(section, "values", values, error_output)) return false;
				
				WIDGET_SPINNINGCAR * new_widget = NewWidget<WIDGET_SPINNINGCAR>();
				new_widget->SetupDrawable(sref, textures, models, texsize, datapath, centerxy[0], centerxy[1], MATHVECTOR<float, 3>(carpos[0], carpos[1], carpos[2]), error_output, 110);
			}
			else if (wtype != "controlgrab")
			{
				error_output << path << ": unknown " << section->first << " type: " << wtype << ", ignoring" << std::endl;
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
				//if (n->empty()) continue;
				
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
		
		if (wtype == "controlgrab")
		{
			std::vector<float> xy(2);
			int fontsize;
			std::string text;
			std::string setting;
			std::string description;
			bool analog(false);
			bool only_one(false);
			if (!pagefile.GetParam(section, "center", xy, error_output)) return false;
			if (!pagefile.GetParam(section, "text", text, error_output)) return false;
			if (!pagefile.GetParam(section, "fontsize", fontsize, error_output)) return false;
			if (!pagefile.GetParam(section, "setting", setting, error_output)) return false;
			pagefile.GetParam(section, "tip", description);
			pagefile.GetParam(section, "analog", analog);
			pagefile.GetParam(section, "only_one", only_one);
			
			std::vector <std::tr1::shared_ptr<TEXTURE> > control(WIDGET_CONTROLGRAB::END);
			if (!textures.Load(texpath+"/widgets/controls/add.png", texinfo, control[WIDGET_CONTROLGRAB::ADD])) return false;
			if (!textures.Load(texpath+"/widgets/controls/add_sel.png", texinfo, control[WIDGET_CONTROLGRAB::ADDSEL])) return false;
			if (!textures.Load(texpath+"/widgets/controls/joy_axis.png", texinfo, control[WIDGET_CONTROLGRAB::JOYAXIS])) return false;
			if (!textures.Load(texpath+"/widgets/controls/joy_axis_sel.png", texinfo, control[WIDGET_CONTROLGRAB::JOYAXISSEL])) return false;
			if (!textures.Load(texpath+"/widgets/controls/joy_btn.png", texinfo, control[WIDGET_CONTROLGRAB::JOYBTN])) return false;
			if (!textures.Load(texpath+"/widgets/controls/joy_btn_sel.png", texinfo, control[WIDGET_CONTROLGRAB::JOYBTNSEL])) return false;
			if (!textures.Load(texpath+"/widgets/controls/key.png", texinfo, control[WIDGET_CONTROLGRAB::KEY])) return false;
			if (!textures.Load(texpath+"/widgets/controls/key_sel.png", texinfo, control[WIDGET_CONTROLGRAB::KEYSEL])) return false;
			if (!textures.Load(texpath+"/widgets/controls/mouse.png", texinfo, control[WIDGET_CONTROLGRAB::MOUSE])) return false;
			if (!textures.Load(texpath+"/widgets/controls/mouse_sel.png", texinfo, control[WIDGET_CONTROLGRAB::MOUSESEL])) return false;
			
			float fontscaley = ((((float) fontsize - 7.0f) * 0.25f) + 1.0f)*0.25;
			float fontscalex = fontscaley*screenhwratio;
			const FONT * font = &fonts.find("lcd")->second;
			
			WIDGET_CONTROLGRAB * new_widget = NewWidget<WIDGET_CONTROLGRAB>();
			new_widget->SetupDrawable(
					sref, controlsconfig, setting, control, 
					font, text, xy[0],xy[1], fontscalex,fontscaley,
					analog, only_one);
			new_widget->SetDescription(description);
			controlgrabs.push_back(new_widget);
		}
	}
	
	// tooltip widget
	if (!reloadcontrolsonly)
	{
		tooltip_widget = NewWidget<WIDGET_LABEL>();
		assert(tooltip_widget);
		assert(fonts.find("futuresans") != fonts.end());
		const FONT * font = &fonts.find("futuresans")->second;
		tooltip_widget->SetupDrawable(sref, font, "", 0.5,0.95, 0.2*screenhwratio,0.2, 1,1,1, 1);
	}
	
	return true;
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
void GUIPAGE::UpdateOptions(SCENENODE & parent, bool save_to_options, std::map<std::string, GUIOPTION> & optionmap, std::ostream & error_output)
{
	SCENENODE & sref = parent.GetNode(s);
	for (std::list <DERIVED <WIDGET> >::iterator i = widgets.begin(); i != widgets.end(); ++i)
	{
		(*i)->UpdateOptions(sref, save_to_options, optionmap, error_output);
	}
}

std::list <std::pair <std::string, bool> > GUIPAGE::ProcessInput(
	SCENENODE & parent, bool movedown, bool moveup, float cursorx, float cursory,
	bool cursordown, bool cursorjustup, float screenhwratio)
{
	assert(fontmap);
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
		//assert(fontmap->find("futuresans") != fontmap->end());
		const FONT * font = &fontmap->find("futuresans")->second;
		tooltip_widget->ReviseDrawable(sref, font, tooltip, 0.5,0.95, 0.2*screenhwratio,0.2);
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
	fontmap = 0;
	dialog = false;
	widgets.clear();
	if (s.valid())
	{
		SCENENODE & sref = parentnode.GetNode(s);
		sref.Clear();
	}
	s.invalidate();
}