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

#include "eventsystem.h"
#include "unittest.h"

#include <iostream>
#include <numeric>

EventSystem::EventSystem() :
	lasttick(0),
	dt(0),
	quit(false),
	mousex(0),
	mousey(0),
	mousexrel(0),
	mouseyrel(0)
{
	// ctor
}

EventSystem::~EventSystem()
{
	for (const auto & j : joysticks)
	{
		auto jptr = SDL_GetJoystickFromID(j.GetId());
		if (jptr) SDL_CloseJoystick(jptr);
	}
}

void EventSystem::Init(std::ostream & info_output, std::ostream & error_output)
{
	int num_joysticks = 0;
	SDL_JoystickID * joystiks_ids = SDL_GetJoysticks(&num_joysticks);
	info_output << "Joysticks found: " << num_joysticks << std::endl;

	joysticks.resize(num_joysticks);
	for (int i = 0; i < num_joysticks; ++i)
	{
		SDL_JoystickID id = joystiks_ids[i];
		SDL_Joystick * jp = SDL_OpenJoystick(id);
		if (jp == NULL)
		{
			error_output << SDL_GetError() << std::endl;
		}
		joysticks[i] = Joystick(id, SDL_GetNumJoystickAxes(jp), SDL_GetNumJoystickButtons(jp), SDL_GetNumJoystickHats(jp));
		info_output << "  " << id << " " << SDL_GetJoystickName(jp) << std::endl;
	}

	SDL_free(joystiks_ids);
	SDL_SetJoystickEventsEnabled(true);
}

void EventSystem::BeginFrame()
{
	if (lasttick == 0)
		lasttick = SDL_GetTicks();
	else
	{
		double thistick = SDL_GetTicks();

		dt = (thistick-lasttick)*1E-3;

		/*if (throttle && dt < game.TickPeriod())
		{
			//cout << "throttling: " << lasttick.data << "," << thistick << std::endl;
			SDL_Delay(10);
			thistick = SDL_GetTicks();
			dt = (thistick-lasttick)*1E-3;
		}*/

		lasttick = thistick;
	}

	RecordFPS(1/dt);
}

void EventSystem::ProcessEvents()
{
	AgeToggles(keymap);
	AgeToggles(mbutmap);
	for (auto & joystick : joysticks)
	{
		joystick.AgeToggles();
	}

	SDL_Event event;
	while (SDL_PollEvent(&event))
	{
		switch (event.type)
		{
		case SDL_EVENT_MOUSE_MOTION:
			HandleMouseMotion(event.motion.x, event.motion.y, event.motion.xrel, event.motion.yrel);
			break;
		case SDL_EVENT_MOUSE_BUTTON_DOWN:
			HandleMouseButton(DOWN, event.button.button);
			break;
		case SDL_EVENT_MOUSE_BUTTON_UP:
			HandleMouseButton(UP, event.button.button);
			break;
		case SDL_EVENT_KEY_DOWN:
			HandleKey(DOWN, event.key.key);
			break;
		case SDL_EVENT_KEY_UP:
			HandleKey(UP, event.key.key);
			break;
		case SDL_EVENT_JOYSTICK_BUTTON_DOWN:
			HandleJoystickButton(event.jbutton.which, event.jbutton.button, true);
			break;
		case SDL_EVENT_JOYSTICK_BUTTON_UP:
			HandleJoystickButton(event.jbutton.which, event.jbutton.button, false);
			break;
		case SDL_EVENT_JOYSTICK_HAT_MOTION:
			HandleJoystickHat(event.jhat.which, event.jhat.hat, event.jhat.value);
			break;
		case SDL_EVENT_JOYSTICK_AXIS_MOTION:
			HandleJoystickAxis(event.jbutton.which, event.jaxis.axis, event.jaxis.value);
			break;
		case SDL_EVENT_QUIT:
			HandleQuit();
			break;
		default:
			break;
		}
	}
}

void EventSystem::HandleMouseMotion(int x, int y, int xrel, int yrel)
{
	mousex = x;
	mousey = y;
	mousexrel = xrel;
	mouseyrel = yrel;
}

