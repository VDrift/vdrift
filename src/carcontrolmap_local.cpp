#include "carcontrolmap_local.h"
#include "config.h"

#include <string>
#include <list>
#include <iomanip>
#include <algorithm>

/// ramps the start value to the end value using rate button_ramp.
/// if button_ramp is zero, infinite rate is assumed.
static float Ramp(float start, float end, float button_ramp, float dt)
{
	//early exits
	if (start == end) //no ramp
		return end;
	if (dt <= 0) //no time increment
		return start;
	if (button_ramp == 0) //assume infinite rate
		return end;

	float cur = start;
	float sign = 0.3;
	if (end < start)
		sign = -1.2;
	if (button_ramp > 0)
		cur += button_ramp*dt*sign;

	//std::cout << "start: " << start << ", end: " << end << ", cur: " << cur << ", increment: "  << button_ramp*dt*sign << std::endl;

	if (cur < 0)
		return 0;
	if (cur > 1.0)
		return 1.0;
	return cur;
}

static float ApplyDeadzone(float dz, float val)
{
	if (std::abs(val) < dz)
		val = 0;
	else
	{
		if (val < 0)
			val = (val + dz)*(1.0/(1.0-dz));
		else
			val = (val - dz)*(1.0/(1.0-dz));
	}

	return val;
}

static float ApplyGain(float gain, float val)
{
	val *= gain;
	if (val < -1.0)
		val = -1.0;
	if (val > 1.0)
		val = 1.0;

	return val;
}

static float ApplyExponent(float exponent, float val)
{
	val = pow(val, exponent);
	if (val < -1.0)
		val = -1.0;
	if (val > 1.0)
		val = 1.0;

	return val;
}

CARCONTROLMAP_LOCAL::CARCONTROLMAP_LOCAL()
{
	carinput_stringmap["gas"] = CARINPUT::THROTTLE;
	carinput_stringmap["nos"] = CARINPUT::NOS;
	carinput_stringmap["brake"] = CARINPUT::BRAKE;
	carinput_stringmap["handbrake"] = CARINPUT::HANDBRAKE;
	carinput_stringmap["clutch"] = CARINPUT::CLUTCH;
	carinput_stringmap["steer_left"] = CARINPUT::STEER_LEFT;
	carinput_stringmap["steer_right"] = CARINPUT::STEER_RIGHT;
	carinput_stringmap["disengage_shift_up"] = CARINPUT::SHIFT_UP;
	carinput_stringmap["disengage_shift_down"] = CARINPUT::SHIFT_DOWN;
	carinput_stringmap["start_engine"] = CARINPUT::START_ENGINE;
	carinput_stringmap["abs_toggle"] = CARINPUT::ABS_TOGGLE;
	carinput_stringmap["tcs_toggle"] = CARINPUT::TCS_TOGGLE;
	carinput_stringmap["neutral"] = CARINPUT::NEUTRAL;
	carinput_stringmap["first_gear"] = CARINPUT::FIRST_GEAR;
	carinput_stringmap["second_gear"] = CARINPUT::SECOND_GEAR;
	carinput_stringmap["third_gear"] = CARINPUT::THIRD_GEAR;
	carinput_stringmap["fourth_gear"] = CARINPUT::FOURTH_GEAR;
	carinput_stringmap["fifth_gear"] = CARINPUT::FIFTH_GEAR;
	carinput_stringmap["sixth_gear"] = CARINPUT::SIXTH_GEAR;
	carinput_stringmap["reverse"] = CARINPUT::REVERSE;
	carinput_stringmap["rear_view"] = CARINPUT::REAR_VIEW;
	carinput_stringmap["rollover_recover"] = CARINPUT::ROLLOVER_RECOVER;

	carinput_stringmap["view_prev"] = CARINPUT::VIEW_PREV;
	carinput_stringmap["view_next"] = CARINPUT::VIEW_NEXT;
	carinput_stringmap["view_hood"] = CARINPUT::VIEW_HOOD;
	carinput_stringmap["view_incar"] = CARINPUT::VIEW_INCAR;
	carinput_stringmap["view_chaserigid"] = CARINPUT::VIEW_CHASERIGID;
	carinput_stringmap["view_chase"] = CARINPUT::VIEW_CHASE;
	carinput_stringmap["view_orbit"] = CARINPUT::VIEW_ORBIT;
	carinput_stringmap["view_free"] = CARINPUT::VIEW_FREE;
	carinput_stringmap["pan_left"] = CARINPUT::PAN_LEFT;
	carinput_stringmap["pan_right"] = CARINPUT::PAN_RIGHT;
	carinput_stringmap["pan_up"] = CARINPUT::PAN_UP;
	carinput_stringmap["pan_down"] = CARINPUT::PAN_DOWN;
	carinput_stringmap["zoom_in"] = CARINPUT::ZOOM_IN;
	carinput_stringmap["zoom_out"] = CARINPUT::ZOOM_OUT;
	carinput_stringmap["screen_shot"] = CARINPUT::SCREENSHOT;
	carinput_stringmap["pause"] = CARINPUT::PAUSE;
	carinput_stringmap["reload_shaders"] = CARINPUT::RELOAD_SHADERS;
    carinput_stringmap["reload_gui"] = CARINPUT::RELOAD_GUI;

	PopulateLegacyKeycodes();
}

