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

#include <map>
#include <list>
#include <vector>
#include <iostream>
#include <numeric>
#include <cassert>

using std::vector;
using std::map;
using std::list;
using std::endl;

EventSystem::EventSystem() :
	lasttick(0),
	dt(0),
	quit(false),
	mousex(0),
	mousey(0),
	mousexrel(0),
	mouseyrel(0),
	fps_memory_window(10)
{
	// ctor
}

EventSystem::~EventSystem()
{
	// dtor
}

void EventSystem::Init(std::ostream & info_output)
{
	const int num_joysticks = SDL_NumJoysticks();

	info_output << num_joysticks << " joystick";
	if (num_joysticks != 1)
		info_output << "s";
	info_output << " found";
	if (num_joysticks > 0)
		info_output << ":" << endl;
	else
		info_output << "." << endl;

	SDL_JoystickEventState(SDL_ENABLE);

	joysticks.resize(num_joysticks);
	for (int i = 0; i < num_joysticks; ++i)
	{
		SDL_Joystick * jp = SDL_JoystickOpen(i);
		assert(jp);

		const int id = SDL_JoystickInstanceID(jp);
		assert(id >= 0 && id < num_joysticks);

		joysticks[id] = Joystick(jp, SDL_JoystickNumAxes(jp), SDL_JoystickNumButtons(jp), SDL_JoystickNumHats(jp));

		info_output << "    " << id << " " << SDL_JoystickName(jp) << endl;
	}
}

void EventSystem::BeginFrame()
{
	if (lasttick == 0)
		lasttick = SDL_GetTicks();
	else
	{
		double thistick = SDL_GetTicks();

		dt = (thistick-lasttick)/1000.0;

		/*if (throttle && dt < game.TickPeriod())
		{
			//cout << "throttling: " << lasttick.data << "," << thistick << endl;
			SDL_Delay(10);
			thistick = SDL_GetTicks();
			dt = (thistick-lasttick)/1000.0;
		}*/

		lasttick = thistick;
	}

	RecordFPS(1.0f/dt);
}

