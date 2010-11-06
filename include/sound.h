#ifndef _SOUND_H

#include "soundsource.h"
#include "quaternion.h"
#include "mathvector.h"

#include <iostream>
#include <string>
#include <list>
#include <map>
#include <fstream>

struct SDL_mutex;

class SOUND
{
public:
	SOUND();
	
	~SOUND();
	
	bool Init(int buffersize, std::ostream & info_output, std::ostream & error_output);
	
	void Pause(const bool pause_on);
	
	void AddSource(SOUNDSOURCE & newsource)
	{
		if (disable) return;
		LockSourceList();
		sourcelist.push_back(&newsource);
		UnlockSourceList();
	}
	
	void RemoveSource(SOUNDSOURCE * todel);
	
	void Clear()
	{
		sourcelist.clear();
	}
	
	void SetMasterVolume(float newvol)
	{
		volume_filter.SetFilterOrder0(newvol * 0.25);
	}
	
	void SetListener(const MATHVECTOR <float, 3> & npos, const QUATERNION <float> & nrot, const MATHVECTOR <float, 3> & nvel)
	{
		lpos = npos;
		lrot = nrot;
		lvel = nvel;
	}
	
	void Disable()
	{
		disable = true;
	}
	
	bool Enabled() const
	{
		return !disable;
	}
	
	const SOUNDINFO & GetDeviceInfo()
	{
		return deviceinfo;
	}
	
	void Callback16bitStereo(void *unused, unsigned char *stream, int len);

private:
	bool initdone;
	bool paused;

	SOUNDINFO deviceinfo;
	std::list <SOUNDSOURCE *> sourcelist;

	float gain_estimate;
	SOUNDFILTER volume_filter;
	bool disable;

	MATHVECTOR <float, 3> lpos;
	QUATERNION <float> lrot;
	MATHVECTOR <float, 3> lvel;
	
	SDL_mutex * sourcelistlock;
	
	void DetermineActiveSources(std::list <SOUNDSOURCE *> & active_sourcelist, std::list <SOUNDSOURCE *> & inaudible_sourcelist) const;
	void Compute3DEffects(std::list <SOUNDSOURCE *> & sources, const MATHVECTOR <float, 3> & listener_pos, const QUATERNION <float> & listener_rot) const;
	void CollectGarbage();
	void LockSourceList();
	void UnlockSourceList();
};

#define _SOUND_H
#endif
