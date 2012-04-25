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

	void Pause(bool value);

	// use to pass a source list at once, will empty passed list
	void AddSources(std::list<SOUNDSOURCE *> & sources);

	// add a single source
	void AddSource(SOUNDSOURCE & source);

	// remove a single source, O(N)
	void RemoveSource(SOUNDSOURCE * todel);

	// remove all sources
	void Clear();

	// call to commit state changes
	void Update();

	// active sources limit can be adjusted at runtime
	void SetMaxActiveSources(size_t value)
	{
		assert(value > 0);
		max_active_sources = value;
	}

	void SetMasterVolume(float value)
	{
		volume_filter.SetFilterOrder0(value * 0.25);
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

	void Disable()
	{
		disable = true;
	}

	bool Enabled() const
	{
		return !disable;
	}

	const SOUNDINFO & GetDeviceInfo() const
	{
		return deviceinfo;
	}

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

	// sampling buffers
	std::vector <int> buffer1, buffer2;

	// double buffered source lists
	std::vector <SOUNDSOURCE *> sources_active_1, sources_active_2;
	std::vector <SOUNDSOURCE *> sources_inactive_1, sources_inactive_2;
	std::vector <SOUNDSOURCE *> * sources_active_p;
	std::vector <SOUNDSOURCE *> * sources_inactive_p;
	SDL_mutex * sourcelistlock;
	
	void DetermineActiveSources(std::vector <SOUNDSOURCE *> & sources_active, std::vector <SOUNDSOURCE *> & sources_inactive);
	void Compute3DEffects(std::vector <SOUNDSOURCE *> & sources_active) const;
	void LimitActiveSources(std::vector <SOUNDSOURCE *> & sources_active, std::vector <SOUNDSOURCE *> & sources_inactive);
	void CollectGarbage();
	void LockSourceList();
	void UnlockSourceList();
	
	void Callback16bitStereo(void *sound, uint8_t *stream, int len);
	static void CallbackWrapper(void *sound, uint8_t *stream, int len);
};

#define _SOUND_H
#endif
