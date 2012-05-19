#include "gui/guicontrolgrab.h"
#include "carcontrolmap_local.h"
#include "config.h"

#include <cassert>

std::string GUICONTROLGRAB::Str[] =
{
	"Add a new input", "Edit", "press", "release", "once", "held", "key", "joy", "mouse", "button", "axis", "motion"
};

GUICONTROLGRAB::CONTROLWIDGET::CONTROLWIDGET() :
	once(true),
	down(false),
	keycode(0),
	joy_index(0),
	joy_axis(0),
	deadzone(0),
	exponent(1),
	gain(1)
{
	// ctor
}

GUICONTROLGRAB::GUICONTROLGRAB() :
	scale_x(0.05),
	scale_y(0.05),
	x(0.5),
	y(0.5),
	z(0),
	w(0.5),
	h(0.5),
	analog(false),
	once(false)
{
	active_widget = &addbutton;
}

GUICONTROLGRAB::~GUICONTROLGRAB()
{
	//std::clog << "ControlGrab Destructor: " << setting << ", " << description << std::endl;
}

void GUICONTROLGRAB::SetAlpha(SCENENODE & scene, float newalpha)
{
	SCENENODE & topnoderef = scene.GetNode(topnode);
	addbutton.SetAlpha(topnoderef, newalpha);

	SCENENODE & ctrlnoderef = topnoderef.GetNode(ctrlnode);
	for (std::list <CONTROLWIDGET>::iterator i = controlbuttons.begin(); i != controlbuttons.end(); ++i)
	{
		i->widget.SetAlpha(ctrlnoderef, newalpha);
	}
}

void GUICONTROLGRAB::SetVisible(SCENENODE & scene, bool newvis)
{
	SCENENODE & topnoderef = scene.GetNode(topnode);
	addbutton.SetVisible(topnoderef, newvis);

	SCENENODE & ctrlnoderef = topnoderef.GetNode(ctrlnode);
	for (std::list <CONTROLWIDGET>::iterator i = controlbuttons.begin(); i != controlbuttons.end(); ++i)
	{
		i->widget.SetVisible(ctrlnoderef, newvis);
	}
}

bool GUICONTROLGRAB::ProcessInput(
	SCENENODE & scene,
	float cursorx, float cursory,
	bool cursordown, bool cursorjustup)
{
	bool infocus = (cursorx > x - w * 0.5 &&
		cursorx < x + w * 0.5 &&
		cursory > y - h * 0.5 &&
		cursory < y + h * 0.5);

	// generate the add input tooltip, check to see if we pressed the add input button, generate an action
	SCENENODE & topnoderef = scene.GetNode(topnode);
	if (addbutton.ProcessInput(
		topnoderef, cursorx, cursory,
		cursordown, cursorjustup))
	{
		if (active_widget != &addbutton)
		{
			SetDescription(Str[ADDNEW_STR]);
			active_widget = &addbutton;
		}
		if (cursorjustup)
		{
			signal_control(
				"add " +
				std::string(analog ? "y " : "n ") +
				std::string(once ? "y " : "n ") +
				setting);
		}
		return infocus;
	}

	// generate the input tooltip, check to see if we clicked, generate an action
	SCENENODE & ctrlnoderef = topnoderef.GetNode(ctrlnode);
	for (std::list <CONTROLWIDGET>::iterator i = controlbuttons.begin(); i != controlbuttons.end(); ++i)
	{
		if (i->widget.ProcessInput(
			ctrlnoderef, cursorx, cursory,
			cursordown, cursorjustup))
		{
			if (active_widget != &i->widget)
			{
				SetDescription(i->widget.GetDescription());
				active_widget = &i->widget;
			}
			if (cursorjustup)
			{
				signal_control("edit " +  GetControlString(*i) + setting);
			}
			break;
		}
	}

	return infocus;
}