bool CARCONTROLMAP_LOCAL::Load(const std::string & controlfile, std::ostream & info_output, std::ostream & error_output)
{
	CONFIG controls_config;
	if (!controls_config.Load(controlfile))
	{
		info_output << "Failed to load car controls file " << controlfile << std::endl;
		return false;
	}

	controls.clear();
	for (CONFIG::const_iterator i = controls_config.begin(); i != controls_config.end(); ++i)
	{
		std::string type, name;
		if (!controls_config.GetParam(i, "type", type)) continue;
		if (!controls_config.GetParam(i, "name", name)) continue;

		CARINPUT::CARINPUT carinput = GetInputFromString(name);
		if (carinput == CARINPUT::INVALID)
		{
			error_output << "Unknown input type in section " << i->first << std::endl;
			continue;
		}

		CONTROL newctrl;
		if (type == "joy")
		{
			newctrl.joynum = 0;
			std::string joy_type;
			if (!controls_config.GetParam(i, "joy_index", newctrl.joynum, error_output)) continue;
			if (!controls_config.GetParam(i, "joy_type", joy_type, error_output)) continue;

			newctrl.type = CONTROL::JOY;
			if (joy_type == "button")
			{
				newctrl.joytype = CONTROL::JOYBUTTON;
				newctrl.joybutton = 0;
				newctrl.joypushdown = false;
				newctrl.onetime = false;
				if (!controls_config.GetParam(i, "joy_button", newctrl.joybutton, error_output)) continue;
				if (!controls_config.GetParam(i, "down", newctrl.joypushdown, error_output)) continue;
				if (!controls_config.GetParam(i, "once", newctrl.onetime, error_output)) continue;
			}
			else if (joy_type == "axis")
			{
				newctrl.joytype = CONTROL::JOYAXIS;
				int joy_axis = 0;
				std::string axis_type;
				float deadzone = 0.0;
				float exponent = 0.0;
				float gain = 0.0;
				if (!controls_config.GetParam(i, "joy_axis", joy_axis, error_output)) continue;
				if (!controls_config.GetParam(i, "joy_axis_type", axis_type, error_output)) continue;
				if (!controls_config.GetParam(i, "deadzone", deadzone, error_output)) continue;
				if (!controls_config.GetParam(i, "exponent", exponent, error_output)) continue;
				if (!controls_config.GetParam(i, "gain", gain, error_output)) continue;

				newctrl.joyaxis = joy_axis;
				if (axis_type == "positive")
				{
					newctrl.joyaxistype = CONTROL::POSITIVE;
				}
				else if (axis_type == "negative")
				{
					newctrl.joyaxistype = CONTROL::NEGATIVE;
				}
				else if (axis_type == "both")
				{
					newctrl.joyaxistype = CONTROL::BOTH;
				}
				else
				{
					error_output << "Error parsing controls, invalid joystick axis type in section " << i->first << std::endl;
					continue;
				}
				newctrl.deadzone = deadzone;
				newctrl.exponent = exponent;
				newctrl.gain = gain;
			}
			else
			{
				error_output << "Error parsing controls, invalid joystick type in section " << i->first << std::endl;
				continue;
			}
		}
		else if (type == "key")
		{
			newctrl.type = CONTROL::KEY;
			int keycode = 0;
			bool key_down = false;
			bool key_once = false;
			if (!controls_config.GetParam(i, "keycode", keycode))
			{
				std::string keyname;
				if (!controls_config.GetParam(i, "key", keyname, error_output)) continue;

				if (legacy_keycodes.find(keyname) == legacy_keycodes.end())
				{
					error_output << "Unknown keyname \"" << keyname << "\" parsing controls in section " << i->first << std::endl;
					continue;
				}
				else
				{
					keycode = legacy_keycodes[keyname];
				}
			}

			if (!controls_config.GetParam(i, "down", key_down, error_output)) continue;
			if (!controls_config.GetParam(i, "once", key_once, error_output)) continue;
			if (keycode < SDLK_LAST && keycode > SDLK_FIRST)
			{
				newctrl.keycode = SDLKey(keycode);
			}
			newctrl.keypushdown = key_down;
			newctrl.onetime = key_once;
		}
		else if (type == "mouse")
		{
			newctrl.type = CONTROL::MOUSE;

			std::string mouse_type = "";
			std::string mouse_direction = "";
			controls_config.GetParam(i, "mouse_type", mouse_type );
			if (mouse_type == "button")
			{
				int mouse_btn = 0;
				bool mouse_btn_down = false;
				bool mouse_btn_once = false;
				newctrl.mousetype = CONTROL::MOUSEBUTTON;
				controls_config.GetParam(i, "mouse_button", mouse_btn);
				controls_config.GetParam(i, "down", mouse_btn_down);
				controls_config.GetParam(i, "once", mouse_btn_once);
				newctrl.mbutton = mouse_btn;
				newctrl.mouse_push_down = mouse_btn_down;
				newctrl.onetime = mouse_btn_once;
			}
			else if (mouse_type == "motion")
			{
				newctrl.mousetype = CONTROL::MOUSEMOTION;
				controls_config.GetParam(i, "mouse_motion", mouse_direction);
				if (mouse_direction == "left")
				{
					newctrl.mdir = CONTROL::LEFT;
				}
				else if (mouse_direction == "right")
				{
					newctrl.mdir = CONTROL::RIGHT;
				}
				else if (mouse_direction == "up")
				{
					newctrl.mdir = CONTROL::UP;
				}
				else if (mouse_direction == "down")
				{
					newctrl.mdir = CONTROL::DOWN;
				}
				else
				{
					error_output << "Error parsing controls, invalid mouse direction type " << mouse_direction << " in section " << i->first << std::endl;
				}

				newctrl.deadzone=0;
				newctrl.exponent=1;
				newctrl.gain=1;
				controls_config.GetParam(i, "deadzone", newctrl.deadzone);
				controls_config.GetParam(i, "exponent", newctrl.exponent);
				controls_config.GetParam(i, "gain", newctrl.gain);
			}
			else
			{
				error_output << "Error parsing controls, invalid mouse type " << mouse_type << " in section " << i->first << std::endl;
			}
		}
		else
		{
			error_output << "Error parsing controls, invalid control type in section " << i->first << std::endl;
			continue;
		}

		controls[carinput].push_back(newctrl);
	}

	inputs.resize(CARINPUT::INVALID, 0.0); //this looks weird, but it initialize all inputs and sets them to zero
	lastinputs.resize(CARINPUT::INVALID, 0.0); //this looks weird, but it initialize all inputs and sets them to zero
	return true;
}

