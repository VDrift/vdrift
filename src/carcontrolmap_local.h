#ifndef _CARCONTROLMAP_LOCAL_H
#define _CARCONTROLMAP_LOCAL_H

#include "eventsystem.h"
#include "carinput.h"
#include "car.h"

#include <map>
#include <string>
#include <iostream>
#include <vector>

class WIDGET_CONTROLGRAB;
class CONFIG;
class GAME;

class CARCONTROLMAP_LOCAL
{
public:
	CARCONTROLMAP_LOCAL();

	void Load(const std::string & controlfile, std::ostream & info_output, std::ostream & error_output);

	void Save(const std::string & controlfile, std::ostream & info_output, std::ostream & error_output);

	void Save(CONFIG & controlfile, std::ostream & info_output, std::ostream & error_output);

	void AddInputKey(const std::string & inputname, bool analog, bool only_one, SDLKey key, std::ostream & error_output);

	void AddInputMouseButton(const std::string & inputname, bool analog, bool only_one, int mouse_btn, std::ostream & error_output);

	void AddInputMouseMotion(const std::string & inputname, bool analog, bool only_one, const std::string & mouse_direction, std::ostream & error_output);

	void AddInputJoyButton(const std::string & inputname, bool analog, bool only_one, int joy_num, int joy_btn, std::ostream & error_output);

	void AddInputJoyAxis(const std::string & inputname, bool analog, bool only_one, int joy_num, int joy_axis, const std::string & axis_type, std::ostream & error_output);

	/// query the eventsystem for info, then return the resulting input array
	const std::vector <float> & ProcessInput(const std::string & joytype, EVENTSYSTEM_SDL & eventsystem, float steerpos, float dt, bool joy_200, float carms, float speedsens, int screenw, int screenh, float button_ramp, bool hgateshifter);

	const std::vector< float > & GetInputs() const {return inputs;}

	float GetInput(CARINPUT::CARINPUT inputid) const {assert((unsigned int)inputid < inputs.size()); return inputs[inputid];}

	struct CONTROL
	{
		enum TYPE
		{
			KEY,
			JOY,
			MOUSE,
			UNKNOWN
		} type;

		bool onetime;

		int joynum;
		int joyaxis;
		enum JOYAXISTYPE
		{
			POSITIVE,
			NEGATIVE,
			BOTH
		} joyaxistype;
		int joybutton;

		enum JOYTYPE
		{
			JOYAXIS,
			JOYBUTTON,
			JOYHAT
		} joytype;
		bool joypushdown;

		int keycode;
		bool keypushdown;

		enum MOUSETYPE
		{
			MOUSEBUTTON,
			MOUSEMOTION
		} mousetype;
		int mbutton;
		enum MOUSEDIRECTION
		{
			UP,
			DOWN,
			LEFT,
			RIGHT
		} mdir;
		bool last_mouse_state;
		bool mouse_push_down;

		float deadzone;
		float exponent;
		float gain;

		bool IsAnalog() const
		{
			return (type == JOY && joytype == JOYAXIS) || (type == MOUSE && mousetype == MOUSEMOTION);
		}

		void DebugPrint(std::ostream & out) const
		{
			out << type << " " << onetime << " " << joynum << " " << joyaxis << " " <<
					joyaxistype << " " << joybutton << " " << joytype << " " <<
					joypushdown << " " << keycode << " " << keypushdown << " " <<
					mousetype << " " << mbutton << " " << mdir << " " <<
					last_mouse_state << " " << mouse_push_down << " " <<
					deadzone << " " << exponent << " " << gain << std::endl;
		}

		void ReadFrom(std::istream & in)
		{
			int newtype, newjoyaxistype, newjoytype, newmousetype, newmdir;
			in >> newtype >> onetime >> joynum >> joyaxis >>
					newjoyaxistype >> joybutton >> newjoytype >>
					joypushdown >> keycode >> keypushdown >>
					newmousetype >> mbutton >> newmdir >>
					last_mouse_state >> mouse_push_down >>
					deadzone >> exponent >> gain;
			type=TYPE(newtype);
			joyaxistype=JOYAXISTYPE(newjoyaxistype);
			joytype=JOYTYPE(newjoytype);
			mousetype=MOUSETYPE(newmousetype);
			mdir=MOUSEDIRECTION(newmdir);
		}

		/*void MemDump(std::ostream & out)
		{
			for (unsigned int i = 0; i < sizeof(CONTROL); i++)
			{
				char c = ((char *) this)[i];
				std::cout << (int) c << " ";
			}
			std::cout << std::endl;
		}*/

		bool operator==(const CONTROL & other) const
		{
			CONTROL me = *this;
			CONTROL them = other;

			//don't care about certain flags
			me.onetime = 1;
			me.joypushdown = 1;
			me.keypushdown = 1;
			me.mouse_push_down = 1;
			me.deadzone = 0;
			me.exponent = 1;
			me.gain = 1;
			them.onetime = 1;
			them.joypushdown = 1;
			them.keypushdown = 1;
			them.mouse_push_down = 1;
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

		bool operator<(const CONTROL & other) const
		{
			CONTROL me = *this;
			CONTROL them = other;

			me.onetime = 1;
			me.joypushdown = 1;
			me.keypushdown = 1;
			me.mouse_push_down = 1;
			me.deadzone = 0;
			me.exponent = 1;
			me.gain = 1;
			them.onetime = 1;
			them.joypushdown = 1;
			them.keypushdown = 1;
			them.mouse_push_down = 1;
			them.deadzone = 0;
			them.exponent = 1;
			them.gain = 1;

			std::stringstream mestr;
			std::stringstream themstr;
			me.DebugPrint(mestr);
			them.DebugPrint(themstr);

			return (mestr.str() < themstr.str());
		}

		CONTROL() : type(UNKNOWN),onetime(true),joynum(0),joyaxis(0),joyaxistype(POSITIVE),
			joybutton(0),joytype(JOYAXIS),joypushdown(true),keycode(0),keypushdown(true),
			mousetype(MOUSEBUTTON),mbutton(0),mdir(UP),last_mouse_state(false),
			mouse_push_down(true),
			deadzone(0.0), exponent(1.0), gain(1.0)
		{}
	};

	void DeleteControl(const CONTROL & ctrltodel, const std::string & inputname, std::ostream & error_output);

	void UpdateControl(const CONTROL & ctrltoupdate, const std::string & inputname, std::ostream & error_output);

private:
	std::map <CARINPUT::CARINPUT, std::vector<CONTROL> > controls;

	/// the vector is indexed by CARINPUT values
	std::vector <float> inputs;
	std::vector <float> lastinputs;

	/// used to turn legacy key names from older vdrift releases into keycodes
	std::map <std::string, int> legacy_keycodes;
	void PopulateLegacyKeycodes();

	/// used to stringify/destringify the CARINPUT enum
	std::map <std::string, CARINPUT::CARINPUT> carinput_stringmap;

	std::string GetStringFromInput(const CARINPUT::CARINPUT input) const;

	CARINPUT::CARINPUT GetInputFromString(const std::string & str) const;

	void AddControl(CONTROL newctrl, const std::string & inputname, bool only_one, std::ostream & error_output);

	void ProcessSteering(const std::string & joytype, float steerpos, float dt, bool joy_200, float carmph, float speedsens);
};

#endif
