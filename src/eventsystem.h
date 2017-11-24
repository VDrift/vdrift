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

#include <SDL2/SDL.h>
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
		public:
			class HatPosition
			{
				public:
					HatPosition() : centered(true), up(false), right(false), down(false), left(false) {}

					bool centered;
					bool up;
					bool right;
					bool down;
					bool left;
			};

		private:
			SDL_Joystick * sdl_joyptr;
			std::vector <float> axis;
			std::vector <Toggle> button;
			std::vector <HatPosition> hat;

		public:
			Joystick() : sdl_joyptr(NULL) {}

			Joystick(SDL_Joystick * ptr, int numaxes, int numbuttons, int numhats) : sdl_joyptr(ptr)
			{
				axis.resize(numaxes, 0);
				button.resize(numbuttons);
				hat.resize(numhats);
			}

			int GetNumAxes() const {return axis.size();}

			int GetNumButtons() const {return button.size();}

			void AgeToggles()
			{
				for (auto & b : button)
				{
					b.Tick();
				}
			}

			void SetAxis(unsigned int axisid, float newval)
			{
				assert (axisid < axis.size());
				axis[axisid] = newval;
			}

			float GetAxis(unsigned int axisid) const
			{
				//assert (axisid < axis.size()); //don't want to assert since this could be due to a control file misconfiguration
				if (axisid >= axis.size())
					return 0.0;
				else
					return axis[axisid];
			}

			Toggle GetButton(unsigned int buttonid) const
			{
				//don't want to assert since this could be due to a control file misconfiguration

				if (buttonid >= button.size())
					return Toggle();
				else
					return button[buttonid];
			}

			void SetButton(unsigned int id, bool state)
			{
				assert (id < button.size());
				button[id].Set(state);
			}
	};

	EventSystem();

	~EventSystem();

	void Init(std::ostream & info_output);

	void BeginFrame();

	void EndFrame() {}

	inline double Get_dt() {return dt;}

	inline bool GetQuit() const {return quit;}

	void Quit() {quit = true;}

	void ProcessEvents();

	Toggle GetMouseButtonState(int id) const { return GetToggle(mbutmap, id); }

	Toggle GetKeyState(SDL_Keycode id) const { return GetToggle(keymap, id); }

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

	template <class T>
	void HandleToggle(std::map <T, Toggle> & togglemap, const DirectionEnum dir, const T & id)
	{
		togglemap[id].Tick();
		togglemap[id].Set(dir == DOWN);
	}

	void HandleMouseButton(DirectionEnum dir, int id);

	void HandleKey(DirectionEnum dir, SDL_Keycode id);

	void RecordFPS(const float fps);

	void HandleQuit() {quit = true;}

	template <class T>
	void AgeToggles(std::map <T, Toggle> & togglemap)
	{
		std::list <typename std::map<T, Toggle>::iterator> todel;
		for (auto i = togglemap.begin(); i != togglemap.end(); ++i)
		{
			i->second.Tick();
			if (!i->second.GetState() && !i->second.GetImpulseFalling())
				todel.push_back(i);
		}

		for (auto i : todel)
			togglemap.erase(i);
	}

	template <class T>
	Toggle GetToggle(const std::map <T, Toggle> & togglemap, const T & id) const
	{
		auto i = togglemap.find(id);
		if (i != togglemap.end())
			return i->second;
		return Toggle();
	}
};

#endif
