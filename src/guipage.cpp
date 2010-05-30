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
	CONFIGFILE & controlsconfig,
	SCENENODE & parentnode,
	std::map <std::string, FONT> & fonts,
	std::map<std::string, GUIOPTION> & optionmap,
  	float screenhwratio,
  	const std::string & texsize,
  	TEXTUREMANAGER & textures,
  	std::ostream & error_output,
  	bool reloadcontrolsonly)
{
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
		TEXTUREPTR texture_back = GetTexture(background, texpath, textures, texsize);
		if (!texture_back->Loaded()) return false;
		bg_widget->SetupDrawable(sref, texture_back, 0.5, 0.5, 1.0, 1.0, -1);
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
	
	//keep track of hooks for a second pass
	map <WIDGET *, list <string> > hookmap;
	map <string, WIDGET *> namemap;
	
	std::list <std::string> sectionlist;
	pagefile.GetSectionList(sectionlist);
	if (!sectionlist.empty())
		if (sectionlist.front() == "")
			sectionlist.pop_front();
	for (std::list <std::string>::iterator i = sectionlist.begin(); i != sectionlist.end(); ++i)
	{
		stringstream widgetstr;
		widgetstr.str(*i);
		/*widgetstr << "widget-";
		widgetstr.width(2);
		widgetstr.fill('0');
		widgetstr << i;*/
		
		//std::cout << *i << ": " << widgetstr.str()+".type" << std::endl;
		
		string wtype;
		if (!pagefile.GetParam(widgetstr.str()+".type", wtype)) return false;
		
		//process hooks
		list <string> hooklist;
		{
			string hookstr;
			if (!pagefile.GetParam(widgetstr.str()+".hook", hookstr)) hookstr = "";
			stringstream hookstream(hookstr);
			while (hookstream)
			{
				string hook;
				const int bufsize(256);
				char hookbuf[bufsize];
				hookstream.getline(hookbuf, bufsize, ',');
				hook = hookbuf;
				if (!hook.empty())
					hooklist.push_back(hook);
			}
		}
		
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
				
				string texfn;
				if (!pagefile.GetParam(widgetstr.str()+".filename", texfn)) return false;
				TEXTUREPTR texture = GetTexture(texfn, texpath, textures, texsize);
				if(!texture->Loaded()) return false;
				
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
				
				TEXTUREPTR texture_up = GetTexture("widgets/btn_up_unsel.png", texpath, textures, texsize);
				TEXTUREPTR texture_down = GetTexture("widgets/btn_down.png", texpath, textures, texsize);
				TEXTUREPTR texture_sel = GetTexture("widgets/btn_up.png", texpath, textures, texsize);
				if (!texture_up->Loaded() || !texture_down->Loaded() || !texture_sel->Loaded()) return false;
				float fontscaley = ((((float) fontsize - 7.0f) * 0.25f) + 1.0f)*0.25;
				float fontscalex = fontscaley*screenhwratio;
				
				WIDGET_BUTTON * new_widget = NewWidget<WIDGET_BUTTON>();
				new_widget->SetupDrawable(sref, texture_up, texture_down, texture_sel,
						&fonts["futuresans"], text, xy[0],xy[1], fontscalex,fontscaley, color[0],color[1],color[2]);
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
				
				WIDGET_LABEL * new_widget = NewWidget<WIDGET_LABEL>();
				float fontscaley = ((((float) fontsize - 7.0f) * 0.25f) + 1.0f)*0.25;
				float fontscalex = fontscaley*screenhwratio;
				new_widget->SetupDrawable(sref, &fonts["futuresans"], text, xy[0],xy[1], fontscalex,fontscaley, color[0],color[1],color[2], 2);
				
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
					WIDGET_LABEL * new_widget = NewWidget<WIDGET_LABEL>();
					float fontscaley = ((((float) fontsize - 7.0f) * 0.25f) + 1.0f)*0.25;
					float fontscalex = fontscaley*screenhwratio;
					float fw = new_widget->GetWidth(&fonts["futuresans"], title, fontscalex);
					new_widget->SetupDrawable(sref, &fonts["futuresans"], title, xy[0]+fw*0.5,xy[1]+(0.02*0.25*4.0/3.0), fontscalex,fontscaley, 1,1,1, 2);
				}
				
				//generate toggle
				{
					TEXTUREPTR texture_up = GetTexture("widgets/tog_off_up_unsel.png", texpath, textures, texsize);
					TEXTUREPTR texture_down = GetTexture("widgets/tog_on_up_unsel.png", texpath, textures, texsize);
					TEXTUREPTR texture_upsel = GetTexture("widgets/tog_off_up.png", texpath, textures, texsize);
					TEXTUREPTR texture_downsel = GetTexture("widgets/tog_on_up.png", texpath, textures, texsize);
					TEXTUREPTR texture_trans = GetTexture("widgets/tog_off_down.png", texpath, textures, texsize);
					if (!texture_up->Loaded() || !texture_down->Loaded() || !texture_upsel->Loaded() || 
						!texture_downsel->Loaded() || !texture_trans->Loaded()) return false;
					float h = 0.025;
					float w = h*screenhwratio;
					
					WIDGET_TOGGLE * new_widget = NewWidget<WIDGET_TOGGLE>();
					new_widget->SetupDrawable(sref, texture_up, texture_down, texture_upsel, 
							texture_downsel, texture_trans, xy[0]-0.02,xy[1], w, h);
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
				
				TEXTUREPTR up_left = GetTexture("widgets/wheel_up_l.png", texpath, textures, texsize);
				TEXTUREPTR down_left = GetTexture("widgets/wheel_down_l.png", texpath, textures, texsize);
				TEXTUREPTR up_right = GetTexture("widgets/wheel_up_r.png", texpath, textures, texsize);
				TEXTUREPTR down_right = GetTexture("widgets/wheel_down_r.png", texpath, textures, texsize);
				if (!up_left->Loaded() || !down_left->Loaded() || !up_right->Loaded() || !down_right->Loaded()) return false;
				//float w = 0.02;
				//float h = w*(4.0/3.0);
				float fontscaley = ((((float) fontsize - 7.0f) * 0.25f) + 1.0f)*0.25;
				float fontscalex = fontscaley*screenhwratio;
				
				WIDGET_STRINGWHEEL * new_widget = NewWidget<WIDGET_STRINGWHEEL>();
				new_widget->SetupDrawable(sref, title+":", up_left, down_left, up_right, down_right,
						&fonts["futuresans"], fontscalex, fontscaley, xy[0], xy[1]);
				new_widget->SetDescription(description);
				new_widget->SetSetting(setting);
				new_widget->UpdateOptions(sref, false, optionmap, error_output);
				new_widget->SetAction(action);
				
				std::string name;
				if (pagefile.GetParam(widgetstr.str()+".name", name))
				{
					new_widget->SetName(name);
					namemap[name] = new_widget;
				}
				
				for (list <string>::iterator n = hooklist.begin(); n != hooklist.end(); n++)
				{
					hookmap[new_widget].push_back(*n);
				}
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
				
				
				TEXTUREPTR up_left = GetTexture("widgets/wheel_up_l.png", texpath, textures, texsize);
				TEXTUREPTR down_left = GetTexture("widgets/wheel_down_l.png", texpath, textures, texsize);
				TEXTUREPTR up_right = GetTexture("widgets/wheel_up_r.png", texpath, textures, texsize);
				TEXTUREPTR down_right = GetTexture("widgets/wheel_down_r.png", texpath, textures, texsize);
				if (!up_left->Loaded() || !down_left->Loaded() || !up_right->Loaded() || !down_right->Loaded()) return false;
				//float w = 0.02;
				//float h = w*(4.0/3.0);
				float fontscaley = ((((float) fontsize - 7.0f) * 0.25f) + 1.0f)*0.25;
				float fontscalex = fontscaley*screenhwratio;
				
				WIDGET_DOUBLESTRINGWHEEL * new_widget = NewWidget<WIDGET_DOUBLESTRINGWHEEL>();
				new_widget->SetupDrawable(sref, title+":", up_left, down_left, up_right, down_right,
						&fonts["futuresans"], fontscalex, fontscaley, xy[0], xy[1]);
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
				new_widget->SetupDrawable(sref, texsize, textures, datapath, prefix, postfix, xy[0],xy[1], width, height, error_output, 102);
				
				if (pagefile.GetParam(widgetstr.str()+".name", name)) namemap[name] = new_widget;
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
					WIDGET_LABEL * new_widget = NewWidget<WIDGET_LABEL>();
					float fontscaley = ((((float) fontsize - 7.0f) * 0.25f) + 1.0f)*0.25;
					float fontscalex = fontscaley*screenhwratio;
					fw = new_widget->GetWidth(&fonts["futuresans"], title, fontscalex);
					new_widget->SetupDrawable(sref, &fonts["futuresans"], title, xy[0]+fw*0.5,xy[1]+(0.02*0.25*4.0/3.0), fontscalex,fontscaley, 1,1,1, 2);
				}
				
				//xy[0] += fw+w*4.0*0.5+0.01;
				xy[0] -= w*4.0*0.4;//w*4.0*0.5;
				
				//font settings for % display
				float fontscaley = ((((float) fontsize - 7.0f) * 0.25f) + 1.0f)*0.25;
				float fontscalex = fontscaley*screenhwratio;
				FONT * font = &fonts["lcd"];

				TEXTUREPTR cursor = GetTexture("widgets/sld_cursor.png", texpath, textures, texsize);
				TEXTUREPTR wedge = GetTexture("widgets/sld_wedge.png", texpath, textures, texsize);
				if (!cursor->Loaded() || !wedge->Loaded()) return false;
				
				WIDGET_SLIDER * new_widget = NewWidget<WIDGET_SLIDER>();
				new_widget->SetupDrawable(sref, wedge, cursor, xy[0], xy[1], w, h, min, max, percentage, setting,
     					font, fontscalex, fontscaley, error_output, 102);
				new_widget->SetName(name);
				new_widget->SetDescription(description);
				
				float color[] = {1, 1, 1};
				pagefile.GetParam(widgetstr.str()+".color", color);
				new_widget->SetColor(sref, color[0], color[1], color[2]);
				
				for (list <string>::iterator n = hooklist.begin(); n != hooklist.end(); n++)
				{
					hookmap[new_widget].push_back(*n);
				}
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
				new_widget->SetupDrawable(sref, texsize, datapath, centerxy[0], centerxy[1], MATHVECTOR <float, 3> (carposxy[0], carposxy[1], carposxy[2]), textures, error_output, 110);
				
				if (pagefile.GetParam(widgetstr.str()+".name", name)) namemap[name] = new_widget;
			}
			else if (wtype != "controlgrab")
			{
				error_output << path << ": unknown " << widgetstr.str() << " type: " << wtype << ", ignoring" << endl;
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
			
			WIDGET_CONTROLGRAB * new_widget = NewWidget<WIDGET_CONTROLGRAB>();
			controlgrabs.push_back(new_widget);
			
			std::vector <TEXTUREPTR> control(WIDGET_CONTROLGRAB::END);
			control[WIDGET_CONTROLGRAB::ADD] = GetTexture("widgets/controls/add.png", texpath, textures, texsize);
			control[WIDGET_CONTROLGRAB::ADDSEL] = GetTexture("widgets/controls/add_sel.png", texpath, textures, texsize);
			control[WIDGET_CONTROLGRAB::JOYAXIS] = GetTexture("widgets/controls/joy_axis.png", texpath, textures, texsize);
			control[WIDGET_CONTROLGRAB::JOYAXISSEL] = GetTexture("widgets/controls/joy_axis_sel.png", texpath, textures, texsize);
			control[WIDGET_CONTROLGRAB::JOYBTN] = GetTexture("widgets/controls/joy_btn.png", texpath, textures, texsize);
			control[WIDGET_CONTROLGRAB::JOYBTNSEL] = GetTexture("widgets/controls/joy_btn_sel.png", texpath, textures, texsize);
			control[WIDGET_CONTROLGRAB::KEY] = GetTexture("widgets/controls/key.png", texpath, textures, texsize);
			control[WIDGET_CONTROLGRAB::KEYSEL] = GetTexture("widgets/controls/key_sel.png", texpath, textures, texsize);
			control[WIDGET_CONTROLGRAB::MOUSE] = GetTexture("widgets/controls/mouse.png", texpath, textures, texsize);
			control[WIDGET_CONTROLGRAB::MOUSESEL] = GetTexture("widgets/controls/mouse_sel.png", texpath, textures, texsize);
			std::vector <TEXTUREPTR>::iterator it;
			for (it = control.begin(); it < control.end(); it++)
			{
				if (!it->get()->Loaded()) return false;
			}
			
			float fontscaley = ((((float) fontsize - 7.0f) * 0.25f) + 1.0f)*0.25;
			float fontscalex = fontscaley*screenhwratio;
			new_widget->SetupDrawable(sref, controlsconfig, setting, control, 
					&fonts["futuresans"], text, xy[0],xy[1], fontscalex,fontscaley,
					analog, only_one);
			new_widget->SetDescription(description);
		}
	}
	
	if (!reloadcontrolsonly)
	{
		//do a second pass to assign hooks
		for (map <WIDGET *, list <string> >::iterator i = hookmap.begin(); i != hookmap.end(); ++i)
		{
			for (list <string>::iterator n = i->second.begin(); n != i->second.end(); ++n)
			{
				map <string, WIDGET *>::iterator hookee = namemap.find(*n);
				if (hookee != namemap.end())
					i->first->AddHook(hookee->second);
				else
					error_output << path << ": unknown hook reference to " << *n << endl;
			}
		}
		
		tooltip_widget = NewWidget<WIDGET_LABEL>();
		assert(tooltip_widget);
		assert(fonts.find("futuresans") != fonts.end());
		tooltip_widget->SetupDrawable(sref, &fonts["futuresans"], "", 0.5,0.95, 0.2*screenhwratio,0.2, 1,1,1, 1);
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
		bool mouseover = (*i)->ProcessInput(sref, cursorx, cursory, cursordown, cursorjustup);
		if (mouseover)
			tooltip = (*i)->GetDescription();
		string action = (*i)->GetAction();
		if (!action.empty())
			actions.push_back(std::pair <std::string, bool> (action, !(*i)->GetCancel()));
	}
	
	if (tooltip != tooltip_widget->GetText())
	{
		tooltip_widget->ReviseDrawable(sref, &(*fontmap)["futuresans"], tooltip, 0.5,0.95, 0.2*screenhwratio,0.2);
	}
	
	return actions;
}
