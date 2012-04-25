#ifndef _SOUND_H

#include "soundsource.h"
#include "quaternion.h"
#include "mathvector.h"

#include <string>
#include <list>
#include <ostream>
#include "stdint.h"

struct SDL_mutex;

class SOUND
{
public:
	SOUND();

	~SOUND();

	bool Init(int buffersize, std::ostream & info_output, std::ostream & error_output);

	void Callback16bitstereo(void *unused, uint8_t *stream, int len);

	void Pause(bool value);

	// use to pass a source list at once, will empty passed list
	void AddSources(std::list<SOUNDSOURCE *> & sources);

	// add a single source
	void AddSource(SOUNDSOURCE & source);

	// remove a single source, O(N)
	void RemoveSource(SOUNDSOURCE * todel);

	// remove all sources
	void Clear();

	void SetMasterVolume(float value)
	{
		volume_filter.SetFilterOrder0(value * 0.25);
	}

	void Disable()
	{
		disable = true;
	}

	void SetListener(
		const MATHVECTOR <float, 3> & pos,
		const QUATERNION <float> & rot,
		const MATHVECTOR <float, 3> & vel)
	{
		listener_pos = pos;
		listener_rot = rot;
		listener_vel = vel;
	}

	bool Enabled() const
	{
		return !disable;
	}

	const SOUNDINFO & GetDeviceInfo() const
	{
		return deviceinfo;
	}

	void DetermineActiveSources();

	void Compute3DEffects() const;

	void LimitActiveSources();

	void CollectGarbage();

private:
	bool initdone;
	bool paused;
	bool disable;

	SOUNDINFO deviceinfo;
	SOUNDFILTER volume_filter;
	std::list <SOUNDSOURCE *> sourcelist;
	MATHVECTOR <float, 3> listener_pos;
	MATHVECTOR <float, 3> listener_vel;
	QUATERNION <float> listener_rot;
	size_t max_active_sources;

	// cache
	std::vector <SOUNDSOURCE *> sources_active;
	std::vector <SOUNDSOURCE *> sources_inactive;
	std::vector <int> buffer1, buffer2;
	size_t activemax, inactivemax;

	SDL_mutex * sourcelistlock;
	void LockSourceList();
	void UnlockSourceList();
};

#define _SOUND_H
#endif