void CARCONTROLMAP_LOCAL::Save(const std::string & controlfile, std::ostream & info_output, std::ostream & error_output)
{
	CONFIG controls_config;
	Save(controls_config, info_output, error_output);
	controls_config.Write(true, controlfile);
}

void CARCONTROLMAP_LOCAL::Save(CONFIG & controls_config, std::ostream & info_output, std::ostream & error_output)
{
	int id = 0;
	for (std::map <CARINPUT::CARINPUT, std::vector <CONTROL> >::iterator n = controls.begin(); n != controls.end(); ++n)
	{
		for (std::vector <CONTROL>::iterator i = n->second.begin(); i != n->second.end(); ++i)
		{
			std::string ctrl_name = "INVALID";
			CARINPUT::CARINPUT inputid = n->first;
			for (std::map <std::string, CARINPUT::CARINPUT>::const_iterator s = carinput_stringmap.begin(); s != carinput_stringmap.end(); ++s)
			{
				if (s->second == inputid) ctrl_name = s->first;
			}

			std::stringstream ss;
			ss << "control mapping " << std::setfill('0') << std::setw(2) << id;

			CONFIG::iterator section;
			controls_config.GetSection(ss.str(), section);
			controls_config.SetParam(section, "name", ctrl_name);

			CONTROL & curctrl = *i;
			if (curctrl.type == CONTROL::JOY)
			{
				controls_config.SetParam(section, "type", "joy");
				controls_config.SetParam(section, "joy_index", curctrl.joynum);

				if (curctrl.joytype == CONTROL::JOYAXIS)
				{
					controls_config.SetParam(section, "joy_type", "axis");
					controls_config.SetParam(section, "joy_axis", curctrl.joyaxis );
					switch (curctrl.joyaxistype) {
						case CONTROL::POSITIVE:
							controls_config.SetParam(section, "joy_axis_type", "positive");
							break;
						case CONTROL::NEGATIVE:
							controls_config.SetParam(section, "joy_axis_type", "negative");
							break;
						case CONTROL::BOTH:
							controls_config.SetParam(section, "joy_axis_type", "both");
							break;
					}
					controls_config.SetParam(section, "deadzone", curctrl.deadzone);
					controls_config.SetParam(section, "exponent", curctrl.exponent);
					controls_config.SetParam(section, "gain", curctrl.gain);
				}
				else if (curctrl.joytype == CONTROL::JOYBUTTON)
				{
					controls_config.SetParam(section, "joy_type", "button");
					controls_config.SetParam(section, "joy_button", curctrl.joybutton);
					controls_config.SetParam(section, "once", curctrl.onetime);
					controls_config.SetParam(section, "down", curctrl.joypushdown);
				}
			}
			else if (curctrl.type == CONTROL::KEY)
			{
				controls_config.SetParam(section, "type", "key");
				std::string keyname = "UNKNOWN";
				for (std::map <std::string, int>::iterator k = legacy_keycodes.begin(); k != legacy_keycodes.end(); k++)
					if (k->second == curctrl.keycode)
						keyname = k->first;
				controls_config.SetParam(section, "key", keyname);
				controls_config.SetParam(section, "keycode", curctrl.keycode);
				controls_config.SetParam(section, "once", curctrl.onetime);
				controls_config.SetParam(section, "down", curctrl.keypushdown);
			}
			else if (curctrl.type == CONTROL::MOUSE)
			{
				controls_config.SetParam(section, "type", "mouse");
				if (curctrl.mousetype == CONTROL::MOUSEBUTTON)
				{
					controls_config.SetParam(section, "mouse_type", "button");
					controls_config.SetParam(section, "mouse_button", curctrl.mbutton );
					controls_config.SetParam(section, "once", curctrl.onetime );
					controls_config.SetParam(section, "down", curctrl.mouse_push_down );
				}
				else if (curctrl.mousetype == CONTROL::MOUSEMOTION)
				{
					std::string direction = "invalid";
					CONTROL::MOUSEDIRECTION mdir = curctrl.mdir;
					if ( mdir == CONTROL::UP )
					{
						direction = "up";
					}
					else if ( mdir == CONTROL::DOWN )
					{
						direction = "down";
					}
					else if ( mdir == CONTROL::LEFT )
					{
						direction = "left";
					}
					else if ( mdir == CONTROL::RIGHT )
					{
						direction = "right";
					}

					controls_config.SetParam(section, "mouse_type", "motion");
					controls_config.SetParam(section, "mouse_motion", direction);

					controls_config.SetParam(section, "deadzone", curctrl.deadzone);
					controls_config.SetParam(section, "exponent", curctrl.exponent);
					controls_config.SetParam(section, "gain", curctrl.gain);
				}
			}

			id++;
		}
	}
}