void EventSystem::HandleMouseButton(DirectionEnum dir, int id)
{
	//std::cout << "Mouse button " << id << ", " << (dir==DOWN) << endl;
	//mbutmap[id].Tick();
	HandleToggle(mbutmap, dir, id);
}

void EventSystem::HandleKey(DirectionEnum dir, SDL_Keycode id)
{
	//if (dir == DOWN) std::cout << "Key #" << (int)id << " pressed" << endl;
	HandleToggle(keymap, dir, id);
}

void EventSystem::HandleJoystickButton(unsigned joyid, uint8_t button, bool up)
{
	for (auto & joy : joysticks)
	{
		if (joy.GetId() == joyid)
		{
			joy.SetButton(button, up);
			return;
		}
	}
	// TODO: log unknown joystick button event
}

template <class Joystick>
inline void SetHatButton(Joystick & joystick, unsigned buttonoffset, uint8_t hatvalue, bool state)
{
	if (hatvalue & SDL_HAT_UP)
		joystick.SetButton(buttonoffset + 0, state);
	if (hatvalue & SDL_HAT_RIGHT)
		joystick.SetButton(buttonoffset + 1, state);
	if (hatvalue & SDL_HAT_DOWN)
		joystick.SetButton(buttonoffset + 2, state);
	if (hatvalue & SDL_HAT_LEFT)
		joystick.SetButton(buttonoffset + 3, state);
}

template <class Joystick>
inline void HandleHat(Joystick & joystick, uint8_t hatid, uint8_t hatvalue)
{
	assert(hatid < joystick.GetNumHats());
	auto newvalue = hatvalue;
	auto oldvalue = joystick.GetHat(hatid);
	if (newvalue != oldvalue)
	{
		auto buttonoffset = joystick.GetNumButtons() - (joystick.GetNumHats() - hatid) * 4;
		SetHatButton(joystick, buttonoffset, oldvalue, false);
		SetHatButton(joystick, buttonoffset, newvalue, true);
		joystick.SetHat(hatid, newvalue);
	}
}

void EventSystem::HandleJoystickHat(unsigned joyid, uint8_t hat, uint8_t val)
{
	for (auto & joy : joysticks)
	{
		if (joy.GetId() == joyid)
		{
			HandleHat(joy, hat, val);
			return;
		}
	}
	// TODO: log unknown joystick hat event
}

void EventSystem::HandleJoystickAxis(unsigned joyid, uint8_t axis, int val)
{
	for (auto & joy : joysticks)
	{
		if (joy.GetId() == joyid)
		{
			joy.SetAxis(axis, val / 32768.0f);
			return;
		}
	}
	// TODO: log unknown joystick axis event
}

std::vector <int> EventSystem::GetMousePosition() const
{
	std::vector <int> o;
	o.reserve(2);
	o.push_back(mousex);
	o.push_back(mousey);
	return o;
}

std::vector <int> EventSystem::GetMouseRelativeMotion() const
{
	std::vector <int> o;
	o.reserve(2);
	o.push_back(mousexrel);
	o.push_back(mouseyrel);
	return o;
}

void EventSystem::TestStim(StimEnum stim)
{
	if (stim == STIM_AGE_KEYS)
	{
		AgeToggles(keymap);
	}
	if (stim == STIM_AGE_MBUT)
	{
		AgeToggles(mbutmap);
	}
	if (stim == STIM_INSERT_KEY_DOWN)
	{
		HandleKey(DOWN, SDLK_T);
	}
	if (stim == STIM_INSERT_KEY_UP)
	{
		HandleKey(UP, SDLK_T);
	}
	if (stim == STIM_INSERT_MBUT_DOWN)
	{
		HandleMouseButton(DOWN, SDL_BUTTON_LEFT);
	}
	if (stim == STIM_INSERT_MBUT_UP)
	{
		HandleMouseButton(UP, SDL_BUTTON_LEFT);
	}
	if (stim == STIM_INSERT_MOTION)
	{
		HandleMouseMotion(50,55,2,1);
	}
}

