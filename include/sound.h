#ifndef _SOUND_H

#include "soundsource.h"
#include "quaternion.h"
#include "mathvector.h"

#include <iostream>
#include <string>
#include <list>
#include <map>
#include <fstream>
#include "stdint.h"

struct SDL_mutex;

class SOUND
{
public:
	SOUND();
	
	~SOUND();
	
	bool Init(int buffersize, std::ostream & info_output, std::ostream & error_output);
	
	void Callback16bitstereo(void *unused, uint8_t *stream, int len);
	
	void Pause(const bool pause_on);
	
	void AddSource(SOUNDSOURCE & newsource) {if (disable) return; LockSourceList();sourcelist.push_back(&newsource);UnlockSourceList();}
	
	void RemoveSource(SOUNDSOURCE * todel);
	
	void Clear() {sourcelist.clear();}
	
	void SetMasterVolume(float newvol) {volume_filter.SetFilterOrder0(newvol*0.25);}
	
	void Disable() {disable=true;}
	
	void SetListener(const MATHVECTOR <float, 3> & npos, const QUATERNION <float> & nrot, const MATHVECTOR <float, 3> & nvel) {lpos = npos; lrot = nrot; lvel = nvel;}
	
	bool Enabled() const {return !disable;}
	
	const SOUNDINFO & GetDeviceInfo() {return deviceinfo;}
	
private:
	bool initdone;
	bool paused;

	SOUNDINFO deviceinfo;
	std::list <SOUNDSOURCE *> sourcelist;

	float gain_estimate;
	SOUNDFILTER volume_filter;
	bool disable;

	MATHVECTOR <float, 3> lpos;
	MATHVECTOR <float, 3> lvel;
	QUATERNION <float> lrot;
	
	SDL_mutex * sourcelistlock;
	
	void DetermineActiveSources(std::list <SOUNDSOURCE *> & active_sourcelist, std::list <SOUNDSOURCE *> & inaudible_sourcelist) const;
	void Compute3DEffects(std::list <SOUNDSOURCE *> & sources, const MATHVECTOR <float, 3> & listener_pos, const QUATERNION <float> & listener_rot) const;
	void CollectGarbage();
	void LockSourceList();
	void UnlockSourceList();
};

#define _SOUND_H
#endif