void GUICONTROLGRAB::SetupDrawable(
	SCENENODE & scene,
	const std::string & newsetting,
	const CONFIG & c,
	const FONT & font,
	float scalex, float scaley,
	float centerx, float centery, float newz,
	bool newanalog, bool newonly_one)
{
	setting = newsetting;
	topnode = scene.AddNode();
	SCENENODE & topnoderef = scene.GetNode(topnode);
	ctrlnode = topnoderef.AddNode();

	x = centerx;
	y = centery;
	z = newz;
	h = scaley;
	w = 0.5;
	scale_x = scalex;
	scale_y = scaley;
	analog = newanalog;
	once = newonly_one;

	addbutton.SetupDrawable(
		topnoderef,
		font, 0, scalex * 1.5, scaley * 1.5,
		x, y, scalex * 2, scaley, z,
		m_r ,m_g, m_b);
	addbutton.SetText("+");

	LoadControls(scene, c, font);
}

void GUICONTROLGRAB::LoadControls(SCENENODE & scene, const CONFIG & c, const FONT & font)
{
	assert(!setting.empty()); //ensure that we've already done a SetupDrawable

	SCENENODE & parentnode = scene.GetNode(topnode).GetNode(ctrlnode);
	parentnode.Clear();
	controlbuttons.clear();

	for (CONFIG::const_iterator section = c.begin(); section != c.end(); ++section)
	{
		std::string controlname;
		c.GetParam(section, "name",  controlname);

		if (controlname == setting)
		{
			controlbuttons.push_back(CONTROLWIDGET());
			CONTROLWIDGET & button = controlbuttons.back();
			c.GetParam(section, "type", button.type);
			c.GetParam(section, "once", button.once);
			c.GetParam(section, "down", button.down);
			c.GetParam(section, "key", button.key);
			c.GetParam(section, "keycode", button.keycode);
			c.GetParam(section, "joy_type", button.joy_type);
			c.GetParam(section, "joy_index", button.joy_index);
			c.GetParam(section, "joy_button", button.keycode);
			c.GetParam(section, "joy_axis", button.joy_axis);
			c.GetParam(section, "joy_axis_type", button.joy_axis_type);
			c.GetParam(section, "mouse_type", button.mouse_type);
			c.GetParam(section, "mouse_motion", button.mouse_motion);
			c.GetParam(section, "mouse_button", button.keycode);
			c.GetParam(section, "deadzone", button.deadzone);
			c.GetParam(section, "exponent", button.exponent);
			c.GetParam(section, "gain", button.gain);

			std::string text = "none";
			if (button.type == "key")
			{
				text = button.key;
			}
			else if (button.type == "joy")
			{
				if (button.joy_type == "button")
				{
					std::stringstream s;
					s << button.keycode;
					text = "button " + s.str();
				}
				else if (button.joy_type == "axis")
				{
					std::stringstream s;
					s << button.joy_axis;
					text = "axis " + s.str();
				}
			}
			else if (button.type == "mouse")
			{
				if (button.mouse_type == "motion")
				{
					text = "mouse " + button.mouse_motion;
				}
				else
				{
					std::stringstream s;
					s << button.keycode;
					text = "mouse " + s.str();
				}
			}

			float bx = x + scale_x * 4 * controlbuttons.size();
			float by = y;
			button.widget.SetupDrawable(
				parentnode, font,
				0, scale_x, scale_y,
				bx, by, scale_x * 4, scale_y, z,
				m_r, m_g, m_b);
			button.widget.SetText(text);
			button.widget.SetDescription(GetDescription(button));
		}
	}
}

keyed_container <SCENENODE>::handle GUICONTROLGRAB::GetNode()
{
	return topnode;
}