void CARCONTROLMAP_LOCAL::AddInputKey(const std::string & inputname, bool analog, bool only_one, SDLKey key, std::ostream & error_output)
{
	CONTROL newctrl;

	newctrl.type = CONTROL::KEY;
	newctrl.keycode = key;
	newctrl.keypushdown = true;
	newctrl.onetime = !analog;

	//std::cout << "Assigning input " << inputname << " keycode " << key << " onlyone " << only_one << std::endl;

	AddControl(newctrl, inputname, only_one, error_output);
}

void CARCONTROLMAP_LOCAL::AddInputMouseButton(const std::string & inputname, bool analog, bool only_one, int mouse_btn, std::ostream & error_output)
{
	CONTROL newctrl;

	newctrl.type = CONTROL::MOUSE;
	newctrl.mousetype = CONTROL::MOUSEBUTTON;
	newctrl.mbutton = mouse_btn;
	newctrl.mouse_push_down = true;
	newctrl.onetime = !analog;

	//std::cout << "Assigning input " << inputname << " keycode " << key << " onlyone " << only_one << std::endl;

	AddControl(newctrl, inputname, only_one, error_output);
}

void CARCONTROLMAP_LOCAL::AddInputMouseMotion(const std::string & inputname, bool analog, bool only_one, const std::string & mouse_direction, std::ostream & error_output)
{
	CONTROL newctrl;

	newctrl.type = CONTROL::MOUSE;
	newctrl.mousetype = CONTROL::MOUSEMOTION;
	if (mouse_direction == "left")
		newctrl.mdir = CONTROL::LEFT;
	else if (mouse_direction == "right")
		newctrl.mdir = CONTROL::RIGHT;
	else if (mouse_direction == "up")
		newctrl.mdir = CONTROL::UP;
	else if (mouse_direction == "down")
		newctrl.mdir = CONTROL::DOWN;
	else
		return;

	//std::cout << "Assigning input " << inputname << " keycode " << key << " onlyone " << only_one << std::endl;

	AddControl(newctrl, inputname, only_one, error_output);
}

void CARCONTROLMAP_LOCAL::AddInputJoyButton(const std::string & inputname, bool analog, bool only_one, int joy_num, int joy_btn, std::ostream & error_output)
{
	CONTROL newctrl;

	newctrl.type = CONTROL::JOY;
	newctrl.joynum = joy_num;
	newctrl.joytype = CONTROL::JOYBUTTON;
	newctrl.joybutton = joy_btn;
	newctrl.joypushdown = true;
	newctrl.onetime = !analog;

	//std::cout << "Assigning input " << inputname << " keycode " << key << " onlyone " << only_one << std::endl;

	AddControl(newctrl, inputname, only_one, error_output);
}

void CARCONTROLMAP_LOCAL::AddInputJoyAxis(const std::string & inputname, bool analog, bool only_one, int joy_num, int joy_axis, const std::string & axis_type, std::ostream & error_output)
{
	CONTROL newctrl;

	newctrl.type = CONTROL::JOY;
	newctrl.joytype = CONTROL::JOYAXIS;
	newctrl.joynum = joy_num;
	float deadzone = 0.0;
	float exponent = 1.0;
	float gain = 1.0;
	newctrl.joyaxis = ( joy_axis );
	if ( axis_type == "positive" )
	{
		newctrl.joyaxistype = CONTROL::POSITIVE;
	}
	else if ( axis_type == "negative" )
	{
		newctrl.joyaxistype = CONTROL::NEGATIVE;
	}
	else if ( axis_type == "both" )
	{
		newctrl.joyaxistype = CONTROL::BOTH;
	}
	else
	{
		return;
	}
	newctrl.deadzone = ( deadzone );
	newctrl.exponent = ( exponent );
	newctrl.gain = ( gain );

	//std::cout << "Assigning input " << inputname << " keycode " << key << " onlyone " << only_one << std::endl;

	AddControl(newctrl, inputname, only_one, error_output);
}

