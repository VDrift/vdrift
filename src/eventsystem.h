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

#ifndef _EVENTSYSTEM_H
#define _EVENTSYSTEM_H

#include "toggle.h"

#include <SDL3/SDL.h>
#include <vector>
#include <map>
#include <list>
#include <iosfwd>
#include <cassert>

class EventSystem
{
public:
	class Joystick
	{
		private:
			unsigned id;
			std::vector <float> axis;
			std::vector <Toggle> button;
			std::vector <uint8_t> hat;

		public:
			Joystick() : id(-1) {}

			Joystick(unsigned jid, int numaxes, int numbuttons, int numhats) : id(jid)
			{
				axis.resize(numaxes, 0);
				button.resize(numbuttons + numhats * 4);
				hat.resize(numhats);
			}

			unsigned GetId() const {return id;}

			int GetNumAxes() const {return axis.size();}

			int GetNumButtons() const {return button.size();}

			int GetNumHats() const {return hat.size();}

			void AgeToggles()
			{
				for (auto & b : button)
				{
					b.Tick();
				}
			}

			void SetAxis(unsigned n, float val)
			{
				assert (n < axis.size());
				axis[n] = val;
			}

			float GetAxis(unsigned n) const
			{
				if (n < axis.size())
					return axis[n];
				return 0;
			}

			Toggle GetButton(unsigned n) const
			{
				// don't want to assert since this could be due to a control file misconfiguration
				if (n < button.size())
					return button[n];
				return Toggle();
			}

			void SetButton(unsigned n, bool state)
			{
				assert (n < button.size());
				button[n].Set(state);
			}

			uint8_t GetHat(unsigned n) const
			{
				assert (n < hat.size());
				return hat[n];
			}

			void SetHat(unsigned n, uint8_t state)
			{
				assert (n < hat.size());
				hat[n] = state;
			}
	};

	EventSystem();

	~EventSystem();

	void Init(std::ostream & info_output, std::ostream & error_output);

	void BeginFrame();

	void EndFrame() {}

	inline double Get_dt() {return dt;}

	inline bool GetQuit() const {return quit;}

	void Quit() {quit = true;}

	void ProcessEvents();

	Toggle GetMouseButtonState(int n) const { return GetToggle(mbutmap, n); }

	Toggle GetKeyState(SDL_Keycode k) const { return GetToggle(keymap, k); }

	std::map <SDL_Keycode, Toggle> & GetKeyMap() {return keymap;}

	///returns a 2 element vector (x,y)
	std::vector <int> GetMousePosition() const;

	///returns a 2 element vector (x,y)
	std::vector <int> GetMouseRelativeMotion() const;

	float GetFPS() const;

	enum StimEnum
	{
		STIM_AGE_KEYS,
  		STIM_AGE_MBUT,
		STIM_INSERT_KEY_DOWN,
		STIM_INSERT_KEY_UP,
		STIM_INSERT_MBUT_DOWN,
		STIM_INSERT_MBUT_UP,
		STIM_INSERT_MOTION
	};
	void TestStim(StimEnum stim);

	float GetJoyAxis(unsigned int joynum, int axisnum) const
	{
		if (joynum < joysticks.size())
			return joysticks[joynum].GetAxis(axisnum);
		else
			return 0.0;
	}

	Toggle GetJoyButton(unsigned int joynum, int buttonnum) const
	{
		if (joynum < joysticks.size())
			return joysticks[joynum].GetButton(buttonnum);
		else
			return Toggle();
	}

	int GetNumJoysticks() const
	{
		return joysticks.size();
	}

	int GetNumAxes(unsigned int joynum) const
	{
		if (joynum < joysticks.size())
			return joysticks[joynum].GetNumAxes();
		else
			return 0;
	}

	int GetNumButtons(unsigned int joynum) const
	{
		if (joynum < joysticks.size())
			return joysticks[joynum].GetNumButtons();
		else
			return 0;
	}

	std::vector <Joystick> & GetJoysticks()
	{
		return joysticks;
	}

private:
	double lasttick;
	double dt;
	bool quit;

	std::vector <Joystick> joysticks;
	std::map <SDL_Keycode, Toggle> keymap;
	std::map <int, Toggle> mbutmap;

	int mousex, mousey, mousexrel, mouseyrel;

	unsigned int fps_memory_window;
	std::list <float> fps_memory;

	enum DirectionEnum {UP, DOWN};

	void HandleMouseMotion(int x, int y, int xrel, int yrel);

	void HandleMouseButton(DirectionEnum dir, int id);

	void HandleKey(DirectionEnum dir, SDL_Keycode id);

	void HandleJoystickButton(unsigned joyid, uint8_t button, bool up);

	void HandleJoystickHat(unsigned joyid, uint8_t hat, uint8_t val);

	void HandleJoystickAxis(unsigned joyid, uint8_t axis, int val);

	void RecordFPS(const float fps);

	void HandleQuit() {quit = true;}

	template <class ToggleMap, typename T>
	Toggle GetToggle(const ToggleMap & togglemap, const T & id) const
	{
		auto i = togglemap.find(id);
		if (i != togglemap.end())
			return i->second;
		return Toggle();
	}

	template <class ToggleMap, typename T>
	void HandleToggle(ToggleMap & togglemap, const DirectionEnum dir, const T & id)
	{
		togglemap[id].Tick();
		togglemap[id].Set(dir == DOWN);
	}

	template <class ToggleMap>
	void AgeToggles(ToggleMap & togglemap)
	{
		auto i = togglemap.begin();
		while (i != togglemap.end())
		{
			i->second.Tick();
			if (!i->second.GetState() && !i->second.GetImpulseFalling())
				togglemap.erase(i++);
			else
				i++;
		}
	}
};

#endif
