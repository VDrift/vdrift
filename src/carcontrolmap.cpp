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
#include "minmax.h"

#include <unordered_map>
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
	float sign = 0.3f;
	if (end < start)
		sign = -1.2f;
	if (button_ramp > 0)
		cur += button_ramp*dt*sign;

	//std::cout << "start: " << start << ", end: " << end << ", cur: " << cur << ", increment: "  << button_ramp*dt*sign << std::endl;
	return Clamp(cur, 0.0f, 1.0f);
}

static inline float ApplyDeadzone(float dz, float val)
{
	if (std::abs(val) < dz)
		val = 0;
	else
	{
		if (val < 0)
			val = (val + dz) * (1 / (1 - dz));
		else
			val = (val - dz) * (1 / (1 - dz));
	}
	return val;
}

static inline float ApplyGain(float gain, float val)
{
	return Clamp(val * gain, -1.0f, 1.0f);
}

static inline float ApplyExponent(float exponent, float val)
{
	return Clamp(std::pow(val, exponent), -1.0f, 1.0f);
}

static const std::string invalid("INVALID");

static const std::vector<std::string> carinput_strings
{
	"gas", // THROTTLE
	"nos", // NOS
	"brake", // BRAKE
	"handbrake", // HANDBRAKE
	"clutch", // CLUTCH
	"steer_left", // STEER_LEFT
	"steer_right", // STEER_RIGHT
	"disengage_shift_up", // SHIFT_UP
	"disengage_shift_down", // SHIFT_DOWN
	"start_engine", // START_ENGINE
	"abs_toggle", // ABS_TOGGLE
	"tcs_toggle", // TCS_TOGGLE
	"neutral", // NEUTRAL
	"first_gear", // FIRST_GEAR
	"second_gear", // SECOND_GEAR
	"third_gear", // THIRD_GEAR
	"fourth_gear", // FOURTH_GEAR
	"fifth_gear", // FIFTH_GEAR
	"sixth_gear", // SIXTH_GEAR
	"reverse", // REVERSE
	"rollover_recover", // ROLLOVER
	"rear_view", // VIEW_REAR
	"view_prev", // VIEW_PREV
	"view_next", // VIEW_NEXT
	"view_hood", // VIEW_HOOD
	"view_incar", // VIEW_INCAR
	"view_chaserigid", // VIEW_CHASERIGID
	"view_chase", // VIEW_CHASE
	"view_orbit", // VIEW_ORBIT
	"view_free", // VIEW_FREE
	"focus_prev_car", // FOCUS_PREV
	"focus_next_car", // FOCUS_NEXT
	"pan_left", // PAN_LEFT
	"pan_right", // PAN_RIGHT
	"pan_up", // PAN_UP
	"pan_down", // PAN_DOWN
	"zoom_in", // ZOOM_IN
	"zoom_out", // ZOOM_OUT
	"replay_ff", // REPLAY_FF
	"replay_rw", // REPLAY_RW
	"screen_shot", // SCREENSHOT
	"pause", // PAUSE
	"reload_shaders", // RELOAD_SHADERS
	"reload_gui", // RELOAD_GUI
	"gui_left", // GUI_LEFT
	"gui_right", // GUI_RIGHT
	"gui_up", // GUI_UP
	"gui_down", // GUI_DOWN
	"gui_select", // GUI_SELECT
	"gui_cancel", // GUI_CANCEL
};

static const std::unordered_map<std::string, unsigned> carinput_stringmap = []
{
	std::unordered_map<std::string, unsigned> stringmap(carinput_strings.size());
	for (size_t i = 0; i < carinput_strings.size(); ++i)
	{
		stringmap[carinput_strings[i]] = i;
	}
	return stringmap;
}();

