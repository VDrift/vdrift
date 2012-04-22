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
		const MATHVECTOR <float, 3> & npos,
		const QUATERNION <float> & nrot,
		const MATHVECTOR <float, 3> & nvel)
	{
		lpos = npos;
		lrot = nrot;
		lvel = nvel;
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
	std::vector <int> buffer1, buffer2;

	MATHVECTOR <float, 3> lpos;
	MATHVECTOR <float, 3> lvel;
	QUATERNION <float> lrot;

	SDL_mutex * sourcelistlock;

	void DetermineActiveSources(std::list <SOUNDSOURCE *> & active, std::list <SOUNDSOURCE *> & inactive);
	void Compute3DEffects(std::list <SOUNDSOURCE *> & sources, const MATHVECTOR <float, 3> & listener_pos, const QUATERNION <float> & listener_rot) const;
	void CollectGarbage();
	void LockSourceList();
	void UnlockSourceList();
};

#define _SOUND_H
#endif
