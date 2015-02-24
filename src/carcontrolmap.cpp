/************************************************************************/
/*                                                                      */
/* This file is part of VDrift.                                         */
/*                                                                      */
/* VDrift is free software: you can redistribute it and/or modify       */
/* it under the terms of the GNU General Public License as published by */
/* the Free Software Foundation, either version 3 of the License, or    */
/* (at your option) any later version.                                  */
/*                                                                      */
/* VDrift is distributed in the hope that it will be useful,            */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of       */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the        */
/* GNU General Public License for more details.                         */
/*                                                                      */
/* You should have received a copy of the GNU General Public License    */
/* along with VDrift.  If not, see <http://www.gnu.org/licenses/>.      */
/*                                                                      */
/************************************************************************/

#include "carcontrolmap.h"
#include "eventsystem.h"
#include "cfg/config.h"

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

const std::vector<std::string> CarControlMap::carinput_strings = CarControlMap::InitCarInputStrings();

const std::map <std::string, unsigned> CarControlMap::carinput_stringmap = CarControlMap::InitCarInputStringMap();

const std::map <std::string, int> CarControlMap::keycode_stringmap = CarControlMap::InitKeycodeStringMap();

CarControlMap::CarControlMap() :
	controls(GameInput::INVALID)
{
	// constructor
}

