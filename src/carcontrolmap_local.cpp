#include "carcontrolmap_local.h"
#include "config.h"

#include <string>
#include <list>
#include <iomanip>
#include <algorithm>
#include <cmath>

/// ramps the start value to the end value using rate button_ramp.
/// if button_ramp is zero, infinite rate is assumed.
static inline float Ramp(float start, float end, float button_ramp, float dt)
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

static inline float ApplyDeadzone(float dz, float val)
{
	if (fabs(val) < dz)
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

static inline float ApplyGain(float gain, float val)
{
	val *= gain;
	if (val < -1.0)
		val = -1.0;
	if (val > 1.0)
		val = 1.0;

	return val;
}

static inline float ApplyExponent(float exponent, float val)
{
	val = pow(val, exponent);
	if (val < -1.0)
		val = -1.0;
	if (val > 1.0)
		val = 1.0;

	return val;
}

static const std::string invalid("INVALID");

const std::vector<std::string> CARCONTROLMAP_LOCAL::carinput_strings = CARCONTROLMAP_LOCAL::InitCarInputStrings();

const std::map <std::string, CARINPUT::CARINPUT> CARCONTROLMAP_LOCAL::carinput_stringmap = CARCONTROLMAP_LOCAL::InitCarInputStringMap();

const std::map <std::string, int> CARCONTROLMAP_LOCAL::keycode_stringmap = CARCONTROLMAP_LOCAL::InitKeycodeStringMap();

CARCONTROLMAP_LOCAL::CARCONTROLMAP_LOCAL() :
	controls(CARINPUT::INVALID)
{
	// constructor
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
	controls.resize(CARINPUT::INVALID);
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
				newctrl.keycode = 0;
				newctrl.pushdown = false;
				newctrl.onetime = false;
				if (!controls_config.GetParam(i, "joy_button", newctrl.keycode, error_output)) continue;
				if (!controls_config.GetParam(i, "down", newctrl.pushdown, error_output)) continue;
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

				keycode = GetKeycodeFromString(keyname);
				if (keycode == 0)
				{
					error_output << "Unknown keyname \"" << keyname << "\" parsing controls in section " << i->first << std::endl;
					continue;
				}
			}

			if (!controls_config.GetParam(i, "down", key_down, error_output)) continue;
			if (!controls_config.GetParam(i, "once", key_once, error_output)) continue;
			if (keycode < SDLK_LAST && keycode > SDLK_FIRST)
			{
				newctrl.keycode = SDLKey(keycode);
			}
			newctrl.pushdown = key_down;
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
				newctrl.keycode = mouse_btn;
				newctrl.pushdown = mouse_btn_down;
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

void CARCONTROLMAP_LOCAL::Save(const std::string & controlfile)
{
	CONFIG controls_config;
	Save(controls_config);
	controls_config.Write(controlfile);
}

void CARCONTROLMAP_LOCAL::Save(CONFIG & controls_config)
{
	int id = 0;
	for (size_t n = 0; n < controls.size(); ++n)
	{
		std::string ctrl_name = GetStringFromInput(CARINPUT::CARINPUT(n));
		if (ctrl_name.empty())
			continue;

		for (size_t i = 0; i < controls[n].size(); ++i)
		{
			std::stringstream ss;
			ss << "control mapping " << std::setfill('0') << std::setw(2) << id;

			CONFIG::iterator section;
			controls_config.GetSection(ss.str(), section);
			controls_config.SetParam(section, "name", ctrl_name);

			CONTROL & curctrl = controls[n][i];
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
					}
					controls_config.SetParam(section, "deadzone", curctrl.deadzone);
					controls_config.SetParam(section, "exponent", curctrl.exponent);
					controls_config.SetParam(section, "gain", curctrl.gain);
				}
				else if (curctrl.joytype == CONTROL::JOYBUTTON)
				{
					controls_config.SetParam(section, "joy_type", "button");
					controls_config.SetParam(section, "joy_button", curctrl.keycode);
					controls_config.SetParam(section, "once", curctrl.onetime);
					controls_config.SetParam(section, "down", curctrl.pushdown);
				}
			}
			else if (curctrl.type == CONTROL::KEY)
			{
				controls_config.SetParam(section, "type", "key");
				std::string keyname = GetStringFromKeycode(curctrl.keycode);
				controls_config.SetParam(section, "key", keyname);
				controls_config.SetParam(section, "keycode", curctrl.keycode);
				controls_config.SetParam(section, "once", curctrl.onetime);
				controls_config.SetParam(section, "down", curctrl.pushdown);
			}
			else if (curctrl.type == CONTROL::MOUSE)
			{
				controls_config.SetParam(section, "type", "mouse");
				if (curctrl.mousetype == CONTROL::MOUSEBUTTON)
				{
					controls_config.SetParam(section, "mouse_type", "button");
					controls_config.SetParam(section, "mouse_button", curctrl.keycode );
					controls_config.SetParam(section, "once", curctrl.onetime );
					controls_config.SetParam(section, "down", curctrl.pushdown );
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

const std::vector <float> & CARCONTROLMAP_LOCAL::ProcessInput(const std::string & joytype, EVENTSYSTEM_SDL & eventsystem, float steerpos, float dt, bool joy_200, float carms, float speedsens, int screenw, int screenh, float button_ramp, bool hgateshifter)
{
	//this looks weird, but it ensures that our inputs vector contains exactly one item per input
	assert(inputs.size() == CARINPUT::INVALID);
	assert(lastinputs.size() == CARINPUT::INVALID);

	for (size_t n = 0; n < controls.size(); ++n)
	{
		float newval = 0.0;

		for (std::vector <CONTROL>::iterator i = controls[n].begin(); i != controls[n].end(); ++i)
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
					TOGGLE button = eventsystem.GetJoyButton(i->joynum, i->keycode);

					if (i->onetime)
					{
						if (i->pushdown && button.GetImpulseRising())
							tempval = 1.0;
						else if (!i->pushdown && button.GetImpulseFalling())
							tempval = 1.0;
						else
							tempval = 0.0;
						handled = true;
					}
					else
					{
						float downval = 1.0;
						float upval = 0.0;
						if (!i->pushdown)
						{
							downval = 0.0;
							upval = 1.0;
						}

						tempval = Ramp(lastinputs[n], button.GetState() ? downval : upval, button_ramp, dt);
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
					if (i->pushdown && keystate.just_down)
						tempval = 1.0;
					else if (!i->pushdown && keystate.just_up)
						tempval = 1.0;
					else
						tempval = 0.0;
					handled = true;
				}
				else
				{
					float downval = 1.0;
					float upval = 0.0;
					if (!i->pushdown)
					{
						downval = 0.0;
						upval = 1.0;
					}

					//if (inputs[n->first] != keystate.down ? downval : upval) std::cout << "Key ramp: " << i->keycode << ", " << n->first << std::endl;
					tempval = Ramp(lastinputs[n], keystate.down ? downval : upval, button_ramp, dt);

					handled = true;
				}
			}
			else if (i->type == CONTROL::MOUSE)
			{
				//cout << "type mouse" << std::endl;

				if (i->mousetype == CONTROL::MOUSEBUTTON)
				{
					//cout << "mousebutton" << std::endl;

					EVENTSYSTEM_SDL::BUTTON_STATE buttonstate = eventsystem.GetMouseButtonState(i->keycode);

					if (i->onetime)
					{
						if (i->pushdown && buttonstate.just_down)
							tempval = 1.0;
						else if (!i->pushdown && buttonstate.just_up)
							tempval = 1.0;
						else
							tempval = 0.0;
						handled = true;
					}
					else
					{
						float downval = 1.0;
						float upval = 0.0;
						if (!i->pushdown)
						{
							downval = 0.0;
							upval = 1.0;
						}

						tempval = Ramp(lastinputs[n], buttonstate.down ? downval : upval, button_ramp, dt);
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

		inputs[n] = newval;

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

void CARCONTROLMAP_LOCAL::GetControlsInfo(std::map<std::string, std::string> & info) const
{
	for (size_t n = 0; n < CARINPUT::INVALID; ++n)
	{
		CARINPUT::CARINPUT input = CARINPUT::CARINPUT(n);
		const std::string inputstr = GetStringFromInput(input);
		const std::vector<CONTROL> & ct = controls[input];
		for (size_t m = 0; m < ct.size(); ++m)
		{
			std::stringstream s;
			s << "control." << inputstr << "." << m;
			info[s.str()] = ct[m].GetInfo();
		}
		if (ct.size() < max_controls)
		{
			std::stringstream s;
			s << "control." << inputstr << "." << ct.size();
			info[s.str()] = "new";
		}
		for (size_t m = ct.size() + 1; m < max_controls; ++m)
		{
			std::stringstream s;
			s << "control." << inputstr << "." << m;
			info[s.str()] = "";
		}
	}
}

CARCONTROLMAP_LOCAL::CONTROL CARCONTROLMAP_LOCAL::GetControl(const std::string & inputname, size_t controlid)
{
	size_t input = GetInputFromString(inputname);
	if (input == CARINPUT::INVALID)
		return CONTROL();

	std::vector<CONTROL> & input_controls = controls[input];
	if (controlid < input_controls.size())
		return input_controls[controlid];
	else
		return CONTROL();
}

void CARCONTROLMAP_LOCAL::SetControl(const std::string & inputname, size_t controlid, const CONTROL & control)
{
	CARINPUT::CARINPUT input = GetInputFromString(inputname);
	if (input == CARINPUT::INVALID)
		return;

	std::vector<CONTROL> & input_controls = controls[input];
	if (controlid >= input_controls.size())
	{
		input_controls.push_back(control);
		return;
	}

	input_controls[controlid] = control;
}

void CARCONTROLMAP_LOCAL::DeleteControl(const std::string & inputname, size_t controlid)
{
	CARINPUT::CARINPUT input = GetInputFromString(inputname);
	if (input == CARINPUT::INVALID)
		return;

	std::vector<CONTROL> & input_controls = controls[input];
	if (controlid >= input_controls.size())
		return;

	if (controlid < input_controls.size() - 1)
		input_controls[controlid] = input_controls.back();

	input_controls.pop_back();
}

std::map <std::string, CARINPUT::CARINPUT> CARCONTROLMAP_LOCAL::InitCarInputStringMap()
{
	std::map <std::string, CARINPUT::CARINPUT> stringmap;
	for (size_t i = 0; i < carinput_strings.size(); ++i)
	{
		stringmap[carinput_strings[i]] = CARINPUT::CARINPUT(i);
	}
	return stringmap;
}

std::vector<std::string> CARCONTROLMAP_LOCAL::InitCarInputStrings()
{
	std::vector<std::string> strings(CARINPUT::INVALID);
	strings[CARINPUT::THROTTLE] = "gas";
	strings[CARINPUT::NOS] = "nos";
	strings[CARINPUT::BRAKE] = "brake";
	strings[CARINPUT::HANDBRAKE] = "handbrake";
	strings[CARINPUT::CLUTCH] = "clutch";
	strings[CARINPUT::STEER_LEFT] = "steer_left";
	strings[CARINPUT::STEER_RIGHT] = "steer_right";
	strings[CARINPUT::SHIFT_UP] = "disengage_shift_up";
	strings[CARINPUT::SHIFT_DOWN] = "disengage_shift_down";
	strings[CARINPUT::START_ENGINE] = "start_engine";
	strings[CARINPUT::ABS_TOGGLE] = "abs_toggle";
	strings[CARINPUT::TCS_TOGGLE] = "tcs_toggle";
	strings[CARINPUT::NEUTRAL] = "neutral";
	strings[CARINPUT::FIRST_GEAR] = "first_gear";
	strings[CARINPUT::SECOND_GEAR] = "second_gear";
	strings[CARINPUT::THIRD_GEAR] = "third_gear";
	strings[CARINPUT::FOURTH_GEAR] = "fourth_gear";
	strings[CARINPUT::FIFTH_GEAR] = "fifth_gear";
	strings[CARINPUT::SIXTH_GEAR] = "sixth_gear";
	strings[CARINPUT::REVERSE] = "reverse";
	strings[CARINPUT::ROLLOVER_RECOVER] = "rollover_recover";
	strings[CARINPUT::VIEW_REAR] = "rear_view";
	strings[CARINPUT::VIEW_PREV] = "view_prev";
	strings[CARINPUT::VIEW_NEXT] = "view_next";
	strings[CARINPUT::VIEW_HOOD] = "view_hood";
	strings[CARINPUT::VIEW_INCAR] = "view_incar";
	strings[CARINPUT::VIEW_CHASERIGID] = "view_chaserigid";
	strings[CARINPUT::VIEW_CHASE] = "view_chase";
	strings[CARINPUT::VIEW_ORBIT] = "view_orbit";
	strings[CARINPUT::VIEW_FREE] = "view_free";
	strings[CARINPUT::FOCUS_PREV] = "focus_prev_car";
	strings[CARINPUT::FOCUS_NEXT] = "focus_next_car";
	strings[CARINPUT::PAN_LEFT] = "pan_left";
	strings[CARINPUT::PAN_RIGHT] = "pan_right";
	strings[CARINPUT::PAN_UP] = "pan_up";
	strings[CARINPUT::PAN_DOWN] = "pan_down";
	strings[CARINPUT::ZOOM_IN] = "zoom_in";
	strings[CARINPUT::ZOOM_OUT] = "zoom_out";
	strings[CARINPUT::REPLAY_FF] = "replay_ff";
	strings[CARINPUT::REPLAY_RW] = "replay_rw";
	strings[CARINPUT::SCREENSHOT] = "screen_shot";
	strings[CARINPUT::PAUSE] = "pause";
	strings[CARINPUT::RELOAD_SHADERS] = "reload_shaders";
	strings[CARINPUT::RELOAD_GUI] = "reload_gui";
	strings[CARINPUT::GUI_LEFT] = "gui_left";
	strings[CARINPUT::GUI_RIGHT] = "gui_right";
	strings[CARINPUT::GUI_UP] = "gui_up";
	strings[CARINPUT::GUI_DOWN] = "gui_down";
	strings[CARINPUT::GUI_SELECT] = "gui_select";
	strings[CARINPUT::GUI_CANCEL] = "gui_cancel";
	return strings;
}

std::map <std::string, int> CARCONTROLMAP_LOCAL::InitKeycodeStringMap()
{
	std::map <std::string, int> keycodes;
	keycodes["BACKSPACE"] = 8;
	keycodes["TAB"] = 9;
	keycodes["CLEAR"] = 12;
	keycodes["RETURN"] = 13;
	keycodes["PAUSE"] = 19;
	keycodes["ESCAPE"] = 27;
	keycodes["SPACE"] = 32;
	keycodes["EXCLAIM"] = 33;
	keycodes["QUOTEDBL"] = 34;
	keycodes["HASH"] = 35;
	keycodes["DOLLAR"] = 36;
	keycodes["AMPERSAND"] = 38;
	keycodes["QUOTE"] = 39;
	keycodes["LEFTPAREN"] = 40;
	keycodes["RIGHTPAREN"] = 41;
	keycodes["ASTERISK"] = 42;
	keycodes["PLUS"] = 43;
	keycodes["COMMA"] = 44;
	keycodes["MINUS"] = 45;
	keycodes["PERIOD"] = 46;
	keycodes["SLASH"] = 47;
	keycodes["0"] = 48;
	keycodes["1"] = 49;
	keycodes["2"] = 50;
	keycodes["3"] = 51;
	keycodes["4"] = 52;
	keycodes["5"] = 53;
	keycodes["6"] = 54;
	keycodes["7"] = 55;
	keycodes["8"] = 56;
	keycodes["9"] = 57;
	keycodes["COLON"] = 58;
	keycodes["SEMICOLON"] = 59;
	keycodes["LESS"] = 60;
	keycodes["EQUALS"] = 61;
	keycodes["GREATER"] = 62;
	keycodes["QUESTION"] = 63;
	keycodes["AT"] = 64;
	keycodes["LEFTBRACKET"] = 91;
	keycodes["BACKSLASH"] = 92;
	keycodes["RIGHTBRACKET"] = 93;
	keycodes["CARET"] = 94;
	keycodes["UNDERSCORE"] = 95;
	keycodes["BACKQUOTE"] = 96;
	keycodes["a"] = 97;
	keycodes["b"] = 98;
	keycodes["c"] = 99;
	keycodes["d"] = 100;
	keycodes["e"] = 101;
	keycodes["f"] = 102;
	keycodes["g"] = 103;
	keycodes["h"] = 104;
	keycodes["i"] = 105;
	keycodes["j"] = 106;
	keycodes["k"] = 107;
	keycodes["l"] = 108;
	keycodes["m"] = 109;
	keycodes["n"] = 110;
	keycodes["o"] = 111;
	keycodes["p"] = 112;
	keycodes["q"] = 113;
	keycodes["r"] = 114;
	keycodes["s"] = 115;
	keycodes["t"] = 116;
	keycodes["u"] = 117;
	keycodes["v"] = 118;
	keycodes["w"] = 119;
	keycodes["x"] = 120;
	keycodes["y"] = 121;
	keycodes["z"] = 122;
	keycodes["DELETE"] = 127;
	keycodes["KP0"] = 256;
	keycodes["KP1"] = 257;
	keycodes["KP2"] = 258;
	keycodes["KP3"] = 259;
	keycodes["KP4"] = 260;
	keycodes["KP5"] = 261;
	keycodes["KP6"] = 262;
	keycodes["KP7"] = 263;
	keycodes["KP8"] = 264;
	keycodes["KP9"] = 265;
	keycodes["KP_PERIOD"] = 266;
	keycodes["KP_DIVIDE"] = 267;
	keycodes["KP_MULTIPLY"] = 268;
	keycodes["KP_MINUS"] = 269;
	keycodes["KP_PLUS"] = 270;
	keycodes["KP_ENTER"] = 271;
	keycodes["KP_EQUALS"] = 272;
	keycodes["UP"] = 273;
	keycodes["DOWN"] = 274;
	keycodes["RIGHT"] = 275;
	keycodes["LEFT"] = 276;
	keycodes["INSERT"] = 277;
	keycodes["HOME"] = 278;
	keycodes["END"] = 279;
	keycodes["PAGEUP"] = 280;
	keycodes["PAGEDOWN"] = 281;
	keycodes["F1"] = 282;
	keycodes["F2"] = 283;
	keycodes["F3"] = 284;
	keycodes["F4"] = 285;
	keycodes["F5"] = 286;
	keycodes["F6"] = 287;
	keycodes["F7"] = 288;
	keycodes["F8"] = 289;
	keycodes["F9"] = 290;
	keycodes["F10"] = 291;
	keycodes["F11"] = 292;
	keycodes["F12"] = 293;
	keycodes["F13"] = 294;
	keycodes["F14"] = 295;
	keycodes["F15"] = 296;
	keycodes["NUMLOCK"] = 300;
	keycodes["CAPSLOCK"] = 301;
	keycodes["SCROLLOCK"] = 302;
	keycodes["RSHIFT"] = 303;
	keycodes["LSHIFT"] = 304;
	keycodes["RCTRL"] = 305;
	keycodes["LCTRL"] = 306;
	keycodes["RALT"] = 307;
	keycodes["LALT"] = 308;
	keycodes["RMETA"] = 309;
	keycodes["LMETA"] = 310;
	keycodes["LSUPER"] = 311;
	keycodes["RSUPER"] = 312;
	keycodes["ALTGR"] = 313;
	return keycodes;
}

const std::string & CARCONTROLMAP_LOCAL::GetStringFromInput(const CARINPUT::CARINPUT input)
{
	return carinput_strings[input];
}

CARINPUT::CARINPUT CARCONTROLMAP_LOCAL::GetInputFromString(const std::string & str)
{
	std::map <std::string, CARINPUT::CARINPUT>::const_iterator i = carinput_stringmap.find(str);
	if (i != carinput_stringmap.end())
		return i->second;
	
	return CARINPUT::INVALID;
}

const std::string & CARCONTROLMAP_LOCAL::GetStringFromKeycode(const int code)
{
	for (std::map <std::string, int>::const_iterator i = keycode_stringmap.begin(); i != keycode_stringmap.end(); ++i)
		if (i->second == code)
			return i->first;
	
	return invalid;
}

int CARCONTROLMAP_LOCAL::GetKeycodeFromString(const std::string & str)
{
	std::map <std::string, int>::const_iterator i = keycode_stringmap.find(str);
	if (i != keycode_stringmap.end())
		return i->second;
	
	return 0;
}

void CARCONTROLMAP_LOCAL::AddControl(CONTROL newctrl, const std::string & inputname, std::ostream & error_output)
{
	CARINPUT::CARINPUT input = GetInputFromString(inputname);
	if (input != CARINPUT::INVALID)
		controls[input].push_back(newctrl);
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

bool CARCONTROLMAP_LOCAL::CONTROL::IsAnalog() const
{
	return (type == JOY && joytype == JOYAXIS) || (type == MOUSE && mousetype == MOUSEMOTION);
}

std::string CARCONTROLMAP_LOCAL::CONTROL::GetInfo() const
{
	if (type == KEY)
	{
		return GetStringFromKeycode(keycode);
	}

	if (type == JOY)
	{
		std::stringstream s;

		if (joytype == JOYAXIS)
		{
			if (joyaxistype == NEGATIVE)
				s << "-";
			else
				s << "+";
			
			s << "JOY" << joynum << ":AXIS" << joyaxis;
		
			return s.str();
		}

		if (joytype == JOYBUTTON)
		{
			s << "JOY" << joynum << ":BUTTON" << keycode;
			return s.str();
		}

		if (joytype == JOYHAT)
		{
			s << "JOY" << joynum << ":HAT" << keycode;
			return s.str();
		}
	}

	if (type == MOUSE)
	{
		std::stringstream s;
		s << "MOUSE";

		if (mousetype == MOUSEBUTTON)
			s << keycode;
		else if (mdir == UP)
			s << "UP";
		else if (mdir == DOWN)
			s << "DOWN";
		else if (mdir == LEFT)
			s << "LEFT";
		else if (mdir == RIGHT)
			s << "RIGHT";
		
		return s.str();
	}

	return invalid;
}

void CARCONTROLMAP_LOCAL::CONTROL::DebugPrint(std::ostream & out) const
{
	out << type << " " << onetime << " " << pushdown << " " << keycode << " " <<
		joynum << " " << joyaxis << " " << joyaxistype << " " << joytype << " " <<
		mousetype << " " << mdir << " " << last_mouse_state << " " <<
		deadzone << " " << exponent << " " << gain << std::endl;
}

bool CARCONTROLMAP_LOCAL::CONTROL::operator==(const CONTROL & other) const
{
	CONTROL me = *this;
	CONTROL them = other;

	//don't care about certain flags
	me.onetime = 1;
	me.pushdown = 1;
	me.deadzone = 0;
	me.exponent = 1;
	me.gain = 1;
	them.onetime = 1;
	them.pushdown = 1;
	them.deadzone = 0;
	them.exponent = 1;
	them.gain = 1;

	std::stringstream mestr;
	std::stringstream themstr;
	me.DebugPrint(mestr);
	them.DebugPrint(themstr);

	return (mestr.str() == themstr.str());

	/*std::cout << "Checking:" << std::endl;
	me.DebugPrint(std::cout);
	me.MemDump(std::cout);
	them.DebugPrint(std::cout);
	them.MemDump(std::cout);
	std::cout << "Equality check: " << (std::memcmp(&me,&them,sizeof(CONTROL)) == 0) << std::endl;

	return (std::memcmp(&me,&them,sizeof(CONTROL)) == 0);*/

	//bool equality = (type == other.type) && (type == other.type) &&
}

bool CARCONTROLMAP_LOCAL::CONTROL::operator<(const CONTROL & other) const
{
	CONTROL me = *this;
	CONTROL them = other;

	me.onetime = 1;
	me.pushdown = 1;
	me.deadzone = 0;
	me.exponent = 1;
	me.gain = 1;
	them.onetime = 1;
	them.pushdown = 1;
	them.deadzone = 0;
	them.exponent = 1;
	them.gain = 1;

	std::stringstream mestr;
	std::stringstream themstr;
	me.DebugPrint(mestr);
	them.DebugPrint(themstr);

	return (mestr.str() < themstr.str());
}

void CARCONTROLMAP_LOCAL::CONTROL::ReadFrom(std::istream & in)
{
	int newtype, newjoyaxistype, newjoytype, newmousetype, newmdir;
	in >> newtype >> onetime >> pushdown >> keycode >>
		joynum >> joyaxis >> newjoyaxistype >> newjoytype >>
		newmousetype >> newmdir >> 	last_mouse_state >>
		deadzone >> exponent >> gain;
	type = TYPE(newtype);
	joyaxistype = JOYAXISTYPE(newjoyaxistype);
	joytype = JOYTYPE(newjoytype);
	mousetype = MOUSETYPE(newmousetype);
	mdir = MOUSEDIRECTION(newmdir);
}

CARCONTROLMAP_LOCAL::CONTROL::CONTROL() :
	type(UNKNOWN), onetime(true), pushdown(true), keycode(0),
	joynum(0), joyaxis(0), joyaxistype(POSITIVE), joytype(JOYAXIS),
	mousetype(MOUSEBUTTON), mdir(UP), last_mouse_state(false),
	deadzone(0.0), exponent(1.0), gain(1.0)
{}