/// map legacy key names from older vdrift releases to keycodes
static const std::unordered_map<std::string, int> keycode_stringmap
{
	{"UNKNOWN", SDLK_UNKNOWN},
	{"BACKSPACE", SDLK_BACKSPACE},
	{"TAB", SDLK_TAB},
	{"CLEAR", SDLK_CLEAR},
	{"RETURN", SDLK_RETURN},
	{"PAUSE", SDLK_PAUSE},
	{"ESCAPE", SDLK_ESCAPE},
	{"SPACE", SDLK_SPACE},
	{"EXCLAIM", SDLK_EXCLAIM},
	{"QUOTEDBL", SDLK_QUOTEDBL},
	{"HASH", SDLK_HASH},
	{"DOLLAR", SDLK_DOLLAR},
	{"AMPERSAND", SDLK_AMPERSAND},
	{"QUOTE", SDLK_QUOTE},
	{"LEFTPAREN", SDLK_LEFTPAREN},
	{"RIGHTPAREN", SDLK_RIGHTPAREN},
	{"ASTERISK", SDLK_ASTERISK},
	{"PLUS", SDLK_PLUS},
	{"COMMA", SDLK_COMMA},
	{"MINUS", SDLK_MINUS},
	{"PERIOD", SDLK_PERIOD},
	{"SLASH", SDLK_SLASH},
	{"0", SDLK_0},
	{"1", SDLK_1},
	{"2", SDLK_2},
	{"3", SDLK_3},
	{"4", SDLK_4},
	{"5", SDLK_5},
	{"6", SDLK_6},
	{"7", SDLK_7},
	{"8", SDLK_8},
	{"9", SDLK_9},
	{"COLON", SDLK_COLON},
	{"SEMICOLON", SDLK_SEMICOLON},
	{"LESS", SDLK_LESS},
	{"EQUALS", SDLK_EQUALS},
	{"GREATER", SDLK_GREATER},
	{"QUESTION", SDLK_QUESTION},
	{"AT", SDLK_AT},
	{"LEFTBRACKET", SDLK_LEFTBRACKET},
	{"BACKSLASH", SDLK_BACKSLASH},
	{"RIGHTBRACKET", SDLK_RIGHTBRACKET},
	{"CARET", SDLK_CARET},
	{"UNDERSCORE", SDLK_UNDERSCORE},
	{"BACKQUOTE", SDLK_BACKQUOTE},
	{"a", SDLK_a},
	{"b", SDLK_b},
	{"c", SDLK_c},
	{"d", SDLK_d},
	{"e", SDLK_e},
	{"f", SDLK_f},
	{"g", SDLK_g},
	{"h", SDLK_h},
	{"i", SDLK_i},
	{"j", SDLK_j},
	{"k", SDLK_k},
	{"l", SDLK_l},
	{"m", SDLK_m},
	{"n", SDLK_n},
	{"o", SDLK_o},
	{"p", SDLK_p},
	{"q", SDLK_q},
	{"r", SDLK_r},
	{"s", SDLK_s},
	{"t", SDLK_t},
	{"u", SDLK_u},
	{"v", SDLK_v},
	{"w", SDLK_w},
	{"x", SDLK_x},
	{"y", SDLK_y},
	{"z", SDLK_z},
	{"DELETE", SDLK_DELETE},
	{"KP_PERIOD", SDLK_KP_PERIOD},
	{"KP_DIVIDE", SDLK_KP_DIVIDE},
	{"KP_MULTIPLY", SDLK_KP_MULTIPLY},
	{"KP_MINUS", SDLK_KP_MINUS},
	{"KP_PLUS", SDLK_KP_PLUS},
	{"KP_ENTER", SDLK_KP_ENTER},
	{"KP_EQUALS", SDLK_KP_EQUALS},
	{"UP", SDLK_UP},
	{"DOWN", SDLK_DOWN},
	{"RIGHT", SDLK_RIGHT},
	{"LEFT", SDLK_LEFT},
	{"INSERT", SDLK_INSERT},
	{"HOME", SDLK_HOME},
	{"END", SDLK_END},
	{"PAGEUP", SDLK_PAGEUP},
	{"PAGEDOWN", SDLK_PAGEDOWN},
	{"F1", SDLK_F1},
	{"F2", SDLK_F2},
	{"F3", SDLK_F3},
	{"F4", SDLK_F4},
	{"F5", SDLK_F5},
	{"F6", SDLK_F6},
	{"F7", SDLK_F7},
	{"F8", SDLK_F8},
	{"F9", SDLK_F9},
	{"F10", SDLK_F10},
	{"F11", SDLK_F11},
	{"F12", SDLK_F12},
	{"F13", SDLK_F13},
	{"F14", SDLK_F14},
	{"F15", SDLK_F15},
	{"MENU", SDLK_MENU},
	{"CAPSLOCK", SDLK_CAPSLOCK},
	{"RSHIFT", SDLK_RSHIFT},
	{"LSHIFT", SDLK_LSHIFT},
	{"RCTRL", SDLK_RCTRL},
	{"LCTRL", SDLK_LCTRL},
	{"RALT", SDLK_RALT},
	{"LALT", SDLK_LALT},
	{"KP0", SDLK_KP_0},
	{"KP1", SDLK_KP_1},
	{"KP2", SDLK_KP_2},
	{"KP3", SDLK_KP_3},
	{"KP4", SDLK_KP_4},
	{"KP5", SDLK_KP_5},
	{"KP6", SDLK_KP_6},
	{"KP7", SDLK_KP_7},
	{"KP8", SDLK_KP_8},
	{"KP9", SDLK_KP_9},
	{"COMPOSE", SDLK_APPLICATION},
	{"NUMLOCK", SDLK_NUMLOCKCLEAR},
	{"SCROLLLOCK", SDLK_SCROLLLOCK},
	{"RMETA", SDLK_RGUI},
	{"LMETA", SDLK_LGUI},
	{"LSUPER", SDLK_LGUI},
	{"RSUPER", SDLK_RGUI},
};

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
	for (auto i = controls_config.begin(); i != controls_config.end(); ++i)
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
			std::string joy_type;
			int device = 0;
			if (!controls_config.get(i, "joy_index", device, error_output)) continue;
			newctrl.device = (unsigned char) device;
			if (!controls_config.get(i, "joy_type", joy_type, error_output)) continue;

			if (joy_type == "button")
			{
				newctrl.type = Control::BUTTON;
				newctrl.id = 0;
				newctrl.pushdown = false;
				newctrl.onetime = false;
				if (!controls_config.get(i, "joy_button", newctrl.id, error_output)) continue;
				if (!controls_config.get(i, "down", newctrl.pushdown, error_output)) continue;
				if (!controls_config.get(i, "once", newctrl.onetime, error_output)) continue;
			}
			else if (joy_type == "axis")
			{
				newctrl.type = Control::AXIS;
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

				newctrl.id = joy_axis;
				if (axis_type == "positive")
				{
					newctrl.negative = false;
				}
				else if (axis_type == "negative")
				{
					newctrl.negative = true;
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
				newctrl.id = SDL_Keycode(keycode);
			}
			newctrl.pushdown = key_down;
			newctrl.onetime = key_once;
			newctrl.device = Control::KEYBOARD;
			newctrl.type = Control::BUTTON;
		}
		else if (type == "mouse")
		{
			newctrl.device = Control::MOUSE;

			std::string type;
			controls_config.get(i, "mouse_type", type);
			if (type == "button")
			{
				newctrl.type = Control::BUTTON;
				int mouse_btn = 0;
				bool mouse_btn_down = false;
				bool mouse_btn_once = false;
				controls_config.get(i, "mouse_button", mouse_btn);
				controls_config.get(i, "down", mouse_btn_down);
				controls_config.get(i, "once", mouse_btn_once);
				newctrl.id = mouse_btn;
				newctrl.pushdown = mouse_btn_down;
				newctrl.onetime = mouse_btn_once;
			}
			else if (type == "motion")
			{
				newctrl.type = Control::AXIS;

				std::string mouse_direction;
				controls_config.get(i, "mouse_motion", mouse_direction);
				if (mouse_direction == "left")
				{
					newctrl.id = Control::MOUSEX;
					newctrl.negative = true;
				}
				else if (mouse_direction == "right")
				{
					newctrl.id = Control::MOUSEX;
					newctrl.negative = false;
				}
				else if (mouse_direction == "up")
				{
					newctrl.id = Control::MOUSEY;
					newctrl.negative = true;
				}
				else if (mouse_direction == "down")
				{
					newctrl.id = Control::MOUSEY;
					newctrl.negative = false;
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
				error_output << "Error parsing controls, invalid mouse type " << type << " in section " << i->first << std::endl;
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
		std::string control_name = GetStringFromInput(CarInput::Enum(n));
		if (control_name.empty())
			continue;

		for (auto & control : controls[n])
		{
			std::ostringstream s;
			s << "control mapping " << std::setfill('0') << std::setw(2) << id;

			Config::iterator section;
			controls_config.get(s.str(), section);
			controls_config.set(section, "name", control_name);
			if (control.device < Control::JOYSTICKS)
			{
				controls_config.set(section, "type", "joy");
				controls_config.set(section, "joy_index", (unsigned)control.device);

				if (control.type == Control::AXIS)
				{
					controls_config.set(section, "joy_type", "axis");
					controls_config.set(section, "joy_axis", control.id);
					controls_config.set(section, "joy_axis_type", control.negative ? "negative" : "positive");
					controls_config.set(section, "deadzone", control.deadzone);
					controls_config.set(section, "exponent", control.exponent);
					controls_config.set(section, "gain", control.gain);
				}
				else if (control.type == Control::BUTTON)
				{
					controls_config.set(section, "joy_type", "button");
					controls_config.set(section, "joy_button", control.id);
					controls_config.set(section, "once", control.onetime);
					controls_config.set(section, "down", control.pushdown);
				}
			}
			else if (control.device == Control::KEYBOARD)
			{
				controls_config.set(section, "type", "key");
				std::string keyname = GetStringFromKeycode(control.id);
				controls_config.set(section, "key", keyname);
				controls_config.set(section, "once", control.onetime);
				controls_config.set(section, "down", control.pushdown);
			}
			else if (control.device == Control::MOUSE)
			{
				controls_config.set(section, "type", "mouse");
				if (control.type == Control::BUTTON)
				{
					controls_config.set(section, "mouse_type", "button");
					controls_config.set(section, "mouse_button", control.id);
					controls_config.set(section, "once", control.onetime);
					controls_config.set(section, "down", control.pushdown);
				}
				else if (control.type == Control::AXIS)
				{
					std::string direction = "invalid";
					if (control.id == Control::MOUSEY)
					{
						direction = control.negative ? "up" : "down";
					}
					else if (control.id == Control::MOUSEX)
					{
						direction = control.negative ? "left" : "right";
					}

					controls_config.set(section, "mouse_type", "motion");
					controls_config.set(section, "mouse_motion", direction);

					controls_config.set(section, "deadzone", control.deadzone);
					controls_config.set(section, "exponent", control.exponent);
					controls_config.set(section, "gain", control.gain);
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

		for (const auto & control : controls[n])
		{
			bool handled = false;
			float tempval = newval;

			if (control.device < Control::JOYSTICKS)
			{
				if (control.type == Control::AXIS)
				{
					float val = eventsystem.GetJoyAxis(control.device, control.id);
					if (control.negative)
						val = -val;
					val = ApplyDeadzone(control.deadzone,val);
					val = ApplyGain(control.gain,val);

					float absval = val;
					bool neg = false;
					if (val < 0)
					{
						absval = -val;
						neg = true;
					}
					val = ApplyExponent(control.exponent,absval);
					if (neg)
						val = -val;

					tempval = val;
					handled = true;
				}
				else if (control.type == Control::BUTTON)
				{
					auto button = eventsystem.GetJoyButton(control.device, control.id);

					if (control.onetime)
					{
						if (control.pushdown && button.GetImpulseRising())
							tempval = 1.0;
						else if (!control.pushdown && button.GetImpulseFalling())
							tempval = 1.0;
						else
							tempval = 0.0;
						handled = true;
					}
					else
					{
						float downval = 1.0;
						float upval = 0.0;
						if (!control.pushdown)
						{
							downval = 0.0;
							upval = 1.0;
						}

						tempval = Ramp(lastinputs[n], button.GetState() ? downval : upval, button_ramp, dt);
						handled = true;
					}
				}
			}
			else if (control.device == Control::KEYBOARD)
			{
				//cout << "type key" << std::endl;

				auto key = eventsystem.GetKeyState(SDL_Keycode(control.id));

				if (control.onetime)
				{
					if (control.pushdown && key.GetImpulseRising())
						tempval = 1.0;
					else if (!control.pushdown && key.GetImpulseFalling())
						tempval = 1.0;
					else
						tempval = 0.0;
					handled = true;
				}
				else
				{
					float downval = 1.0;
					float upval = 0.0;
					if (!control.pushdown)
					{
						downval = 0.0;
						upval = 1.0;
					}

					//if (inputs[n->first] != keystate.down ? downval : upval) std::cout << "Key ramp: " << control.keycode << ", " << n->first << std::endl;
					tempval = Ramp(lastinputs[n], key.GetState() ? downval : upval, button_ramp, dt);

					handled = true;
				}
			}
			else if (control.device == Control::MOUSE)
			{
				//cout << "type mouse" << std::endl;

				if (control.type == Control::BUTTON)
				{
					//cout << "mousebutton" << std::endl;

					auto button = eventsystem.GetMouseButtonState(control.id);

					if (control.onetime)
					{
						if (control.pushdown && button.GetImpulseRising())
							tempval = 1.0;
						else if (!control.pushdown && button.GetImpulseFalling())
							tempval = 1.0;
						else
							tempval = 0.0;
						handled = true;
					}
					else
					{
						float downval = 1.0;
						float upval = 0.0;
						if (!control.pushdown)
						{
							downval = 0.0;
							upval = 1.0;
						}

						tempval = Ramp(lastinputs[n], button.GetState() ? downval : upval, button_ramp, dt);
						handled = true;
					}
				}
				else if (control.type == Control::AXIS)
				{
					//cout << "mousemotion" << std::endl;

					std::vector <int> pos = eventsystem.GetMousePosition();
					//std::cout << pos[0] << "," << pos[1] << std::endl;

					float xval = (pos[0]-screenw/2.0)/(screenw/4.0);
					float yval = (pos[1]-screenh/2.0)/(screenh/4.0);
					xval = Clamp(xval, -1.0f, 1.0f);
					yval = Clamp(yval, -1.0f, 1.0f);

					float val = 0;

					if (control.id == Control::MOUSEY)
						val = control.negative ? -yval : yval;
					else if (control.id == Control::MOUSEX)
						val = control.negative ? -xval : xval;

					val = Clamp(val, 0.0f, 1.0f);
					val = ApplyDeadzone(control.deadzone,val);
					val = ApplyGain(control.gain,val);
					val = Clamp(val, 0.0f, 1.0f);

					float absval = val;
					bool neg = false;
					if (val < 0)
					{
						absval = -val;
						neg = true;
					}
					val = ApplyExponent(control.exponent,absval);
					if (neg)
						val = -val;

					tempval = Clamp(val, 0.0f, 1.0f);

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

		inputs[n] = Clamp(newval, 0.0f, 1.0f);

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
			inputs[CarInput::NEUTRAL] = 1;
	}

	float steerpos = lastinputs[CarInput::STEER_RIGHT] - lastinputs[CarInput::STEER_LEFT];

	lastinputs = inputs;

	//do steering processing
	ProcessSteering(joytype, steerpos, dt, joy_200, carms*2.23693629f, speedsens);

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

const std::string & CarControlMap::GetStringFromInput(const unsigned input)
{
	return carinput_strings[input];
}

unsigned CarControlMap::GetInputFromString(const std::string & str)
{
	auto i = carinput_stringmap.find(str);
	if (i != carinput_stringmap.end())
		return i->second;

	return GameInput::INVALID;
}

const std::string & CarControlMap::GetStringFromKeycode(const int code)
{
	for (const auto & i : keycode_stringmap)
		if (i.second == code)
			return i.first;

	return invalid;
}

int CarControlMap::GetKeycodeFromString(const std::string & str)
{
	auto i = keycode_stringmap.find(str);
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
			decimate = 1;
		else if (carmph < normalat)
		{
			float coeff = (carmph - transat)/(normalat - transat);
			decimate = (decimate-1)*coeff + 1;
		}

		//std::cout << "Decimating: " << val << " to " << val / decimate << std::endl;

		val = val/decimate;
	}

	//do speed sensitivity
	if ( speedsens != 0 )
	{
		float coeff = 1;
		if (carmph > 1)
		{
			float ratio = 20;
			float ssco = speedsens*(1-std::pow(val,2.0f));
			coeff = ratio*45*(1-std::atan(carmph*80*ssco)*0.6366198f);
		}
		if (coeff > 1) coeff = 1;

		//std::cout << "Speed sensitivity coefficient: " << coeff << std::endl;

		val = val*coeff;
	}

	//std::cout << "After speed sensitivity: " << val << std::endl;

	//rate-limit non-wheel controls
	if (joytype != "wheel")
	{
		//if (i->first == inputs[CarInput::STEER_LEFT])
		//steerpos = -steerpos;
		float steerstep = 5*dt;

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

		val = Clamp(steerpos, -1.0f, 1.0f);

		/*float coeff = 0.97f;
		val = steerpos * coeff + val * (1-coeff);*/
	}

	//std::cout << "After rate limit val: " << val << std::endl;

	//std::cout << "Steer left: " << inputs[CarInput::STEER_LEFT] << std::endl;
	//std::cout << "Steer right: " << inputs[CarInput::STEER_RIGHT] << std::endl;
	//std::cout << "Current steering: " << car.GetLastSteer() << std::endl;
	//std::cout << "New steering: " << val << std::endl;

	inputs[CarInput::STEER_LEFT] = 0;
	inputs[CarInput::STEER_RIGHT] = 0;
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
	return type == AXIS;
}

std::string CarControlMap::Control::GetInfo() const
{
	if (device == KEYBOARD)
	{
		return GetStringFromKeycode(id);
	}

	if (device == MOUSE)
	{
		std::ostringstream s;
		s << "MOUSE";

		if (type == BUTTON)
			s << id;
		else if (type == AXIS)
		{
			if (id == MOUSEY)
				s << (negative ? "UP" : "DOWN");
			else
				s << (negative ? "LEFT" : "RIGHT");
		}
		return s.str();
	}

	if (device < JOYSTICKS)
	{
		std::ostringstream s;
		s << "JOY" << (unsigned)device;

		if (type == AXIS)
		{
			s << "AXIS" << id << (negative ? "-" : "+");
			return s.str();
		}

		if (type == BUTTON)
		{
			s << "BTN" << id;
			return s.str();
		}

		if (type == HAT)
		{
			s << "HAT" << id;
			return s.str();
		}
	}

	return invalid;
}

void CarControlMap::Control::DebugPrint(std::ostream & out) const
{
	out << id << " " << type << " " << (unsigned)device << " " <<
		negative << " " << onetime << " " << pushdown << " " <<
		deadzone << " " << exponent << " " << gain << std::endl;
}

bool CarControlMap::Control::operator==(const Control & other) const
{
	// don't care about certain flags
	return id == other.id &&
		type == other.type &&
		device == other.device &&
		negative == other.negative;
}

bool CarControlMap::Control::operator<(const Control & other) const
{
	// don't care about certain flags
	return id < other.id &&
		type < other.type &&
		device < other.device &&
		negative < other.negative;
}

void CarControlMap::Control::ReadFrom(std::istream & in)
{
	int newtype;
	in >> id >> newtype >> device >>
		negative >> onetime >> pushdown >>
		deadzone >> exponent >> gain;
	type = TypeEnum(newtype);
}

CarControlMap::Control::Control() :
	id(0), type(AXIS), device(UNKNOWN),
	negative(false), onetime(true), pushdown(false),
	deadzone(0), exponent(1), gain(1)
{}
