#ifndef _CARCONTROLMAP_LOCAL_H
#define _CARCONTROLMAP_LOCAL_H

#include "eventsystem.h"
#include "carinput.h"

#include <map>
#include <string>
#include <iostream>
#include <vector>

class CONFIG;
class GAME;

class CARCONTROLMAP_LOCAL
{
public:
	CARCONTROLMAP_LOCAL();

	bool Load(const std::string & controlfile, std::ostream & info_output, std::ostream & error_output);

	void Save(const std::string & controlfile, std::ostream & info_output, std::ostream & error_output);

	void Save(CONFIG & controlfile, std::ostream & info_output, std::ostream & error_output);

	/// query the eventsystem for info, then return the resulting input array
	const std::vector <float> & ProcessInput(const std::string & joytype, EVENTSYSTEM_SDL & eventsystem, float steerpos, float dt, bool joy_200, float carms, float speedsens, int screenw, int screenh, float button_ramp, bool hgateshifter);

	const std::vector< float > & GetInputs() const {return inputs;}

	float GetInput(CARINPUT::CARINPUT inputid) const {assert((unsigned int)inputid < inputs.size()); return inputs[inputid];}

	void GetControlsInfo(std::map<std::string, std::string> & info) const;

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
		bool pushdown;
		int keycode;

		int joynum;
		int joyaxis;
		enum JOYAXISTYPE
		{
			POSITIVE,
			NEGATIVE
		} joyaxistype;
		enum JOYTYPE
		{
			JOYAXIS,
			JOYBUTTON,
			JOYHAT
		} joytype;

		enum MOUSETYPE
		{
			MOUSEBUTTON,
			MOUSEMOTION
		} mousetype;
		enum MOUSEDIRECTION
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

		bool operator==(const CONTROL & other) const;

		bool operator<(const CONTROL & other) const;

		void ReadFrom(std::istream & in);

		CONTROL();
	};

	/// if controlid is greather than number of controls return unknown control
	CONTROL GetControl(const std::string & inputname, size_t controlid);

	/// if controlid greater than number of controls, add new control
	void SetControl(const std::string & inputname, size_t controlid, const CONTROL & control);

	/// delete if controlid less than number of controls
	void DeleteControl(const std::string & inputname, size_t controlid);

private:
	std::vector< std::vector<CONTROL> > controls;

	/// the vector is indexed by CARINPUT values
	std::vector <float> inputs;
	std::vector <float> lastinputs;

	/// max number of controls per input
	static const int max_controls = 3;

	/// used to stringify/destringify the CARINPUT enum
	static const std::map <std::string, CARINPUT::CARINPUT> carinput_stringmap;
	static const std::vector<std::string> carinput_strings;
	
	/// used to turn legacy key names from older vdrift releases into keycodes
	static const std::map <std::string, int> keycode_stringmap;

	static std::map <std::string, CARINPUT::CARINPUT> InitCarInputStringMap();
	static std::vector<std::string> InitCarInputStrings();
	static std::map <std::string, int> InitKeycodeStringMap();
	
	static const std::string & GetStringFromInput(const CARINPUT::CARINPUT input);
	static CARINPUT::CARINPUT GetInputFromString(const std::string & str);
	static const std::string & GetStringFromKeycode(const int code);
	static int GetKeycodeFromString(const std::string & str);

	void AddControl(CONTROL newctrl, const std::string & inputname, std::ostream & error_output);

	void ProcessSteering(const std::string & joytype, float steerpos, float dt, bool joy_200, float carmph, float speedsens);
};

#endif
