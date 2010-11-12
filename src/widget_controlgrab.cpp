#include "widget_controlgrab.h"
#include "carcontrolmap_local.h"
#include "config.h"

#include <cassert>

WIDGET * WIDGET_CONTROLGRAB::clone() const
{
	return new WIDGET_CONTROLGRAB(*this);
}

void WIDGET_CONTROLGRAB::SetAlpha(SCENENODE & scene, float newalpha)
{
	SCENENODE & topnoderef = scene.GetNode(topnode);
	label.SetAlpha(topnoderef, newalpha);
	addbutton.SetAlpha(topnoderef, newalpha);
	for (std::list <CONTROLWIDGET>::iterator i = controlbuttons.begin(); i != controlbuttons.end(); ++i)
	{
		i->widget.SetAlpha(topnoderef, newalpha);
	}
}

void WIDGET_CONTROLGRAB::SetVisible(SCENENODE & scene, bool newvis)
{
	SCENENODE & topnoderef = scene.GetNode(topnode);
	label.SetVisible(topnoderef, newvis);
	addbutton.SetVisible(topnoderef, newvis);
	for (std::list <CONTROLWIDGET>::iterator i = controlbuttons.begin(); i != controlbuttons.end(); ++i)
	{
		i->widget.SetVisible(topnoderef, newvis);
	}
}

std::string WIDGET_CONTROLGRAB::GetAction() const
{
	return active_action;
}

std::string WIDGET_CONTROLGRAB::GetDescription() const
{
	if (!tempdescription.empty())
		return tempdescription;
	else
		return description;
}

void WIDGET_CONTROLGRAB::SetDescription(const std::string & newdesc)
{
	description = newdesc;
}

bool WIDGET_CONTROLGRAB::ProcessInput(
	SCENENODE & scene,
	float cursorx, float cursory,
	bool cursordown, bool cursorjustup)
{
	active_action.clear();
	
	tempdescription.clear();
	
	SCENENODE & topnoderef = scene.GetNode(topnode);
	
	//generate the add input tooltip, check to see if we pressed the add input button, generate an action
	if (addbutton.ProcessInput(topnoderef, cursorx, cursory, cursordown, cursorjustup))
	{
		tempdescription = "Add a new input";
		
		if (cursorjustup)
			active_action = "controlgrabadd:"+std::string(analog?"y":"n")+":"+std::string(only_one?"y":"n")+":"+setting;
	}
	
	//generate the input tooltip, check to see if we clicked, generate an action
	for (std::list <CONTROLWIDGET>::iterator i = controlbuttons.begin(); i != controlbuttons.end(); ++i)
	{
		if (i->widget.ProcessInput(topnoderef, cursorx, cursory, cursordown, cursorjustup))
		{
			if (i->type == "key")
			{
				if (i->key.empty())
					tempdescription = "Edit "+i->type+" #"+i->keycode+" "+(i->down?"press":"release")+
						" ("+(i->once?"once":"held")+")";
				else
					tempdescription = "Edit "+i->type+" "+i->key+" "+(i->down?"press":"release")+
						" ("+(i->once?"once":"held")+")";
			}
			else if (i->type == "joy")
			{
				std::stringstream desc;
				if (i->joy_type == "button")
					desc << "Edit "<<i->type<<" "<<i->joy_index<<" "<<i->joy_type<<" "<<i->joy_button<<" "<<(i->down?"press":"release")<<
						" ("<<(i->once?"once":"held")<<")";
				else if (i->joy_type == "axis")
					desc << "Edit "<<i->type<<" "<<i->joy_index<<" "<<i->joy_type<<" "<<i->joy_axis<<" "<<
							"("<<(i->joy_axis_type=="negative"?"-":"+")<<")";
				tempdescription = desc.str();
			}
			else if (i->type == "mouse")
			{
				std::stringstream desc;
				if (i->mouse_type == "button")
					desc << "Edit "<<i->type<<" "<<i->mouse_type<<" "<<i->mouse_button<<" "<<(i->down?"press":"release")<<
							" ("<<(i->once?"once":"held")<<")";
				else if (i->mouse_type == "motion")
					desc << "Edit "<<i->type<<" "<<i->mouse_type<<" "<<i->mouse_motion;
				tempdescription = desc.str();
			}
			
			//generate an action.  code up an action string based on the DebugPrint string representation of a CONTROL
			if (cursorjustup)
			{
				CARCONTROLMAP_LOCAL::CONTROL newctrl;
				newctrl.deadzone = i->deadzone;
				newctrl.exponent = i->exponent;
				newctrl.gain = i->gain;
				
				if (i->type == "key")
				{
					newctrl.type = CARCONTROLMAP_LOCAL::CONTROL::KEY;
					std::stringstream keycodestr(i->keycode);
					keycodestr >> newctrl.keycode;
					newctrl.keypushdown = i->down;
				}
				else if (i->type == "joy")
				{
					newctrl.type = CARCONTROLMAP_LOCAL::CONTROL::JOY;
					newctrl.joynum = i->joy_index;
					
					if (i->joy_type == "axis")
					{
						newctrl.joytype = CARCONTROLMAP_LOCAL::CONTROL::JOYAXIS;
						newctrl.joyaxis = i->joy_axis;
						if (i->joy_axis_type == "negative")
							newctrl.joyaxistype = CARCONTROLMAP_LOCAL::CONTROL::NEGATIVE;
						else if (i->joy_axis_type == "both")
							newctrl.joyaxistype = CARCONTROLMAP_LOCAL::CONTROL::BOTH;
						else
							newctrl.joyaxistype = CARCONTROLMAP_LOCAL::CONTROL::POSITIVE;
					}
					else if (i->joy_type == "hat")
					{
						newctrl.joytype = CARCONTROLMAP_LOCAL::CONTROL::JOYHAT;
					}
					else
					{
						newctrl.joytype = CARCONTROLMAP_LOCAL::CONTROL::JOYBUTTON;
						newctrl.joybutton = i->joy_button;
						newctrl.joypushdown = i->down;
						
					}
				}
				else if (i->type == "mouse")
				{
					newctrl.type = CARCONTROLMAP_LOCAL::CONTROL::MOUSE;
					if (i->mouse_type == "motion")
					{
						newctrl.mousetype = CARCONTROLMAP_LOCAL::CONTROL::MOUSEMOTION;
						if (i->mouse_motion == "up")
							newctrl.mdir = CARCONTROLMAP_LOCAL::CONTROL::UP;
						else if (i->mouse_motion == "down")
							newctrl.mdir = CARCONTROLMAP_LOCAL::CONTROL::DOWN;
						else if (i->mouse_motion == "left")
							newctrl.mdir = CARCONTROLMAP_LOCAL::CONTROL::LEFT;
						else
							newctrl.mdir = CARCONTROLMAP_LOCAL::CONTROL::RIGHT;
					}
					else
					{
						newctrl.mousetype = CARCONTROLMAP_LOCAL::CONTROL::MOUSEBUTTON;
						newctrl.mbutton = i->mouse_button;
						newctrl.mouse_push_down = i->down;
					}
				}
				else
					newctrl.type = CARCONTROLMAP_LOCAL::CONTROL::UNKNOWN;
					
				newctrl.onetime = i->once;
				
				std::stringstream controlstring;
				newctrl.DebugPrint(controlstring);
				
				active_action = "controlgrabedit:"+controlstring.str()+setting;
			}
		}
	}
	
	if (cursorx > x-w*0.5 && cursorx < x+w*0.5 &&
		cursory > y-h*0.5 && cursory < y+h*0.5)
	{
		return true;
	}
	else
	{
		return false;
	}
}