void CARCONTROLMAP_LOCAL::DeleteControl(const CONTROL & ctrltodel, const std::string & inputname, std::ostream & error_output)
{
	if (carinput_stringmap.find(inputname) != carinput_stringmap.end())
	{
		assert(controls.find(carinput_stringmap[inputname]) != controls.end());

		std::vector<CONTROL>::iterator todel = controls[carinput_stringmap[inputname]].end();

		for (std::vector<CONTROL>::iterator i =
			controls[carinput_stringmap[inputname]].begin();
			i != controls[carinput_stringmap[inputname]].end(); ++i)
		{
			if (ctrltodel == *i)
				todel = i;
			else
			{
				/*std::cout << "this: ";
				i->DebugPrint(std::cout);
				std::cout << "not : ";
				ctrltodel.DebugPrint(std::cout);*/
			}
		}
		assert (todel != controls[carinput_stringmap[inputname]].end());
		controls[carinput_stringmap[inputname]].erase(todel);
	}
	else
		error_output << "Input named " << inputname << " couldn't be deleted because it isn't used" << std::endl;
}

void CARCONTROLMAP_LOCAL::UpdateControl(const CONTROL & ctrltoupdate, const std::string & inputname, std::ostream & error_output)
{
	if (carinput_stringmap.find(inputname) != carinput_stringmap.end())
	{
		//input name was found

		//verify a control list was found for this input
		assert(controls.find(carinput_stringmap[inputname]) != controls.end());

		bool did_update = false;

		for (std::vector<CONTROL>::iterator i =
			controls[carinput_stringmap[inputname]].begin();
			i != controls[carinput_stringmap[inputname]].end(); ++i)
		{
			if (ctrltoupdate == *i)
			{
				*i = ctrltoupdate;
				did_update = true;
				//std::cout << "New deadzone: " << i->deadzone << std::endl;
			}
			else
			{
				/*std::cout << "this: ";
				i->DebugPrint(std::cout);
				std::cout << "not : ";
				ctrltodel.DebugPrint(std::cout);*/
			}
		}

		assert(did_update);
	}
	else
		error_output << "Input named " << inputname << " couldn't be deleted because it isn't used" << std::endl;
}

