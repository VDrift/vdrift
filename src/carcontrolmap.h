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

#ifndef _CARCONTROLMAP_H
#define _CARCONTROLMAP_H

#include "gameinput.h"

#include <cassert>
#include <iosfwd>
#include <string>
#include <vector>
#include <map>

class Config;
class EventSystem;

class CarControlMap
{
public:
	CarControlMap();

	bool Load(const std::string & controlfile, std::ostream & info_output, std::ostream & error_output);

	void Save(const std::string & controlfile);

	void Save(Config & controlfile);

	/// query the eventsystem for info, then return the resulting input array
	const std::vector <float> & ProcessInput(
		const std::string & joytype, const EventSystem & eventsystem,
		float dt, bool joy_200, float carms, float speedsens,
		int screenw, int screenh, float button_ramp, bool hgateshifter);

	const std::vector< float > & GetInputs() const {return inputs;}

	float GetInput(GameInput::Enum inputid) const {assert((unsigned)inputid < inputs.size()); return inputs[inputid];}

	void GetControlsInfo(std::map<std::string, std::string> & info) const;

	struct Control
	{
		enum TypeEnum
		{
			KEY,
			JOY,
			MOUSE,
			UNKNOWN
		} type;
		bool onetime;
		bool pushdown;
		int keycode;

		int joynum;
		int joyaxis;
		enum JoyAxisEnum
		{
			POSITIVE,
			NEGATIVE
		} joyaxistype;
		enum JoyTypeEnum
		{
			JOYAXIS,
			JOYBUTTON,
			JOYHAT
		} joytype;

		enum MouseTypeEnum
		{
			MOUSEBUTTON,
			MOUSEMOTION
		} mousetype;
		enum MouseDirectionEnum
		{
			UP,
			DOWN,
			LEFT,
			RIGHT
		} mdir;
		bool last_mouse_state;

		float deadzone;
		float exponent;
		float gain;

		bool IsAnalog() const;

		std::string GetInfo() const;

		void DebugPrint(std::ostream & out) const;

		bool operator==(const Control & other) const;

		bool operator<(const Control & other) const;

		void ReadFrom(std::istream & in);

		Control();
	};

	/// if controlid is greather than number of controls return unknown control
	Control GetControl(const std::string & inputname, size_t controlid);

	/// if controlid greater than number of controls, add new control
	void SetControl(const std::string & inputname, size_t controlid, const Control & control);

	/// delete if controlid less than number of controls
	void DeleteControl(const std::string & inputname, size_t controlid);

private:
	std::vector< std::vector<Control> > controls;

	/// the vector is indexed by CARINPUT values
	std::vector <float> inputs;
	std::vector <float> lastinputs;

	/// max number of controls per input
	static const size_t max_controls = 3;

	/// used to stringify/destringify the CARINPUT enum
	static const std::map <std::string, unsigned> carinput_stringmap;
	static const std::vector<std::string> carinput_strings;

	/// used to turn legacy key names from older vdrift releases into keycodes
	static const std::map <std::string, int> keycode_stringmap;

	static std::map <std::string, unsigned> InitCarInputStringMap();
	static std::vector<std::string> InitCarInputStrings();
	static std::map <std::string, int> InitKeycodeStringMap();

	static const std::string & GetStringFromInput(const unsigned input);
	static unsigned GetInputFromString(const std::string & str);
	static const std::string & GetStringFromKeycode(const int code);
	static int GetKeycodeFromString(const std::string & str);

	void AddControl(Control newctrl, const std::string & inputname, std::ostream & error_output);

	void ProcessSteering(const std::string & joytype, float steerpos, float dt, bool joy_200, float carmph, float speedsens);
};

#endif