void WIDGET_CONTROLGRAB::SetupDrawable(
	SCENENODE & scene,
	const CONFIG & c,
	const std::string & newsetting,
	const std::vector <std::tr1::shared_ptr<TEXTURE> > & texturevector,
	const FONT & font,
	const std::string & text,
	const float centerx,
	const float centery,
	const float scalex,
	const float scaley,
	bool newanalog,
	bool newonly_one)
{
	assert(texturevector.size() == END);
	assert(!newsetting.empty());
	for (int i = 0; i < END; i++)
		assert(texturevector[i]);
	
	topnode = scene.AddNode();
	SCENENODE & topnoderef = scene.GetNode(topnode);
	
	setting = newsetting;
	
	h = 0.06*scaley*4.0;
	
	w = 0.5;
	x = centerx;
	y = centery;
	
	scale_x = scalex;
	scale_y = scaley;
	
	analog = newanalog;
	only_one = newonly_one;
	
	float textw = label.GetWidth(font, text, scalex);
	float textx = x-w*0.5+textw*0.5;
	
	float r(1),g(1),b(1);
	
	//std::cout << scalex << "," << scaley << std::endl;
	
	label.SetupDrawable(topnoderef, font, text, textx, y, scalex, scaley, r, g, b, 2);
	addbutton.SetupDrawable(topnoderef, texturevector[ADD], texturevector[ADDSEL], texturevector[ADDSEL],
		font, "", x, y, scalex*0.8*(4.0/3.0), scaley*0.8, r,g,b);
	
	//add control buttons as necessary
	LoadControls(topnoderef, c, texturevector, font);
}

