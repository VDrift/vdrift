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

#include "forcefeedback.h"
#include "minmax.h"

#include <ostream>
#include <cstring>

ForceFeedback::ForceFeedback(
	const std::string & device,
	std::ostream & error_output,
	std::ostream & info_output) :
	device_name(device),
	enabled(true),
	lastforce(0),
	haptic(0),
	effect_id(-1)
{
	// Close haptic if already open.
	if (haptic)
	{
		SDL_CloseHaptic(haptic);
		haptic = NULL;
	}

	// Do we have force feedback devices?
	int haptic_num = 0;
	SDL_HapticID * haptics = SDL_GetHaptics(&haptic_num);
	if (haptics == NULL)
	{
		info_output << "No force feedback devices found." << std::endl;
		return;
	}
	info_output << "Number of force feedback devices: " << haptic_num << std::endl;

	SDL_HapticID id = haptics[0];
	SDL_free(haptics);

	// Try to create a haptic device.
	haptic = SDL_OpenHaptic(id);
	if (!haptic)
	{
		error_output << "Failed to initialize force feedback device: " << SDL_GetError();
		return;
	}

	// Check for constant force support.
	unsigned int haptic_query = SDL_GetHapticFeatures(haptic);
	if (!(haptic_query & SDL_HAPTIC_CONSTANT))
	{
		error_output << "Constant force feedback not supported: " << SDL_GetError();
		return;
	}

	// Create the effect.
	memset(&effect, 0, sizeof(SDL_HapticEffect) ); // 0 is safe default
	effect.type = SDL_HAPTIC_CONSTANT;
	effect.constant.direction.type = SDL_HAPTIC_CARTESIAN; // Using cartesian direction encoding.
	effect.constant.direction.dir[0] = 1; // X position
	effect.constant.direction.dir[1] = 0; // Y position
	effect.constant.length = 0xffff;
	effect.constant.delay = 0;
	effect.constant.button = 0;
	effect.constant.interval = 0;
	effect.constant.level = 0;
	effect.constant.attack_length = 0;
	effect.constant.attack_level = 0;
	effect.constant.fade_length = 0;
	effect.constant.fade_level = 0;

	// Upload the effect.
	effect_id = SDL_CreateHapticEffect(haptic, &effect);
	if (effect_id == -1)
	{
		error_output << "Failed to initialize force feedback effect: " << SDL_GetError();
		return;
	}

	info_output << "Force feedback enabled." << std::endl;
}

ForceFeedback::~ForceFeedback()
{
	if (haptic)
		SDL_CloseHaptic(haptic);
}

void ForceFeedback::update(
	float force,
	float /*dt*/,
	std::ostream & error_output)
{
	if (!enabled || !haptic || (effect_id == -1))
		return;

	// Clamp force.
	force = Clamp(force, -1.0f, 1.0f);

	// Low pass filter.
	lastforce = (lastforce + force) * 0.5f;

	// Update effect.
	effect.constant.level = Sint16(lastforce * 32767);
	int new_effect_id = SDL_UpdateHapticEffect(haptic, effect_id, &effect);
	if (new_effect_id == -1)
	{
		error_output << "Failed to update force feedback effect: " << SDL_GetError();
		return;
	}
	else
	{
		effect_id = new_effect_id;
	}

	// Run effect.
	if (SDL_RunHapticEffect(haptic, effect_id, 1) == false)
	{
		error_output << "Failed to run force feedback effect: " << SDL_GetError();
		return;
	}
}

void ForceFeedback::disable()
{
	enabled = false;
}
