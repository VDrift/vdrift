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

#ifndef _FORCEFEEDBACK_H
#define _FORCEFEEDBACK_H

#include <SDL2/SDL_haptic.h>
#include <iosfwd>
#include <string>

class ForceFeedback
{
public:
	ForceFeedback(
		const std::string & device,
		std::ostream & error_output,
		std::ostream & info_output);

	~ForceFeedback();

	void update(
		float force,
		float dt,
		std::ostream & error_output);

	void disable();

private:
	std::string device_name;
	bool enabled;
	bool stop_and_play;
	int device_handle;
	int axis_code;
	int axis_min;
	int axis_max;
	float lastforce;

	SDL_Haptic * haptic;
	SDL_HapticEffect effect;
	int effect_id;
};

#endif // _FORCEFEEDBACK_H