void WIDGET_CONTROLGRAB::LoadControls(
	SCENENODE & scene,
	const CONFIG & c,
	const std::vector <std::tr1::shared_ptr<TEXTURE> > & texturevector,
	const FONT & font)
{
	assert(!setting.empty()); //ensure that we've already done a SetupDrawable
	
	controlbuttons.clear();

	for (CONFIG::const_iterator section = c.begin(); section != c.end(); ++section)
	{
		std::string controlname;
		c.GetParam(section, "name",  controlname);
		
		if (controlname == setting)
		{
			std::string type;
			bool once(true);
			bool down(false);
			std::string key, keycode;
			std::string joy_type;
			int joy_index(0);
			int joy_button(0);
			int joy_axis(0);
			std::string joy_axis_type;
			std::string mouse_type;
			std::string mouse_motion;
			int mouse_button(0);
			float deadzone(0),exponent(1),gain(1);
			
			c.GetParam(section, "type", type);
			c.GetParam(section, "once", once);
			c.GetParam(section, "down", down);
			c.GetParam(section, "key", key);
			c.GetParam(section, "keycode", keycode);
			c.GetParam(section, "joy_type", joy_type);
			c.GetParam(section, "joy_index", joy_index);
			c.GetParam(section, "joy_button", joy_button);
			c.GetParam(section, "joy_axis", joy_axis);
			c.GetParam(section, "joy_axis_type", joy_axis_type);
			c.GetParam(section, "mouse_type", mouse_type);
			c.GetParam(section, "mouse_motion", mouse_motion);
			c.GetParam(section, "mouse_button", mouse_button);
			c.GetParam(section, "deadzone", deadzone);
			c.GetParam(section, "exponent", exponent);
			c.GetParam(section, "gain", gain);
			
			if (type == "key")
				AddButton(scene, texturevector[KEY], texturevector[KEYSEL], font, type, controlname, scale_x, scale_y, y,
					once, down, key, keycode, joy_type, joy_index, joy_button, joy_axis, joy_axis_type,
					mouse_type, mouse_motion, mouse_button, deadzone,exponent,gain);
			else if (type == "joy")
			{
				if (joy_type == "button")
					AddButton(scene, texturevector[JOYBTN], texturevector[JOYBTNSEL], font, type, controlname, scale_x, scale_y, y,
						once, down, key, keycode, joy_type, joy_index, joy_button, joy_axis, joy_axis_type,
						mouse_type, mouse_motion, mouse_button, deadzone,exponent,gain);
				else if (joy_type == "axis")
					AddButton(scene, texturevector[JOYAXIS], texturevector[JOYAXISSEL], font, type, controlname, scale_x, scale_y, y,
						once, down, key, keycode, joy_type, joy_index, joy_button, joy_axis, joy_axis_type,
						mouse_type, mouse_motion, mouse_button, deadzone,exponent,gain);
			}
			else if (type == "mouse")
				AddButton(scene, texturevector[MOUSE], texturevector[MOUSESEL], font, type, controlname, scale_x, scale_y, y,
					once, down, key, keycode, joy_type, joy_index, joy_button, joy_axis, joy_axis_type,
					mouse_type, mouse_motion, mouse_button, deadzone,exponent,gain);
			
		}
		//else std::cout << "Ignoring since " << controlname << " != " << setting << std::endl;
	}
}

void WIDGET_CONTROLGRAB::AddButton(
	SCENENODE & scene,
	std::tr1::shared_ptr<TEXTURE> tex_unsel,
	std::tr1::shared_ptr<TEXTURE> tex_sel,
	const FONT & font, 
	const std::string & type,
	const std::string & name,
	const float scalex,
	const float scaley,
	const float y,
	const bool once,
	const bool down,
	const std::string & key,
	const std::string & keycode,
	const std::string & joy_type,
	const int joy_index,
	const int joy_button,
	const int joy_axis,
	const std::string & joy_axis_type,
	const std::string & mouse_type,
	const std::string & mouse_motion,
	const int mouse_button,
	const float deadzone,
	const float exponent,
	const float gain)
{
	controlbuttons.push_back(CONTROLWIDGET());
	controlbuttons.back().type = type;
	controlbuttons.back().name = name;
	controlbuttons.back().once = once;
	controlbuttons.back().down = down;
	controlbuttons.back().key = key;
	controlbuttons.back().keycode = keycode;
	controlbuttons.back().joy_type = joy_type;
	controlbuttons.back().joy_index = joy_index;
	controlbuttons.back().joy_button = joy_button;
	controlbuttons.back().joy_axis = joy_axis;
	controlbuttons.back().joy_axis_type = joy_axis_type;
	controlbuttons.back().mouse_type = mouse_type;
	controlbuttons.back().mouse_motion = mouse_motion;
	controlbuttons.back().mouse_button = mouse_button;
	controlbuttons.back().deadzone = deadzone;
	controlbuttons.back().exponent = exponent;
	controlbuttons.back().gain = gain;
	
	//float x = 0.5+0.06*scaley*4.0*0.5*(0.2+0.8)*(scaley/scalex)*controlbuttons.size();
	//float x = 0.5+0.06*scaley*4.0*0.5*(0.2+0.8)*controlbuttons.size();
	//float x = 0.5+0.06*scaley*4.0*0.5*(0.2+0.8)*(scaley/scalex)*controlbuttons.size();
	float x = 0.5+1.25*2.0*0.06*scaley*4.0/((scaley/scalex)*3.0)*controlbuttons.size();
	
	float r(1),g(1),b(1);
	
	controlbuttons.back().widget.SetupDrawable(scene, tex_unsel, tex_sel, tex_sel,
		font, "", x, y, scalex*0.8*(4.0/3.0), scaley*0.8, r,g,b);
}