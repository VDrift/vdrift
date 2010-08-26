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

class SOUNDBUFFERLIBRARY
{
	private:
		std::string librarypath;
		std::map <std::string, SOUNDBUFFER> buffermap;
		bool FileExists(const std::string & filename)
		{
			std::ifstream f(filename.c_str());
			return f;
		}
		
	public:
		///set the path to the sound buffers, minus the trailing /
		void SetLibraryPath(const std::string & newpath)
		{
			librarypath = newpath;
		}
		
		///buffername is the path to the sound minus the path prefix and file extension postfix
		bool Load(const std::string & buffername, const SOUNDINFO & sound_device_info, std::ostream & error_output)
		{
			std::map <std::string, SOUNDBUFFER>::iterator existing = buffermap.find(buffername);
			if (existing != buffermap.end()) //already loaded
				return true;
			
			//prefer ogg
			std::string filename = librarypath+"/"+buffername+".ogg";
			if (!FileExists(filename))
				filename = librarypath+"/"+buffername+".wav";
			 
			if (!buffermap[buffername].Load(filename, sound_device_info, error_output))
			{
				buffermap.erase(buffername);
				return false;
			}
			else
				return true;
		}
		
		///returns NULL if the buffer isn't found
		const SOUNDBUFFER * Get(const std::string & buffername) const
		{
			std::map <std::string, SOUNDBUFFER>::const_iterator buff = buffermap.find(buffername);
			if (buff != buffermap.end())
				return &(buff->second);
			else
				return NULL;
		}
};

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