bool CarControlMap::Load(const std::string & controlfile, std::ostream & info_output, std::ostream & error_output)
{
	Config controls_config;
	if (!controls_config.load(controlfile))
	{
		info_output << "Failed to load car controls file " << controlfile << std::endl;
		return false;
	}

	controls.clear();
	controls.resize(GameInput::INVALID);
	for (Config::const_iterator i = controls_config.begin(); i != controls_config.end(); ++i)
	{
		std::string type, name;
		if (!controls_config.get(i, "type", type)) continue;
		if (!controls_config.get(i, "name", name)) continue;

		unsigned newinput = GetInputFromString(name);
		if (newinput == GameInput::INVALID)
		{
			error_output << "Unknown input type in section " << i->first << std::endl;
			continue;
		}

		Control newctrl;
		if (type == "joy")
		{
			newctrl.joynum = 0;
			std::string joy_type;
			if (!controls_config.get(i, "joy_index", newctrl.joynum, error_output)) continue;
			if (!controls_config.get(i, "joy_type", joy_type, error_output)) continue;

			newctrl.type = Control::JOY;
			if (joy_type == "button")
			{
				newctrl.joytype = Control::JOYBUTTON;
				newctrl.keycode = 0;
				newctrl.pushdown = false;
				newctrl.onetime = false;
				if (!controls_config.get(i, "joy_button", newctrl.keycode, error_output)) continue;
				if (!controls_config.get(i, "down", newctrl.pushdown, error_output)) continue;
				if (!controls_config.get(i, "once", newctrl.onetime, error_output)) continue;
			}
			else if (joy_type == "axis")
			{
				newctrl.joytype = Control::JOYAXIS;
				int joy_axis = 0;
				std::string axis_type;
				float deadzone = 0.0;
				float exponent = 0.0;
				float gain = 0.0;
				if (!controls_config.get(i, "joy_axis", joy_axis, error_output)) continue;
				if (!controls_config.get(i, "joy_axis_type", axis_type, error_output)) continue;
				if (!controls_config.get(i, "deadzone", deadzone, error_output)) continue;
				if (!controls_config.get(i, "exponent", exponent, error_output)) continue;
				if (!controls_config.get(i, "gain", gain, error_output)) continue;

				newctrl.joyaxis = joy_axis;
				if (axis_type == "positive")
				{
					newctrl.joyaxistype = Control::POSITIVE;
				}
				else if (axis_type == "negative")
				{
					newctrl.joyaxistype = Control::NEGATIVE;
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
			std::string keyname;
			if (!controls_config.get(i, "key", keyname, error_output))
				continue;

			int keycode = GetKeycodeFromString(keyname);
			if (keycode == 0)
			{
				error_output << "Unknown keyname \"" << keyname << "\" parsing controls in section " << i->first << std::endl;
				continue;
			}

			bool key_down = false;
			bool key_once = false;
			if (!controls_config.get(i, "down", key_down, error_output)) continue;

			if (!controls_config.get(i, "once", key_once, error_output)) continue;
			if (keycode != SDLK_UNKNOWN)
			{
				newctrl.keycode = SDL_Keycode(keycode);
			}
			newctrl.pushdown = key_down;
			newctrl.onetime = key_once;
			newctrl.type = Control::KEY;
		}
		else if (type == "mouse")
		{
			newctrl.type = Control::MOUSE;

			std::string mouse_type = "";
			std::string mouse_direction = "";
			controls_config.get(i, "mouse_type", mouse_type );
			if (mouse_type == "button")
			{
				int mouse_btn = 0;
				bool mouse_btn_down = false;
				bool mouse_btn_once = false;
				newctrl.mousetype = Control::MOUSEBUTTON;
				controls_config.get(i, "mouse_button", mouse_btn);
				controls_config.get(i, "down", mouse_btn_down);
				controls_config.get(i, "once", mouse_btn_once);
				newctrl.keycode = mouse_btn;
				newctrl.pushdown = mouse_btn_down;
				newctrl.onetime = mouse_btn_once;
			}
			else if (mouse_type == "motion")
			{
				newctrl.mousetype = Control::MOUSEMOTION;
				controls_config.get(i, "mouse_motion", mouse_direction);
				if (mouse_direction == "left")
				{
					newctrl.mdir = Control::LEFT;
				}
				else if (mouse_direction == "right")
				{
					newctrl.mdir = Control::RIGHT;
				}
				else if (mouse_direction == "up")
				{
					newctrl.mdir = Control::UP;
				}
				else if (mouse_direction == "down")
				{
					newctrl.mdir = Control::DOWN;
				}
				else
				{
					error_output << "Error parsing controls, invalid mouse direction type " << mouse_direction << " in section " << i->first << std::endl;
				}

				newctrl.deadzone=0;
				newctrl.exponent=1;
				newctrl.gain=1;
				controls_config.get(i, "deadzone", newctrl.deadzone);
				controls_config.get(i, "exponent", newctrl.exponent);
				controls_config.get(i, "gain", newctrl.gain);
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

		controls[newinput].push_back(newctrl);
	}

	inputs.resize(GameInput::INVALID, 0.0); //this looks weird, but it initialize all inputs and sets them to zero
	lastinputs.resize(GameInput::INVALID, 0.0); //this looks weird, but it initialize all inputs and sets them to zero
	return true;
}

void CarControlMap::Save(const std::string & controlfile)
{
	Config controls_config;
	Save(controls_config);
	controls_config.write(controlfile);
}

void CarControlMap::Save(Config & controls_config)
{
	int id = 0;
	for (size_t n = 0; n < controls.size(); ++n)
	{
		std::string ctrl_name = GetStringFromInput(CarInput::Enum(n));
		if (ctrl_name.empty())
			continue;

		for (size_t i = 0; i < controls[n].size(); ++i)
		{
			std::ostringstream s;
			s << "control mapping " << std::setfill('0') << std::setw(2) << id;

			Config::iterator section;
			controls_config.get(s.str(), section);
			controls_config.set(section, "name", ctrl_name);

			Control & curctrl = controls[n][i];
			if (curctrl.type == Control::JOY)
			{
				controls_config.set(section, "type", "joy");
				controls_config.set(section, "joy_index", curctrl.joynum);

				if (curctrl.joytype == Control::JOYAXIS)
				{
					controls_config.set(section, "joy_type", "axis");
					controls_config.set(section, "joy_axis", curctrl.joyaxis );
					switch (curctrl.joyaxistype) {
						case Control::POSITIVE:
							controls_config.set(section, "joy_axis_type", "positive");
							break;
						case Control::NEGATIVE:
							controls_config.set(section, "joy_axis_type", "negative");
							break;
					}
					controls_config.set(section, "deadzone", curctrl.deadzone);
					controls_config.set(section, "exponent", curctrl.exponent);
					controls_config.set(section, "gain", curctrl.gain);
				}
				else if (curctrl.joytype == Control::JOYBUTTON)
				{
					controls_config.set(section, "joy_type", "button");
					controls_config.set(section, "joy_button", curctrl.keycode);
					controls_config.set(section, "once", curctrl.onetime);
					controls_config.set(section, "down", curctrl.pushdown);
				}
			}
			else if (curctrl.type == Control::KEY)
			{
				controls_config.set(section, "type", "key");
				std::string keyname = GetStringFromKeycode(curctrl.keycode);
				controls_config.set(section, "key", keyname);
				controls_config.set(section, "once", curctrl.onetime);
				controls_config.set(section, "down", curctrl.pushdown);
			}
			else if (curctrl.type == Control::MOUSE)
			{
				controls_config.set(section, "type", "mouse");
				if (curctrl.mousetype == Control::MOUSEBUTTON)
				{
					controls_config.set(section, "mouse_type", "button");
					controls_config.set(section, "mouse_button", curctrl.keycode );
					controls_config.set(section, "once", curctrl.onetime );
					controls_config.set(section, "down", curctrl.pushdown );
				}
				else if (curctrl.mousetype == Control::MOUSEMOTION)
				{
					std::string direction = "invalid";
					Control::MouseDirectionEnum mdir = curctrl.mdir;
					if ( mdir == Control::UP )
					{
						direction = "up";
					}
					else if ( mdir == Control::DOWN )
					{
						direction = "down";
					}
					else if ( mdir == Control::LEFT )
					{
						direction = "left";
					}
					else if ( mdir == Control::RIGHT )
					{
						direction = "right";
					}

					controls_config.set(section, "mouse_type", "motion");
					controls_config.set(section, "mouse_motion", direction);

					controls_config.set(section, "deadzone", curctrl.deadzone);
					controls_config.set(section, "exponent", curctrl.exponent);
					controls_config.set(section, "gain", curctrl.gain);
				}
			}

			id++;
		}
	}
}

const std::vector <float> & CarControlMap::ProcessInput(
	const std::string & joytype, const EventSystem & eventsystem,
	float dt, bool joy_200, float carms, float speedsens,
	int screenw, int screenh, float button_ramp, bool hgateshifter)
{
	//this looks weird, but it ensures that our inputs vector contains exactly one item per input
	assert(inputs.size() == GameInput::INVALID);
	assert(lastinputs.size() == GameInput::INVALID);

	for (size_t n = 0; n < controls.size(); ++n)
	{
		float newval = 0.0;

		for (std::vector <Control>::iterator i = controls[n].begin(); i != controls[n].end(); ++i)
		{
			bool handled = false;
			float tempval = newval;

			if (i->type == Control::JOY)
			{
				//cout << "type joy" << std::endl;

				if (i->joytype == Control::JOYAXIS)
				{
					float val = eventsystem.GetJoyAxis(i->joynum, i->joyaxis);
					if (i->joyaxistype == Control::NEGATIVE)
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
				else if (i->joytype == Control::JOYBUTTON)
				{
					Toggle button = eventsystem.GetJoyButton(i->joynum, i->keycode);

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
			else if (i->type == Control::KEY)
			{
				//cout << "type key" << std::endl;

				EventSystem::ButtonState keystate = eventsystem.GetKeyState(SDL_Keycode(i->keycode));

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
			else if (i->type == Control::MOUSE)
			{
				//cout << "type mouse" << std::endl;

				if (i->mousetype == Control::MOUSEBUTTON)
				{
					//cout << "mousebutton" << std::endl;

					EventSystem::ButtonState buttonstate = eventsystem.GetMouseButtonState(i->keycode);

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
				else if (i->mousetype == Control::MOUSEMOTION)
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

					if (i->mdir == Control::UP)
						val = -yval;
					else if (i->mdir == Control::DOWN)
						val = yval;
					else if (i->mdir == Control::LEFT)
						val = -xval;
					else if (i->mdir == Control::RIGHT)
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
		bool havegear = inputs[CarInput::FIRST_GEAR] ||
				inputs[CarInput::SECOND_GEAR] ||
				inputs[CarInput::THIRD_GEAR] ||
				inputs[CarInput::FOURTH_GEAR] ||
				inputs[CarInput::FIFTH_GEAR] ||
				inputs[CarInput::SIXTH_GEAR] ||
				inputs[CarInput::REVERSE];
		if (!havegear)
			inputs[CarInput::NEUTRAL] = 1.0;
	}

	float steerpos = lastinputs[CarInput::STEER_RIGHT] - lastinputs[CarInput::STEER_LEFT];

	lastinputs = inputs;

	//do steering processing
	ProcessSteering(joytype, steerpos, dt, joy_200, carms*2.23693629, speedsens);

	return inputs;
}

void CarControlMap::GetControlsInfo(std::map<std::string, std::string> & info) const
{
	for (size_t n = 0; n < GameInput::INVALID; ++n)
	{
		CarInput::Enum input = CarInput::Enum(n);
		const std::string inputstr = GetStringFromInput(input);
		const std::vector<Control> & ct = controls[input];
		for (size_t m = 0; m < ct.size(); ++m)
		{
			std::ostringstream s;
			s << "control." << inputstr << "." << m;
			info[s.str()] = ct[m].GetInfo();
		}
		if (ct.size() < max_controls)
		{
			std::ostringstream s;
			s << "control." << inputstr << "." << ct.size();
			info[s.str()] = "new";
		}
		for (size_t m = ct.size() + 1; m < max_controls; ++m)
		{
			std::ostringstream s;
			s << "control." << inputstr << "." << m;
			info[s.str()] = "";
		}
	}
}

CarControlMap::Control CarControlMap::GetControl(const std::string & inputname, size_t controlid)
{
	size_t input = GetInputFromString(inputname);
	if (input == GameInput::INVALID)
		return Control();

	std::vector<Control> & input_controls = controls[input];
	if (controlid < input_controls.size())
		return input_controls[controlid];
	else
		return Control();
}

void CarControlMap::SetControl(const std::string & inputname, size_t controlid, const Control & control)
{
	unsigned input = GetInputFromString(inputname);
	if (input == GameInput::INVALID)
		return;

	std::vector<Control> & input_controls = controls[input];
	if (controlid >= input_controls.size())
	{
		input_controls.push_back(control);
		return;
	}

	input_controls[controlid] = control;
}

void CarControlMap::DeleteControl(const std::string & inputname, size_t controlid)
{
	unsigned input = GetInputFromString(inputname);
	if (input == GameInput::INVALID)
		return;

	std::vector<Control> & input_controls = controls[input];
	if (controlid >= input_controls.size())
		return;

	if (controlid < input_controls.size() - 1)
		input_controls[controlid] = input_controls.back();

	input_controls.pop_back();
}

std::map <std::string, unsigned> CarControlMap::InitCarInputStringMap()
{
	std::map <std::string, unsigned> stringmap;
	for (size_t i = 0; i < carinput_strings.size(); ++i)
	{
		stringmap[carinput_strings[i]] = CarInput::Enum(i);
	}
	return stringmap;
}

std::vector<std::string> CarControlMap::InitCarInputStrings()
{
	std::vector<std::string> strings(GameInput::INVALID);
	strings[CarInput::THROTTLE] = "gas";
	strings[CarInput::NOS] = "nos";
	strings[CarInput::BRAKE] = "brake";
	strings[CarInput::HANDBRAKE] = "handbrake";
	strings[CarInput::CLUTCH] = "clutch";
	strings[CarInput::STEER_LEFT] = "steer_left";
	strings[CarInput::STEER_RIGHT] = "steer_right";
	strings[CarInput::SHIFT_UP] = "disengage_shift_up";
	strings[CarInput::SHIFT_DOWN] = "disengage_shift_down";
	strings[CarInput::START_ENGINE] = "start_engine";
	strings[CarInput::ABS_TOGGLE] = "abs_toggle";
	strings[CarInput::TCS_TOGGLE] = "tcs_toggle";
	strings[CarInput::NEUTRAL] = "neutral";
	strings[CarInput::FIRST_GEAR] = "first_gear";
	strings[CarInput::SECOND_GEAR] = "second_gear";
	strings[CarInput::THIRD_GEAR] = "third_gear";
	strings[CarInput::FOURTH_GEAR] = "fourth_gear";
	strings[CarInput::FIFTH_GEAR] = "fifth_gear";
	strings[CarInput::SIXTH_GEAR] = "sixth_gear";
	strings[CarInput::REVERSE] = "reverse";
	strings[CarInput::ROLLOVER] = "rollover_recover";
	strings[GameInput::VIEW_REAR] = "rear_view";
	strings[GameInput::VIEW_PREV] = "view_prev";
	strings[GameInput::VIEW_NEXT] = "view_next";
	strings[GameInput::VIEW_HOOD] = "view_hood";
	strings[GameInput::VIEW_INCAR] = "view_incar";
	strings[GameInput::VIEW_CHASERIGID] = "view_chaserigid";
	strings[GameInput::VIEW_CHASE] = "view_chase";
	strings[GameInput::VIEW_ORBIT] = "view_orbit";
	strings[GameInput::VIEW_FREE] = "view_free";
	strings[GameInput::FOCUS_PREV] = "focus_prev_car";
	strings[GameInput::FOCUS_NEXT] = "focus_next_car";
	strings[GameInput::PAN_LEFT] = "pan_left";
	strings[GameInput::PAN_RIGHT] = "pan_right";
	strings[GameInput::PAN_UP] = "pan_up";
	strings[GameInput::PAN_DOWN] = "pan_down";
	strings[GameInput::ZOOM_IN] = "zoom_in";
	strings[GameInput::ZOOM_OUT] = "zoom_out";
	strings[GameInput::REPLAY_FF] = "replay_ff";
	strings[GameInput::REPLAY_RW] = "replay_rw";
	strings[GameInput::SCREENSHOT] = "screen_shot";
	strings[GameInput::PAUSE] = "pause";
	strings[GameInput::RELOAD_SHADERS] = "reload_shaders";
	strings[GameInput::RELOAD_GUI] = "reload_gui";
	strings[GameInput::GUI_LEFT] = "gui_left";
	strings[GameInput::GUI_RIGHT] = "gui_right";
	strings[GameInput::GUI_UP] = "gui_up";
	strings[GameInput::GUI_DOWN] = "gui_down";
	strings[GameInput::GUI_SELECT] = "gui_select";
	strings[GameInput::GUI_CANCEL] = "gui_cancel";
	return strings;
}

std::map <std::string, int> CarControlMap::InitKeycodeStringMap()
{
	std::map <std::string, int> keycodes;
	keycodes["UNKNOWN"] = SDLK_UNKNOWN;
	keycodes["BACKSPACE"] = SDLK_BACKSPACE;
	keycodes["TAB"] = SDLK_TAB;
	keycodes["CLEAR"] = SDLK_CLEAR;
	keycodes["RETURN"] = SDLK_RETURN;
	keycodes["PAUSE"] = SDLK_PAUSE;
	keycodes["ESCAPE"] = SDLK_ESCAPE;
	keycodes["SPACE"] = SDLK_SPACE;
	keycodes["EXCLAIM"] = SDLK_EXCLAIM;
	keycodes["QUOTEDBL"] = SDLK_QUOTEDBL;
	keycodes["HASH"] = SDLK_HASH;
	keycodes["DOLLAR"] = SDLK_DOLLAR;
	keycodes["AMPERSAND"] = SDLK_AMPERSAND;
	keycodes["QUOTE"] = SDLK_QUOTE;
	keycodes["LEFTPAREN"] = SDLK_LEFTPAREN;
	keycodes["RIGHTPAREN"] = SDLK_RIGHTPAREN;
	keycodes["ASTERISK"] = SDLK_ASTERISK;
	keycodes["PLUS"] = SDLK_PLUS;
	keycodes["COMMA"] = SDLK_COMMA;
	keycodes["MINUS"] = SDLK_MINUS;
	keycodes["PERIOD"] = SDLK_PERIOD;
	keycodes["SLASH"] = SDLK_SLASH;
	keycodes["0"] = SDLK_0;
	keycodes["1"] = SDLK_1;
	keycodes["2"] = SDLK_2;
	keycodes["3"] = SDLK_3;
	keycodes["4"] = SDLK_4;
	keycodes["5"] = SDLK_5;
	keycodes["6"] = SDLK_6;
	keycodes["7"] = SDLK_7;
	keycodes["8"] = SDLK_8;
	keycodes["9"] = SDLK_9;
	keycodes["COLON"] = SDLK_COLON;
	keycodes["SEMICOLON"] = SDLK_SEMICOLON;
	keycodes["LESS"] = SDLK_LESS;
	keycodes["EQUALS"] = SDLK_EQUALS;
	keycodes["GREATER"] = SDLK_GREATER;
	keycodes["QUESTION"] = SDLK_QUESTION;
	keycodes["AT"] = SDLK_AT;
	keycodes["LEFTBRACKET"] = SDLK_LEFTBRACKET;
	keycodes["BACKSLASH"] = SDLK_BACKSLASH;
	keycodes["RIGHTBRACKET"] = SDLK_RIGHTBRACKET;
	keycodes["CARET"] = SDLK_CARET;
	keycodes["UNDERSCORE"] = SDLK_UNDERSCORE;
	keycodes["BACKQUOTE"] = SDLK_BACKQUOTE;
	keycodes["a"] = SDLK_a;
	keycodes["b"] = SDLK_b;
	keycodes["c"] = SDLK_c;
	keycodes["d"] = SDLK_d;
	keycodes["e"] = SDLK_e;
	keycodes["f"] = SDLK_f;
	keycodes["g"] = SDLK_g;
	keycodes["h"] = SDLK_h;
	keycodes["i"] = SDLK_i;
	keycodes["j"] = SDLK_j;
	keycodes["k"] = SDLK_k;
	keycodes["l"] = SDLK_l;
	keycodes["m"] = SDLK_m;
	keycodes["n"] = SDLK_n;
	keycodes["o"] = SDLK_o;
	keycodes["p"] = SDLK_p;
	keycodes["q"] = SDLK_q;
	keycodes["r"] = SDLK_r;
	keycodes["s"] = SDLK_s;
	keycodes["t"] = SDLK_t;
	keycodes["u"] = SDLK_u;
	keycodes["v"] = SDLK_v;
	keycodes["w"] = SDLK_w;
	keycodes["x"] = SDLK_x;
	keycodes["y"] = SDLK_y;
	keycodes["z"] = SDLK_z;
	keycodes["DELETE"] = SDLK_DELETE;
	keycodes["KP_PERIOD"] = SDLK_KP_PERIOD;
	keycodes["KP_DIVIDE"] = SDLK_KP_DIVIDE;
	keycodes["KP_MULTIPLY"] = SDLK_KP_MULTIPLY;
	keycodes["KP_MINUS"] = SDLK_KP_MINUS;
	keycodes["KP_PLUS"] = SDLK_KP_PLUS;
	keycodes["KP_ENTER"] = SDLK_KP_ENTER;
	keycodes["KP_EQUALS"] = SDLK_KP_EQUALS;
	keycodes["UP"] = SDLK_UP;
	keycodes["DOWN"] = SDLK_DOWN;
	keycodes["RIGHT"] = SDLK_RIGHT;
	keycodes["LEFT"] = SDLK_LEFT;
	keycodes["INSERT"] = SDLK_INSERT;
	keycodes["HOME"] = SDLK_HOME;
	keycodes["END"] = SDLK_END;
	keycodes["PAGEUP"] = SDLK_PAGEUP;
	keycodes["PAGEDOWN"] = SDLK_PAGEDOWN;
	keycodes["F1"] = SDLK_F1;
	keycodes["F2"] = SDLK_F2;
	keycodes["F3"] = SDLK_F3;
	keycodes["F4"] = SDLK_F4;
	keycodes["F5"] = SDLK_F5;
	keycodes["F6"] = SDLK_F6;
	keycodes["F7"] = SDLK_F7;
	keycodes["F8"] = SDLK_F8;
	keycodes["F9"] = SDLK_F9;
	keycodes["F10"] = SDLK_F10;
	keycodes["F11"] = SDLK_F11;
	keycodes["F12"] = SDLK_F12;
	keycodes["F13"] = SDLK_F13;
	keycodes["F14"] = SDLK_F14;
	keycodes["F15"] = SDLK_F15;
	keycodes["MENU"] = SDLK_MENU;
	keycodes["CAPSLOCK"] = SDLK_CAPSLOCK;
	keycodes["RSHIFT"] = SDLK_RSHIFT;
	keycodes["LSHIFT"] = SDLK_LSHIFT;
	keycodes["RCTRL"] = SDLK_RCTRL;
	keycodes["LCTRL"] = SDLK_LCTRL;
	keycodes["RALT"] = SDLK_RALT;
	keycodes["LALT"] = SDLK_LALT;
	keycodes["KP0"] = SDLK_KP_0;
	keycodes["KP1"] = SDLK_KP_1;
	keycodes["KP2"] = SDLK_KP_2;
	keycodes["KP3"] = SDLK_KP_3;
	keycodes["KP4"] = SDLK_KP_4;
	keycodes["KP5"] = SDLK_KP_5;
	keycodes["KP6"] = SDLK_KP_6;
	keycodes["KP7"] = SDLK_KP_7;
	keycodes["KP8"] = SDLK_KP_8;
	keycodes["KP9"] = SDLK_KP_9;
	keycodes["COMPOSE"] = SDLK_APPLICATION;
	keycodes["NUMLOCK"] = SDLK_NUMLOCKCLEAR;
	keycodes["SCROLLLOCK"] = SDLK_SCROLLLOCK;
	keycodes["RMETA"] = SDLK_RGUI;
	keycodes["LMETA"] = SDLK_LGUI;
	keycodes["LSUPER"] = SDLK_LGUI;
	keycodes["RSUPER"] = SDLK_RGUI;
	return keycodes;
}

const std::string & CarControlMap::GetStringFromInput(const unsigned input)
{
	return carinput_strings[input];
}

unsigned CarControlMap::GetInputFromString(const std::string & str)
{
	std::map <std::string, unsigned>::const_iterator i = carinput_stringmap.find(str);
	if (i != carinput_stringmap.end())
		return i->second;

	return GameInput::INVALID;
}

const std::string & CarControlMap::GetStringFromKeycode(const int code)
{
	for (std::map <std::string, int>::const_iterator i = keycode_stringmap.begin(); i != keycode_stringmap.end(); ++i)
		if (i->second == code)
			return i->first;

	return invalid;
}

int CarControlMap::GetKeycodeFromString(const std::string & str)
{
	std::map <std::string, int>::const_iterator i = keycode_stringmap.find(str);
	if (i != keycode_stringmap.end())
		return i->second;

	return 0;
}

void CarControlMap::AddControl(Control newctrl, const std::string & inputname, std::ostream & error_output)
{
	unsigned input = GetInputFromString(inputname);
	if (input != GameInput::INVALID)
		controls[input].push_back(newctrl);
	else
		error_output << "Input named " << inputname << " couldn't be assigned because it isn't used" << std::endl;
}

void CarControlMap::ProcessSteering(const std::string & joytype, float steerpos, float dt, bool joy_200, float carmph, float speedsens)
{
	//std::cout << "steerpos: " << steerpos << std::endl;

	float val = inputs[CarInput::STEER_RIGHT];
	if (std::abs(inputs[CarInput::STEER_LEFT]) > std::abs(inputs[CarInput::STEER_RIGHT])) //use whichever control is larger
		val = -inputs[CarInput::STEER_LEFT];

	/*if (val != 0)
	{
		std::cout << "Initial steer left: " << inputs[CarInput::STEER_LEFT] << std::endl;
		std::cout << "Initial steer right: " << inputs[CarInput::STEER_RIGHT] << std::endl;
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
		//if (i->first == inputs[CarInput::STEER_LEFT])
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

	//std::cout << "Steer left: " << inputs[CarInput::STEER_LEFT] << std::endl;
	//std::cout << "Steer right: " << inputs[CarInput::STEER_RIGHT] << std::endl;
	//std::cout << "Current steering: " << car.GetLastSteer() << std::endl;
	//std::cout << "New steering: " << val << std::endl;

	inputs[CarInput::STEER_LEFT] = 0.0;
	inputs[CarInput::STEER_RIGHT] = 0.0;
	if (val < 0)
		inputs[CarInput::STEER_LEFT] = -val;
	else
		inputs[CarInput::STEER_RIGHT] = val;
	//inputs[CarInput::STEER_RIGHT] = val;
	//inputs[CarInput::STEER_LEFT] = -val;

	/*if (val != 0)
	{
		std::cout << "Steer left: " << inputs[CarInput::STEER_LEFT] << std::endl;
		std::cout << "Steer right: " << inputs[CarInput::STEER_RIGHT] << std::endl;
	}*/
}

bool CarControlMap::Control::IsAnalog() const
{
	return (type == JOY && joytype == JOYAXIS) || (type == MOUSE && mousetype == MOUSEMOTION);
}

std::string CarControlMap::Control::GetInfo() const
{
	if (type == KEY)
	{
		return GetStringFromKeycode(keycode);
	}

	if (type == JOY)
	{
		std::ostringstream s;

		if (joytype == JOYAXIS)
		{
			s << "JOY" << joynum << "AXIS" << joyaxis;

			if (joyaxistype == NEGATIVE)
				s << "-";
			else
				s << "+";

			return s.str();
		}

		if (joytype == JOYBUTTON)
		{
			s << "JOY" << joynum << "BTN" << keycode;
			return s.str();
		}

		if (joytype == JOYHAT)
		{
			s << "JOY" << joynum << "HAT" << keycode;
			return s.str();
		}
	}

	if (type == MOUSE)
	{
		std::ostringstream s;
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

void CarControlMap::Control::DebugPrint(std::ostream & out) const
{
	out << type << " " << onetime << " " << pushdown << " " << keycode << " " <<
		joynum << " " << joyaxis << " " << joyaxistype << " " << joytype << " " <<
		mousetype << " " << mdir << " " << last_mouse_state << " " <<
		deadzone << " " << exponent << " " << gain << std::endl;
}

bool CarControlMap::Control::operator==(const Control & other) const
{
	Control me = *this;
	Control them = other;

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

	std::ostringstream mestr;
	std::ostringstream themstr;
	me.DebugPrint(mestr);
	them.DebugPrint(themstr);

	return (mestr.str() == themstr.str());
}

bool CarControlMap::Control::operator<(const Control & other) const
{
	Control me = *this;
	Control them = other;

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

	std::ostringstream mestr;
	std::ostringstream themstr;
	me.DebugPrint(mestr);
	them.DebugPrint(themstr);

	return (mestr.str() < themstr.str());
}

void CarControlMap::Control::ReadFrom(std::istream & in)
{
	int newtype, newjoyaxistype, newjoytype, newmousetype, newmdir;
	in >> newtype >> onetime >> pushdown >> keycode >>
		joynum >> joyaxis >> newjoyaxistype >> newjoytype >>
		newmousetype >> newmdir >> 	last_mouse_state >>
		deadzone >> exponent >> gain;
	type = TypeEnum(newtype);
	joyaxistype = JoyAxisEnum(newjoyaxistype);
	joytype = JoyTypeEnum(newjoytype);
	mousetype = MouseTypeEnum(newmousetype);
	mdir = MouseDirectionEnum(newmdir);
}

CarControlMap::Control::Control() :
	type(UNKNOWN), onetime(true), pushdown(false), keycode(0),
	joynum(0), joyaxis(0), joyaxistype(POSITIVE), joytype(JOYAXIS),
	mousetype(MOUSEBUTTON), mdir(UP), last_mouse_state(false),
	deadzone(0.0), exponent(1.0), gain(1.0)
{}
