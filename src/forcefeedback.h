#ifndef _FORCEFEEDBACK_H
#define _FORCEFEEDBACK_H

#include <string>
#include <iostream>

#include <SDL/SDL_version.h>
#if SDL_VERSION_ATLEAST(2,0,0)
#include <SDL/SDL_haptic.h>
#elif defined(linux) || defined(__linux)
#include <linux/input.h>
#endif

class FORCEFEEDBACK
{
public:
	FORCEFEEDBACK(
		std::string device,
		std::ostream & error_output,
		std::ostream & info_output);

	~FORCEFEEDBACK();

	void update(
		double force,
		double * position,
		double dt,
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
	double lastforce;

#if SDL_VERSION_ATLEAST(2,0,0)
	SDL_Haptic * haptic;
	SDL_HapticEffect effect;
	int effect_id;
#elif defined(linux) || defined(__linux)
	struct ff_effect effect;
#endif
};

#endif // _FORCEFEEDBACK_H