const std::vector <float> & CARCONTROLMAP_LOCAL::ProcessInput(const std::string & joytype, EVENTSYSTEM_SDL & eventsystem, float steerpos, float dt, bool joy_200, float carms, float speedsens, int screenw, int screenh, float button_ramp, bool hgateshifter)
{
	assert(inputs.size() == CARINPUT::INVALID); //this looks weird, but it ensures that our inputs vector contains exactly one item per input
	assert(lastinputs.size() == CARINPUT::INVALID); //this looks weird, but it ensures that our inputs vector contains exactly one item per input

	for (std::map <CARINPUT::CARINPUT, std::vector <CONTROL> >::iterator n = controls.begin(); n != controls.end(); ++n)
	{
		float newval = 0.0;

		for (std::vector <CONTROL>::iterator i = n->second.begin(); i != n->second.end(); ++i)
		{
			bool handled = false;
			float tempval = newval;

			if (i->type == CONTROL::JOY)
			{
				//cout << "type joy" << std::endl;

				if (i->joytype == CONTROL::JOYAXIS)
				{
					float val = eventsystem.GetJoyAxis(i->joynum, i->joyaxis);
					if (i->joyaxistype == CONTROL::NEGATIVE)
						val = -val;
					val = ApplyDeadzone(i->deadzone,val);
					val = ApplyGain(i->gain,val);

					double absval = val;
					bool neg = false;
					if (val < 0)
					{
						absval = -val;
						neg = true;
					}
					val = ApplyExponent(i->exponent,absval);
					if (neg)
						val = -val;

					tempval = val;
					handled = true;
				}
				else if (i->joytype == CONTROL::JOYBUTTON)
				{
					TOGGLE button = eventsystem.GetJoyButton(i->joynum, i->joybutton);

					if (i->onetime)
					{
						if (i->joypushdown && button.GetImpulseRising())
							tempval = 1.0;
						else if (!i->joypushdown && button.GetImpulseFalling())
							tempval = 1.0;
						else
							tempval = 0.0;
						handled = true;
					}
					else
					{
						float downval = 1.0;
						float upval = 0.0;
						if (!i->joypushdown)
						{
							downval = 0.0;
							upval = 1.0;
						}

						tempval = Ramp(lastinputs[n->first], button.GetState() ? downval : upval, button_ramp, dt);
						handled = true;
					}
				}
			}
			else if (i->type == CONTROL::KEY)
			{
				//cout << "type key" << std::endl;

				EVENTSYSTEM_SDL::BUTTON_STATE keystate = eventsystem.GetKeyState(SDLKey(i->keycode));

				if (i->onetime)
				{
					if (i->keypushdown && keystate.just_down)
						tempval = 1.0;
					else if (!i->keypushdown && keystate.just_up)
						tempval = 1.0;
					else
						tempval = 0.0;
					handled = true;
				}
				else
				{
					float downval = 1.0;
					float upval = 0.0;
					if (!i->keypushdown)
					{
						downval = 0.0;
						upval = 1.0;
					}

					//if (inputs[n->first] != keystate.down ? downval : upval) std::cout << "Key ramp: " << i->keycode << ", " << n->first << std::endl;
					tempval = Ramp(lastinputs[n->first], keystate.down ? downval : upval, button_ramp, dt);

					handled = true;
				}
			}
			else if (i->type == CONTROL::MOUSE)
			{
				//cout << "type mouse" << std::endl;

				if (i->mousetype == CONTROL::MOUSEBUTTON)
				{
					//cout << "mousebutton" << std::endl;

					EVENTSYSTEM_SDL::BUTTON_STATE buttonstate = eventsystem.GetMouseButtonState(i->mbutton);

					if (i->onetime)
					{
						if (i->mouse_push_down && buttonstate.just_down)
							tempval = 1.0;
						else if (!i->mouse_push_down && buttonstate.just_up)
							tempval = 1.0;
						else
							tempval = 0.0;
						handled = true;
					}
					else
					{
						float downval = 1.0;
						float upval = 0.0;
						if (!i->mouse_push_down)
						{
							downval = 0.0;
							upval = 1.0;
						}

						tempval = Ramp(lastinputs[n->first], buttonstate.down ? downval : upval, button_ramp, dt);
						handled = true;
					}
				}
				else if (i->mousetype == CONTROL::MOUSEMOTION)
				{
					//cout << "mousemotion" << std::endl;

					std::vector <int> pos = eventsystem.GetMousePosition();
					//std::cout << pos[0] << "," << pos[1] << std::endl;

					float xval = (pos[0]-screenw/2.0)/(screenw/4.0);
					if (xval < -1) xval = -1;
					if (xval > 1) xval = 1;

					float yval = (pos[1]-screenh/2.0)/(screenh/4.0);
					if (yval < -1) yval = -1;
					if (yval > 1) yval = 1;

					float val = 0;

					if (i->mdir == CONTROL::UP)
						val = -yval;
					else if (i->mdir == CONTROL::DOWN)
						val = yval;
					else if (i->mdir == CONTROL::LEFT)
						val = -xval;
					else if (i->mdir == CONTROL::RIGHT)
						val = xval;

					if (val < 0)
						val = 0;
					else if (val > 1)
						val = 1;

					val = ApplyDeadzone(i->deadzone,val);
					val = ApplyGain(i->gain,val);

					if (val < 0)
						val = 0;
					else if (val > 1)
						val = 1;

					double absval = val;
					bool neg = false;
					if (val < 0)
					{
						absval = -val;
						neg = true;
					}
					val = ApplyExponent(i->exponent,absval);
					if (neg)
						val = -val;

					if (val < 0)
						val = 0;
					else if (val > 1)
						val = 1;

					tempval = val;

					//cout << val << std::endl;

					handled = true;
				}
				//else cout << "mouse???" << std::endl;
			}
			//else cout << "type invalid" << std::endl;

			if (tempval > newval)
				newval = tempval;

			assert(handled);
		}

		if (newval < 0)
			newval = 0;
		if (newval > 1.0)
			newval = 1.0;

		inputs[n->first] = newval;

		//std::cout << "New input value: " << inputs[n->first] << std::endl;
	}

	if (hgateshifter)
	{
		bool havegear = inputs[CARINPUT::FIRST_GEAR] ||
				inputs[CARINPUT::SECOND_GEAR] ||
				inputs[CARINPUT::THIRD_GEAR] ||
				inputs[CARINPUT::FOURTH_GEAR] ||
				inputs[CARINPUT::FIFTH_GEAR] ||
				inputs[CARINPUT::SIXTH_GEAR] ||
				inputs[CARINPUT::REVERSE];
		if (!havegear)
			inputs[CARINPUT::NEUTRAL] = 1.0;
	}

	lastinputs = inputs;

	//do steering processing
	ProcessSteering(joytype, steerpos, dt, joy_200, carms*2.23693629, speedsens);

	return inputs;
}

std::string CARCONTROLMAP_LOCAL::GetStringFromInput(const CARINPUT::CARINPUT input) const
{
	for (std::map <std::string, CARINPUT::CARINPUT>::const_iterator i =
		carinput_stringmap.begin(); i != carinput_stringmap.end(); ++i)
	{
		if (i->second == input) return i->first;
	}
	return "INVALID";
}

CARINPUT::CARINPUT CARCONTROLMAP_LOCAL::GetInputFromString(const std::string & str) const
{
	std::map <std::string, CARINPUT::CARINPUT>::const_iterator i = carinput_stringmap.find(str);
	if (i != carinput_stringmap.end())
		return i->second;
	else
		return CARINPUT::INVALID;
}

void CARCONTROLMAP_LOCAL::AddControl(CONTROL newctrl, const std::string & inputname, bool only_one, std::ostream & error_output)
{
	if (carinput_stringmap.find(inputname) != carinput_stringmap.end())
	{
		if (only_one)
			controls[carinput_stringmap[inputname]].clear();
		controls[carinput_stringmap[inputname]].push_back(newctrl);

		//remove duplicates
		std::sort (controls[carinput_stringmap[inputname]].begin(), controls[carinput_stringmap[inputname]].end());
		std::vector<CONTROL>::iterator it = std::unique(controls[carinput_stringmap[inputname]].begin(), controls[carinput_stringmap[inputname]].end());
		controls[carinput_stringmap[inputname]].resize( it - controls[carinput_stringmap[inputname]].begin() );
	}
	else
		error_output << "Input named " << inputname << " couldn't be assigned because it isn't used" << std::endl;
}