QT_TEST(eventsystem_test)
{
	EventSystem e;

	//key stuff
	{
		//check key insertion
		e.TestStim(EventSystem::STIM_INSERT_KEY_DOWN);
		auto b = e.GetKeyState(SDLK_T);
		QT_CHECK(b.GetState() && b.GetImpulseRising() && !b.GetImpulseFalling());

		//check key aging
		e.TestStim(EventSystem::STIM_AGE_KEYS);
		b = e.GetKeyState(SDLK_T);
		QT_CHECK(b.GetState() && !b.GetImpulseRising() && !b.GetImpulseFalling());
		e.TestStim(EventSystem::STIM_AGE_KEYS); //age again
		b = e.GetKeyState(SDLK_T);
		QT_CHECK(b.GetState() && !b.GetImpulseRising() && !b.GetImpulseFalling());

		//check key removal
		e.TestStim(EventSystem::STIM_INSERT_KEY_UP);
		b = e.GetKeyState(SDLK_T);
		QT_CHECK(!b.GetState() && !b.GetImpulseRising() && b.GetImpulseFalling());

		//check key aging
		e.TestStim(EventSystem::STIM_AGE_KEYS);
		b = e.GetKeyState(SDLK_T);
		QT_CHECK(!b.GetState() && !b.GetImpulseRising() && !b.GetImpulseFalling());
		e.TestStim(EventSystem::STIM_AGE_KEYS); //age again
		b = e.GetKeyState(SDLK_T);
		QT_CHECK(!b.GetState() && !b.GetImpulseRising() && !b.GetImpulseFalling());
	}

	//mouse button stuff
	{
		//check button insertion
		e.TestStim(EventSystem::STIM_INSERT_MBUT_DOWN);
		auto b = e.GetMouseButtonState(SDL_BUTTON_LEFT);
		QT_CHECK(b.GetState() && b.GetImpulseRising() && !b.GetImpulseFalling());

		//check button aging
		e.TestStim(EventSystem::STIM_AGE_MBUT);
		b = e.GetMouseButtonState(SDL_BUTTON_LEFT);
		QT_CHECK(b.GetState() && !b.GetImpulseRising() && !b.GetImpulseFalling());
		e.TestStim(EventSystem::STIM_AGE_MBUT); //age again
		b = e.GetMouseButtonState(SDL_BUTTON_LEFT);
		QT_CHECK(b.GetState() && !b.GetImpulseRising() && !b.GetImpulseFalling());

		//check button removal
		e.TestStim(EventSystem::STIM_INSERT_MBUT_UP);
		b = e.GetMouseButtonState(SDL_BUTTON_LEFT);
		QT_CHECK(!b.GetState() && !b.GetImpulseRising() && b.GetImpulseFalling());

		//check button aging
		e.TestStim(EventSystem::STIM_AGE_MBUT);
		b = e.GetMouseButtonState(SDL_BUTTON_LEFT);
		QT_CHECK(!b.GetState() && !b.GetImpulseRising() && !b.GetImpulseFalling());
		e.TestStim(EventSystem::STIM_AGE_MBUT); //age again
		b = e.GetMouseButtonState(SDL_BUTTON_LEFT);
		QT_CHECK(!b.GetState() && !b.GetImpulseRising() && !b.GetImpulseFalling());
	}

	//mouse motion stuff
	{
		e.TestStim(EventSystem::STIM_INSERT_MOTION);
		std::vector <int> mpos = e.GetMousePosition();
		QT_CHECK_EQUAL(mpos.size(),2);
		QT_CHECK_EQUAL(mpos[0],50);
		QT_CHECK_EQUAL(mpos[1],55);
		std::vector <int> mrel = e.GetMouseRelativeMotion();
		QT_CHECK_EQUAL(mrel.size(),2);
		QT_CHECK_EQUAL(mrel[0],2);
		QT_CHECK_EQUAL(mrel[1],1);
	}
}

void EventSystem::RecordFPS(const float fps)
{
	fps_memory[fps_memory_index] = fps;
	fps_memory_index = (fps_memory_index + 1) % fps_memory_window;

	float fps_sum = 0;
	for (int i = 0; i < fps_memory_window; i++)
		fps_sum += fps_memory[i];

	fps_avg = fps_sum / fps_memory_window;
}