std::string GUICONTROLGRAB::GetDescription(CONTROLWIDGET & widget)
{
	if (widget.type == "key")
	{
		std::stringstream desc;
		desc << Str[EDIT_STR] << " " << Str[KEY_STR];

		if (widget.key.empty())
		{
			desc << " #" << widget.keycode;
		}
		else
		{
			desc << " " << widget.key;
		}

		desc << " " << (widget.down ? Str[PRESS_STR] : Str[RELEASE_STR]) <<
				" (" << (widget.once ? Str[ONCE_STR] : Str[HELD_STR]) << ")";

		return desc.str();
	}

	if (widget.type == "joy")
	{
		std::stringstream desc;
		desc << Str[EDIT_STR] << " " << Str[JOY_STR] << " " << widget.joy_index << " ";

		if (widget.joy_type == "button")
		{
			desc << Str[BUTTON_STR] << " " << widget.keycode <<
				" " << (widget.down ? Str[PRESS_STR] : Str[RELEASE_STR]) <<
				" (" << (widget.once ? Str[ONCE_STR] : Str[HELD_STR]) << ")";
		}
		else if (widget.joy_type == "axis")
		{
			desc << Str[AXIS_STR] << " " << widget.joy_axis << " " <<
				"(" << (widget.joy_axis_type == "negative" ? "-" : "+") << ")";
		}

		return desc.str();
	}

	if (widget.type == "mouse")
	{
		std::stringstream desc;
		desc << Str[EDIT_STR] + " " << Str[MOUSE_STR] << " ";

		if (widget.mouse_type == "button")
		{
			desc << Str[BUTTON_STR] << " " << widget.keycode <<
				" " << (widget.down ? Str[PRESS_STR] : Str[RELEASE_STR]) <<
				" (" << (widget.once ? Str[ONCE_STR] : Str[HELD_STR]) << ")";
		}
		else if (widget.mouse_type == "motion")
		{
			desc << Str[MOTION_STR] << " " << widget.mouse_motion;
		}

		return desc.str();
	}

	return "";
}

std::string GUICONTROLGRAB::GetControlString(CONTROLWIDGET & widget)
{
	CARCONTROLMAP_LOCAL::CONTROL newctrl;
	newctrl.deadzone = widget.deadzone;
	newctrl.exponent = widget.exponent;
	newctrl.gain = widget.gain;
	newctrl.onetime = widget.once;
	newctrl.pushdown = widget.down;
	newctrl.keycode = widget.keycode;
	newctrl.type = CARCONTROLMAP_LOCAL::CONTROL::UNKNOWN;

	if (widget.type == "key")
	{
		newctrl.type = CARCONTROLMAP_LOCAL::CONTROL::KEY;
	}
	else if (widget.type == "joy")
	{
		newctrl.type = CARCONTROLMAP_LOCAL::CONTROL::JOY;
		newctrl.joynum = widget.joy_index;
		newctrl.joytype = CARCONTROLMAP_LOCAL::CONTROL::JOYBUTTON;

		if (widget.joy_type == "axis")
		{
			newctrl.joytype = CARCONTROLMAP_LOCAL::CONTROL::JOYAXIS;
			newctrl.joyaxistype = CARCONTROLMAP_LOCAL::CONTROL::POSITIVE;
			newctrl.joyaxis = widget.joy_axis;

			if (widget.joy_axis_type == "negative")
			{
				newctrl.joyaxistype = CARCONTROLMAP_LOCAL::CONTROL::NEGATIVE;
			}
			else if (widget.joy_axis_type == "both")
			{
				newctrl.joyaxistype = CARCONTROLMAP_LOCAL::CONTROL::BOTH;
			}
		}
		else if (widget.joy_type == "hat")
		{
			newctrl.joytype = CARCONTROLMAP_LOCAL::CONTROL::JOYHAT;
		}
	}
	else if (widget.type == "mouse")
	{
		newctrl.type = CARCONTROLMAP_LOCAL::CONTROL::MOUSE;
		newctrl.mousetype = CARCONTROLMAP_LOCAL::CONTROL::MOUSEBUTTON;

		if (widget.mouse_type == "motion")
		{
			newctrl.mousetype = CARCONTROLMAP_LOCAL::CONTROL::MOUSEMOTION;
			newctrl.mdir = CARCONTROLMAP_LOCAL::CONTROL::RIGHT;

			if (widget.mouse_motion == "up")
			{
				newctrl.mdir = CARCONTROLMAP_LOCAL::CONTROL::UP;
			}
			else if (widget.mouse_motion == "down")
			{
				newctrl.mdir = CARCONTROLMAP_LOCAL::CONTROL::DOWN;
			}
			else if (widget.mouse_motion == "left")
			{
				newctrl.mdir = CARCONTROLMAP_LOCAL::CONTROL::LEFT;
			}
		}
	}

	std::stringstream controlstring;
	newctrl.DebugPrint(controlstring);
	return controlstring.str();
}
