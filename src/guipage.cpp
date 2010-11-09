#include "guipage.h"
#include "configfile.h"
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
using std::list;

#include <string>
using std::string;

#include <map>
using std::map;

#include <fstream>
using std::ifstream;

#include <iostream>
using std::endl;

#include <sstream>
using std::stringstream;

bool GUIPAGE::Load(
	const std::string & path,
	const std::string & texpath,
	const std::string & datapath,
	const std::string & texsize,
	const float screenhwratio,
	const CONFIGFILE & controlsconfig,
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
	
	//error_output << "Loading " << path << endl;
	
	CONFIGFILE pagefile;
	if (!pagefile.Load(path))
	{
		error_output << "Couldn't find GUI page file: " << path << endl;
		return false;
	}
	
	//int widgetnum = 0;
	//if (!pagefile.GetParam("widgets", widgetnum)) return false;
	if (!pagefile.GetParam("dialog", dialog)) return false;
	
	string background;
	if (!pagefile.GetParam("background", background)) return false;
	
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
	map <string, WIDGET *> namemap;
	
	std::list <std::string> sectionlist;
	pagefile.GetSectionList(sectionlist);
	if (!sectionlist.empty() && sectionlist.front() == "")
	{
		sectionlist.pop_front();
	}
	for (std::list <std::string>::iterator i = sectionlist.begin(); i != sectionlist.end(); ++i)
	{
		stringstream widgetstr;
		widgetstr.str(*i);
		
		string wtype;
		if (!pagefile.GetParam(widgetstr.str()+".type", wtype)) return false;
		//std::cout << widgetstr.str() << ": " << wtype << endl;
		
		assert(s.valid());
		
		if (!reloadcontrolsonly)
		{
			if (wtype == "image")
			{
				float xy[3];
				float w, h;
				if (!pagefile.GetParam(widgetstr.str()+".center", xy)) return false;
				if (!pagefile.GetParam(widgetstr.str()+".width", w)) return false;
				if (!pagefile.GetParam(widgetstr.str()+".height", h)) return false;
				
				string texname;
				if (!pagefile.GetParam(widgetstr.str()+".filename", texname)) return false;
				std::tr1::shared_ptr<TEXTURE> texture;
				if (!textures.Load(texpath+"/"+texname, texinfo, texture)) return false;
				
				WIDGET_IMAGE * new_widget = NewWidget<WIDGET_IMAGE>();
				new_widget->SetupDrawable(sref, texture, xy[0], xy[1], w, h);
			}
			else if (wtype == "button")
			{
				float xy[3];
				string text;
				int fontsize;
				float color[3];
				string action;
				string description;
				bool cancel;
				if (!pagefile.GetParam(widgetstr.str()+".center", xy)) return false;
				if (!pagefile.GetParam(widgetstr.str()+".text", text)) return false;
				if (!pagefile.GetParam(widgetstr.str()+".fontsize", fontsize)) return false;
				if (!pagefile.GetParam(widgetstr.str()+".color", color)) return false;
				if (!pagefile.GetParam(widgetstr.str()+".action", action)) return false;
				if (!pagefile.GetParam(widgetstr.str()+".cancel", cancel)) return false;
				if (!pagefile.GetParam(widgetstr.str()+".tip", description)) description = "";
				
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
				float xy[3];
				string text;
				int fontsize;
				float color[3];
				if (!pagefile.GetParam(widgetstr.str()+".center", xy)) return false;
				if (!pagefile.GetParam(widgetstr.str()+".text", text))
				{
					//no text?
				}
				if (!pagefile.GetParam(widgetstr.str()+".fontsize", fontsize)) return false;
				if (!pagefile.GetParam(widgetstr.str()+".color", color))
				{
					color[0] = color[1] = color[2] = 1.0;
				}
				float fontscaley = ((((float) fontsize - 7.0f) * 0.25f) + 1.0f)*0.25;
				float fontscalex = fontscaley*screenhwratio;
				const FONT * font = &fonts.find("futuresans")->second;
				WIDGET_LABEL * new_widget = NewWidget<WIDGET_LABEL>();
				new_widget->SetupDrawable(sref, font, text, xy[0],xy[1], fontscalex,fontscaley, color[0],color[1],color[2], 2);
				
				string name;
				if (pagefile.GetParam(widgetstr.str()+".name", name)) label_widgets[name] = *new_widget;
			}
			else if (wtype == "toggle")
			{
				float xy[3];
				string setting;
				float spacing(0.3);
				int fontsize;
				string valuetype;
				if (!pagefile.GetParam(widgetstr.str()+".center", xy)) return false;
				if (!pagefile.GetParam(widgetstr.str()+".setting", setting)) return false;
				pagefile.GetParam(widgetstr.str()+".spacing", spacing);
				if (!pagefile.GetParam(widgetstr.str()+".fontsize", fontsize)) return false;
				if (!pagefile.GetParam(widgetstr.str()+".values", valuetype)) return false;
				
				string title = "<invalid>";
				string description = "<invalid>";
				
				if (valuetype == "options")
				{
					title = optionmap[setting].GetText();
					description = optionmap[setting].GetDescription();
				}
				else if (valuetype == "manual")
				{
					if (!pagefile.GetParam(widgetstr.str()+".tip", description)) return false;
					string truestr, falsestr;
					if (!pagefile.GetParam(widgetstr.str()+".true", truestr)) return false;
					if (!pagefile.GetParam(widgetstr.str()+".false", falsestr)) return false;
					title = truestr;
				}
				
				//if (!pagefile.GetParam(widgetstr.str()+".action", action)) return false;
				
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
				string setting;
				string values;
				string action;
				float xy[3];
				float spacing;
				int fontsize;
				
				if (!pagefile.GetParam(widgetstr.str()+".setting", setting)) return false;
				if (!pagefile.GetParam(widgetstr.str()+".values", values)) return false;
				if (!pagefile.GetParam(widgetstr.str()+".center", xy)) return false;
				if (!pagefile.GetParam(widgetstr.str()+".spacing", spacing)) spacing = 0.3;
				if (!pagefile.GetParam(widgetstr.str()+".fontsize", fontsize)) return false;
				pagefile.GetParam(widgetstr.str()+".action", action);
				
				xy[0] -= spacing*.5;
				
				string title = "<invalid>";
				string description = "<invalid>";
				
				if (values == "options")
				{
					title = optionmap[setting].GetText();
					description = optionmap[setting].GetDescription();
				}
				else
				{
					error_output << "Widget " << widgetstr << ": unknown value type " << values << endl;
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
				string setting1, setting2;
				string values;
				string hook;
				float xy[3];
				float spacing;
				int fontsize;
				
				if (!pagefile.GetParam(widgetstr.str()+".setting1", setting1)) return false;
				if (!pagefile.GetParam(widgetstr.str()+".setting2", setting2)) return false;
				if (!pagefile.GetParam(widgetstr.str()+".values", values)) return false;
				if (!pagefile.GetParam(widgetstr.str()+".hook", hook)) hook = "";
				if (!pagefile.GetParam(widgetstr.str()+".center", xy)) return false;
				if (!pagefile.GetParam(widgetstr.str()+".spacing", spacing)) spacing = 0.3;
				if (!pagefile.GetParam(widgetstr.str()+".fontsize", fontsize)) return false;
				
				xy[0] -= spacing*.5;
				
				string title = "<invalid>";
				string description = "<invalid>";
				
				if (values == "options")
				{
					title = optionmap[setting1].GetText();
					description = optionmap[setting1].GetDescription();
				}
				else
				{
					error_output << "Widget " << widgetstr << ": unknown value type " << values << endl;
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
				float xy[3];
				float width, height;
				string setting, values, name, prefix, postfix;
				
				if (!pagefile.GetParam(widgetstr.str()+".center", xy)) return false;
				if (!pagefile.GetParam(widgetstr.str()+".width", width)) return false;
				if (!pagefile.GetParam(widgetstr.str()+".height", height)) return false;
				if (!pagefile.GetParam(widgetstr.str()+".setting", setting)) return false;
				if (!pagefile.GetParam(widgetstr.str()+".values", values)) return false;
				if (!pagefile.GetParam(widgetstr.str()+".prefix", prefix)) return false;
				if (!pagefile.GetParam(widgetstr.str()+".postfix", postfix)) return false;
				
				WIDGET_MULTIIMAGE * new_widget = NewWidget<WIDGET_MULTIIMAGE>();
				new_widget->SetupDrawable(sref, textures, texsize, prefix, postfix, xy[0],xy[1], width, height, error_output, 102);
			}
			else if (wtype == "colorpicker")
			{
				float xy[3];
				float width, height;
				string setting, name;
				std::tr1::shared_ptr<TEXTURE> cursor, hue, satval, bg;
				
				if (!pagefile.GetParam(widgetstr.str()+".center", xy)) return false;
				if (!pagefile.GetParam(widgetstr.str()+".width", width)) return false;
				if (!pagefile.GetParam(widgetstr.str()+".height", height)) return false;
				if (!pagefile.GetParam(widgetstr.str()+".setting", setting)) return false;
				
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
				float xy[3];
				float min(0), max(1);
				bool percentage(false);
				string name, setting, values;
				int fontsize;
				
				if (!pagefile.GetParam(widgetstr.str()+".name", name)) return false;
				if (!pagefile.GetParam(widgetstr.str()+".center", xy)) return false;
				if (!pagefile.GetParam(widgetstr.str()+".setting", setting)) return false;
				if (!pagefile.GetParam(widgetstr.str()+".values", values)) return false;
				if (!pagefile.GetParam(widgetstr.str()+".fontsize", fontsize)) return false;
				
				string title = "<invalid>";
				string description = "<invalid>";
				
				if (values == "manual")
				{
					if (!pagefile.GetParam(widgetstr.str()+".tip", description)) return false;
					if (!pagefile.GetParam(widgetstr.str()+".text", title)) return false;
				}
				else
				{
					title = optionmap[setting].GetText();
					description = optionmap[setting].GetDescription();
				}
				
				if (values == "manual")
				{
					if (!pagefile.GetParam(widgetstr.str()+".min", min)) return false;
					if (!pagefile.GetParam(widgetstr.str()+".max", max)) return false;
				}
				else
				{
					if (optionmap.find(setting) == optionmap.end())
					{
						error_output << path << ": slider widget option " << setting << " not found, assuming default min/max/percentage values" << endl;
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
				pagefile.GetParam(widgetstr.str()+".width", w);
				pagefile.GetParam(widgetstr.str()+".height", h);
				if (h < 0.01) h = 0.05;
				if (w < h * screenhwratio) w = h * screenhwratio;
				
				float spacing(0.3);
				pagefile.GetParam(widgetstr.str()+".spacing", spacing);
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
				
				float color[] = {1, 1, 1};
				pagefile.GetParam(widgetstr.str()+".color", color);
				new_widget->SetColor(sref, color[0], color[1], color[2]);
			}
			else if (wtype == "spinningcar")
			{
				float centerxy[3], carposxy[3];
				string setting, values, name, prefix, postfix;
				
				if (!pagefile.GetParam(widgetstr.str()+".center", centerxy)) return false;
				if (!pagefile.GetParam(widgetstr.str()+".carpos", carposxy)) return false;
				if (!pagefile.GetParam(widgetstr.str()+".setting", setting)) return false;
				if (!pagefile.GetParam(widgetstr.str()+".values", values)) return false;
				
				WIDGET_SPINNINGCAR * new_widget = NewWidget<WIDGET_SPINNINGCAR>();
				new_widget->SetupDrawable(sref, textures, models, texsize, datapath, centerxy[0], centerxy[1], MATHVECTOR <float, 3> (carposxy[0], carposxy[1], carposxy[2]), error_output, 110);
			}
			else if (wtype != "controlgrab")
			{
				error_output << path << ": unknown " << widgetstr.str() << " type: " << wtype << ", ignoring" << endl;
			}
			
			// process hooks
			std::string widget_name;
			if (pagefile.GetParam(widgetstr.str()+".name", widget_name))
			{
				widgets.back().Get()->SetName(widget_name);
				
				map <string, WIDGET *>::iterator widget = namemap.find(widget_name);
				if (widget != namemap.end())
				{
					error_output << widget_name << " defined twice." << endl;
					return false;
				}
				namemap[widget_name] = widgets.back().Get();
			}
			
			std::vector<std::string> hooks;
			pagefile.GetParam(widgetstr.str()+".hook", hooks);
			for (std::vector<std::string>::iterator n = hooks.begin(); n != hooks.end(); ++n)
			{
				map <string, WIDGET *>::iterator hookee = namemap.find(*n);
				if (hookee != namemap.end())
					widgets.back().Get()->AddHook(hookee->second); // last widget added
				else
					error_output << path << ": unknown hook reference to " << *n << endl;
			}
		}
		
		if (wtype == "controlgrab")
		{
			float xy[3];
			string text;
			int fontsize;
			string setting;
			string description;
			bool analog, only_one;
			if (!pagefile.GetParam(widgetstr.str()+".center", xy)) return false;
			if (!pagefile.GetParam(widgetstr.str()+".text", text)) return false;
			if (!pagefile.GetParam(widgetstr.str()+".fontsize", fontsize)) return false;
			if (!pagefile.GetParam(widgetstr.str()+".tip", description)) description = "";
			if (!pagefile.GetParam(widgetstr.str()+".setting", setting)) return false;
			if (!pagefile.GetParam(widgetstr.str()+".analog", analog)) analog = false;
			if (!pagefile.GetParam(widgetstr.str()+".only_one", only_one)) only_one = false;
			
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

std::list <std::pair <std::string, bool> > GUIPAGE::ProcessInput(SCENENODE & parent, bool movedown, bool moveup, float cursorx, float cursory,
		bool cursordown, bool cursorjustup, float screenhwratio)
{
	assert(fontmap);
	assert(tooltip_widget);
	
	SCENENODE & sref = parent.GetNode(s);
	
	list <std::pair <std::string, bool> > actions;
	string tooltip;
	
	for (std::list <DERIVED <WIDGET> >::iterator i = widgets.begin(); i != widgets.end(); ++i)
	{
		WIDGET * w = i->Get();
		
		bool mouseover = w->ProcessInput(sref, cursorx, cursory, cursordown, cursorjustup);
		if (mouseover)
		{
			tooltip = w->GetDescription();
		}
		
		string action = w->GetAction();
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