void EventSystem::ProcessEvents()
{
	SDL_Event event;

	AgeToggles <SDL_Keycode> (keymap);
	AgeToggles <int> (mbutmap);
	for (std::vector <Joystick>::iterator i = joysticks.begin(); i != joysticks.end(); i++)
	{
		i->AgeToggles();
	}

	while ( SDL_PollEvent( &event ) )
	{
		switch( event.type )
		{
		case SDL_MOUSEMOTION:
			HandleMouseMotion(event.motion.x, event.motion.y, event.motion.xrel, event.motion.yrel);
			break;
		case SDL_MOUSEBUTTONDOWN:
			HandleMouseButton(DOWN, event.button.button);
			break;
		case SDL_MOUSEBUTTONUP:
			HandleMouseButton(UP, event.button.button);
			break;
		case SDL_KEYDOWN:
			HandleKey(DOWN, event.key.keysym.sym);
			break;
		case SDL_KEYUP:
			HandleKey(UP, event.key.keysym.sym);
			break;
		case SDL_JOYBUTTONDOWN:
			assert(size_t(event.jbutton.which) < joysticks.size()); //ensure the event came from a known joystick
			joysticks[event.jbutton.which].SetButton(event.jbutton.button, true);
			break;
		case SDL_JOYBUTTONUP:
			assert(size_t(event.jbutton.which) < joysticks.size()); //ensure the event came from a known joystick
			joysticks[event.jbutton.which].SetButton(event.jbutton.button, false);
			break;
		case SDL_JOYHATMOTION:
			break;
		case SDL_JOYAXISMOTION:
			assert(size_t(event.jaxis.which) < joysticks.size()); //ensure the event came from a known joystick
			joysticks[event.jaxis.which].SetAxis(event.jaxis.axis, event.jaxis.value / 32768.0f);
			//std::cout << "Joy " << (int) event.jaxis.which << " axis " << (int) event.jaxis.axis << " value " << event.jaxis.value / 32768.0f << endl;
			break;
		case SDL_QUIT:
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
	HandleToggle <int> (mbutmap, dir, id);
}

void EventSystem::HandleKey(DirectionEnum dir, SDL_Keycode id)
{
	//if (dir == DOWN) std::cout << "Key #" << (int)id << " pressed" << endl;
	HandleToggle <SDL_Keycode> (keymap, dir, id);
}

EventSystem::ButtonState EventSystem::GetMouseButtonState(int id) const
{
	return GetToggle <int> (mbutmap, id);
}

EventSystem::ButtonState EventSystem::GetKeyState(SDL_Keycode id) const
{
	return GetToggle <SDL_Keycode> (keymap, id);
}

vector <int> EventSystem::GetMousePosition() const
{
	vector <int> o;
	o.reserve(2);
	o.push_back(mousex);
	o.push_back(mousey);
	return o;
}

vector <int> EventSystem::GetMouseRelativeMotion() const
{
	vector <int> o;
	o.reserve(2);
	o.push_back(mousexrel);
	o.push_back(mouseyrel);
	return o;
}

void EventSystem::TestStim(StimEnum stim)
{
	if (stim == STIM_AGE_KEYS)
	{
		AgeToggles <SDL_Keycode> (keymap);
	}
	if (stim == STIM_AGE_MBUT)
	{
		AgeToggles <int> (mbutmap);
	}
	if (stim == STIM_INSERT_KEY_DOWN)
	{
		HandleKey(DOWN, SDLK_t);
	}
	if (stim == STIM_INSERT_KEY_UP)
	{
		HandleKey(UP, SDLK_t);
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
		EventSystem::ButtonState tstate = e.GetKeyState(SDLK_t);
		QT_CHECK(tstate.down && tstate.just_down && !tstate.just_up);

		//check key aging
		e.TestStim(EventSystem::STIM_AGE_KEYS);
		tstate = e.GetKeyState(SDLK_t);
		QT_CHECK(tstate.down && !tstate.just_down && !tstate.just_up);
		e.TestStim(EventSystem::STIM_AGE_KEYS); //age again
		tstate = e.GetKeyState(SDLK_t);
		QT_CHECK(tstate.down && !tstate.just_down && !tstate.just_up);

		//check key removal
		e.TestStim(EventSystem::STIM_INSERT_KEY_UP);
		tstate = e.GetKeyState(SDLK_t);
		QT_CHECK(!tstate.down && !tstate.just_down && tstate.just_up);

		//check key aging
		e.TestStim(EventSystem::STIM_AGE_KEYS);
		tstate = e.GetKeyState(SDLK_t);
		QT_CHECK(!tstate.down && !tstate.just_down && !tstate.just_up);
		e.TestStim(EventSystem::STIM_AGE_KEYS); //age again
		tstate = e.GetKeyState(SDLK_t);
		QT_CHECK(!tstate.down && !tstate.just_down && !tstate.just_up);
	}

	//mouse button stuff
	{
		//check button insertion
		e.TestStim(EventSystem::STIM_INSERT_MBUT_DOWN);
		EventSystem::ButtonState tstate = e.GetMouseButtonState(SDL_BUTTON_LEFT);
		QT_CHECK(tstate.down && tstate.just_down && !tstate.just_up);

		//check button aging
		e.TestStim(EventSystem::STIM_AGE_MBUT);
		tstate = e.GetMouseButtonState(SDL_BUTTON_LEFT);
		QT_CHECK(tstate.down && !tstate.just_down && !tstate.just_up);
		e.TestStim(EventSystem::STIM_AGE_MBUT); //age again
		tstate = e.GetMouseButtonState(SDL_BUTTON_LEFT);
		QT_CHECK(tstate.down && !tstate.just_down && !tstate.just_up);

		//check button removal
		e.TestStim(EventSystem::STIM_INSERT_MBUT_UP);
		tstate = e.GetMouseButtonState(SDL_BUTTON_LEFT);
		QT_CHECK(!tstate.down && !tstate.just_down && tstate.just_up);

		//check button aging
		e.TestStim(EventSystem::STIM_AGE_MBUT);
		tstate = e.GetMouseButtonState(SDL_BUTTON_LEFT);
		QT_CHECK(!tstate.down && !tstate.just_down && !tstate.just_up);
		e.TestStim(EventSystem::STIM_AGE_MBUT); //age again
		tstate = e.GetMouseButtonState(SDL_BUTTON_LEFT);
		QT_CHECK(!tstate.down && !tstate.just_down && !tstate.just_up);
	}

	//mouse motion stuff
	{
		e.TestStim(EventSystem::STIM_INSERT_MOTION);
		vector <int> mpos = e.GetMousePosition();
		QT_CHECK_EQUAL(mpos.size(),2);
		QT_CHECK_EQUAL(mpos[0],50);
		QT_CHECK_EQUAL(mpos[1],55);
		vector <int> mrel = e.GetMouseRelativeMotion();
		QT_CHECK_EQUAL(mrel.size(),2);
		QT_CHECK_EQUAL(mrel[0],2);
		QT_CHECK_EQUAL(mrel[1],1);
	}
}

void EventSystem::RecordFPS(const float fps)
{
	fps_memory.push_back(fps);
	if (fps_memory.size() > fps_memory_window)
		fps_memory.pop_front();

	//ensure no fps memory corruption
	assert(fps_memory.size() <= fps_memory_window);
}

float EventSystem::GetFPS() const
{
	float avg = std::accumulate(fps_memory.begin(), fps_memory.end(), 0);

	if (!fps_memory.empty())
		avg = avg / fps_memory.size();

	return avg;
}
