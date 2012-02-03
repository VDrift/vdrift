#ifndef _CARCONTROLMAP_LOCAL_H
#define _CARCONTROLMAP_LOCAL_H

#include "eventsystem.h"
#include "carinput.h"

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

	bool Load(const std::string & controlfile, std::ostream & info_output, std::ostream & error_output);

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

		bool IsAnalog() const;

		void DebugPrint(std::ostream & out) const;

		bool operator==(const CONTROL & other) const;

		bool operator<(const CONTROL & other) const;

		void ReadFrom(std::istream & in);

		CONTROL();
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