void CARCONTROLMAP_LOCAL::ProcessSteering(const std::string & joytype, float steerpos, float dt, bool joy_200, float carmph, float speedsens)
{
	//std::cout << "steerpos: " << steerpos << std::endl;

	float val = inputs[CARINPUT::STEER_RIGHT];
	if (std::abs(inputs[CARINPUT::STEER_LEFT]) > std::abs(inputs[CARINPUT::STEER_RIGHT])) //use whichever control is larger
		val = -inputs[CARINPUT::STEER_LEFT];

	/*if (val != 0)
	{
		std::cout << "Initial steer left: " << inputs[CARINPUT::STEER_LEFT] << std::endl;
		std::cout << "Initial steer right: " << inputs[CARINPUT::STEER_RIGHT] << std::endl;
		std::cout << "Initial val: " << val << std::endl;
	}*/

	//std::cout << "val: " << val << std::endl;

	//restrict joystick range if required
	if (joy_200)
	{
		float decimate = 4.5;

		float normalat = 30;
		float transat = 15;

		if (carmph < transat)
			decimate = 1.0;
		else if (carmph < normalat)
		{
			float coeff = (carmph - transat)/(normalat - transat);
			decimate = (decimate-1.0f)*coeff + 1.0f;
		}

		//std::cout << "Decimating: " << val << " to " << val / decimate << std::endl;

		val = val/decimate;
	}

	//do speed sensitivity
	if ( speedsens != 0.0 )
	{
		float coeff = 1.0;
		if (carmph > 1)
		{
			float ratio = 20.0f;
			float ssco = speedsens*(1.0f-pow(val,2.0f));
			coeff = ratio*45.0f*(1.0f-atan(carmph*80.0f*ssco)*0.6366198);
		}
		if (coeff > 1.0f) coeff = 1.0f;

		//std::cout << "Speed sensitivity coefficient: " << coeff << std::endl;

		val = val*coeff;
	}

	//std::cout << "After speed sensitivity: " << val << std::endl;

	//rate-limit non-wheel controls
	if (joytype != "wheel")
	{
		//if (i->first == inputs[CARINPUT::STEER_LEFT])
		//steerpos = -steerpos;
		float steerstep = 5.0*dt;

		if (val > steerpos)
		{
			if (val - steerpos <= steerstep)
				steerpos = val;
			else
				steerpos += steerstep;
		}
		else
		{
			if (steerpos - val <= steerstep)
				steerpos = val;
			else
				steerpos -= steerstep;
		}

		if (steerpos > 1.0)
			steerpos = 1.0;
		if (steerpos < -1.0)
			steerpos = -1.0;

		val = steerpos;

		/*float coeff = 0.97;
		val = steerpos * coeff + val * (1.0-coeff);*/
	}

	//std::cout << "After rate limit val: " << val << std::endl;

	//std::cout << "Steer left: " << inputs[CARINPUT::STEER_LEFT] << std::endl;
	//std::cout << "Steer right: " << inputs[CARINPUT::STEER_RIGHT] << std::endl;
	//std::cout << "Current steering: " << car.GetLastSteer() << std::endl;
	//std::cout << "New steering: " << val << std::endl;

	inputs[CARINPUT::STEER_LEFT] = 0.0;
	inputs[CARINPUT::STEER_RIGHT] = 0.0;
	if (val < 0)
		inputs[CARINPUT::STEER_LEFT] = -val;
	else
		inputs[CARINPUT::STEER_RIGHT] = val;
	//inputs[CARINPUT::STEER_RIGHT] = val;
	//inputs[CARINPUT::STEER_LEFT] = -val;

	/*if (val != 0)
	{
		std::cout << "Steer left: " << inputs[CARINPUT::STEER_LEFT] << std::endl;
		std::cout << "Steer right: " << inputs[CARINPUT::STEER_RIGHT] << std::endl;
	}*/
}

void CARCONTROLMAP_LOCAL::PopulateLegacyKeycodes()
{
	legacy_keycodes["BACKSPACE"] = 8;
	legacy_keycodes["TAB"] = 9;
	legacy_keycodes["CLEAR"] = 12;
	legacy_keycodes["RETURN"] = 13;
	legacy_keycodes["PAUSE"] = 19;
	legacy_keycodes["ESCAPE"] = 27;
	legacy_keycodes["SPACE"] = 32;
	legacy_keycodes["EXCLAIM"] = 33;
	legacy_keycodes["QUOTEDBL"] = 34;
	legacy_keycodes["HASH"] = 35;
	legacy_keycodes["DOLLAR"] = 36;
	legacy_keycodes["AMPERSAND"] = 38;
	legacy_keycodes["QUOTE"] = 39;
	legacy_keycodes["LEFTPAREN"] = 40;
	legacy_keycodes["RIGHTPAREN"] = 41;
	legacy_keycodes["ASTERISK"] = 42;
	legacy_keycodes["PLUS"] = 43;
	legacy_keycodes["COMMA"] = 44;
	legacy_keycodes["MINUS"] = 45;
	legacy_keycodes["PERIOD"] = 46;
	legacy_keycodes["SLASH"] = 47;
	legacy_keycodes["0"] = 48;
	legacy_keycodes["1"] = 49;
	legacy_keycodes["2"] = 50;
	legacy_keycodes["3"] = 51;
	legacy_keycodes["4"] = 52;
	legacy_keycodes["5"] = 53;
	legacy_keycodes["6"] = 54;
	legacy_keycodes["7"] = 55;
	legacy_keycodes["8"] = 56;
	legacy_keycodes["9"] = 57;
	legacy_keycodes["COLON"] = 58;
	legacy_keycodes["SEMICOLON"] = 59;
	legacy_keycodes["LESS"] = 60;
	legacy_keycodes["EQUALS"] = 61;
	legacy_keycodes["GREATER"] = 62;
	legacy_keycodes["QUESTION"] = 63;
	legacy_keycodes["AT"] = 64;
	legacy_keycodes["LEFTBRACKET"] = 91;
	legacy_keycodes["BACKSLASH"] = 92;
	legacy_keycodes["RIGHTBRACKET"] = 93;
	legacy_keycodes["CARET"] = 94;
	legacy_keycodes["UNDERSCORE"] = 95;
	legacy_keycodes["BACKQUOTE"] = 96;
	legacy_keycodes["a"] = 97;
	legacy_keycodes["b"] = 98;
	legacy_keycodes["c"] = 99;
	legacy_keycodes["d"] = 100;
	legacy_keycodes["e"] = 101;
	legacy_keycodes["f"] = 102;
	legacy_keycodes["g"] = 103;
	legacy_keycodes["h"] = 104;
	legacy_keycodes["i"] = 105;
	legacy_keycodes["j"] = 106;
	legacy_keycodes["k"] = 107;
	legacy_keycodes["l"] = 108;
	legacy_keycodes["m"] = 109;
	legacy_keycodes["n"] = 110;
	legacy_keycodes["o"] = 111;
	legacy_keycodes["p"] = 112;
	legacy_keycodes["q"] = 113;
	legacy_keycodes["r"] = 114;
	legacy_keycodes["s"] = 115;
	legacy_keycodes["t"] = 116;
	legacy_keycodes["u"] = 117;
	legacy_keycodes["v"] = 118;
	legacy_keycodes["w"] = 119;
	legacy_keycodes["x"] = 120;
	legacy_keycodes["y"] = 121;
	legacy_keycodes["z"] = 122;
	legacy_keycodes["DELETE"] = 127;
	legacy_keycodes["KP0"] = 256;
	legacy_keycodes["KP1"] = 257;
	legacy_keycodes["KP2"] = 258;
	legacy_keycodes["KP3"] = 259;
	legacy_keycodes["KP4"] = 260;
	legacy_keycodes["KP5"] = 261;
	legacy_keycodes["KP6"] = 262;
	legacy_keycodes["KP7"] = 263;
	legacy_keycodes["KP8"] = 264;
	legacy_keycodes["KP9"] = 265;
	legacy_keycodes["KP_PERIOD"] = 266;
	legacy_keycodes["KP_DIVIDE"] = 267;
	legacy_keycodes["KP_MULTIPLY"] = 268;
	legacy_keycodes["KP_MINUS"] = 269;
	legacy_keycodes["KP_PLUS"] = 270;
	legacy_keycodes["KP_ENTER"] = 271;
	legacy_keycodes["KP_EQUALS"] = 272;
	legacy_keycodes["UP"] = 273;
	legacy_keycodes["DOWN"] = 274;
	legacy_keycodes["RIGHT"] = 275;
	legacy_keycodes["LEFT"] = 276;
	legacy_keycodes["INSERT"] = 277;
	legacy_keycodes["HOME"] = 278;
	legacy_keycodes["END"] = 279;
	legacy_keycodes["PAGEUP"] = 280;
	legacy_keycodes["PAGEDOWN"] = 281;
	legacy_keycodes["F1"] = 282;
	legacy_keycodes["F2"] = 283;
	legacy_keycodes["F3"] = 284;
	legacy_keycodes["F4"] = 285;
	legacy_keycodes["F5"] = 286;
	legacy_keycodes["F6"] = 287;
	legacy_keycodes["F7"] = 288;
	legacy_keycodes["F8"] = 289;
	legacy_keycodes["F9"] = 290;
	legacy_keycodes["F10"] = 291;
	legacy_keycodes["F11"] = 292;
	legacy_keycodes["F12"] = 293;
	legacy_keycodes["F13"] = 294;
	legacy_keycodes["F14"] = 295;
	legacy_keycodes["F15"] = 296;
	legacy_keycodes["NUMLOCK"] = 300;
	legacy_keycodes["CAPSLOCK"] = 301;
	legacy_keycodes["SCROLLOCK"] = 302;
	legacy_keycodes["RSHIFT"] = 303;
	legacy_keycodes["LSHIFT"] = 304;
	legacy_keycodes["RCTRL"] = 305;
	legacy_keycodes["LCTRL"] = 306;
	legacy_keycodes["RALT"] = 307;
	legacy_keycodes["LALT"] = 308;
	legacy_keycodes["RMETA"] = 309;
	legacy_keycodes["LMETA"] = 310;
	legacy_keycodes["LSUPER"] = 311;
	legacy_keycodes["RSUPER"] = 312;
	legacy_keycodes["ALTGR"] = 313;
}
