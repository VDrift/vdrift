#ifndef SOUNDBUFFER_H
#define SOUNDBUFFER_H

#include <string>
#include <ostream>

#include "soundinfo.h"

class SOUNDBUFFER
{
friend class SOUNDSOURCE;
public:
	SOUNDBUFFER();
	
	~SOUNDBUFFER();
	
	bool Load(const std::string & filename, const SOUNDINFO & device_info, std::ostream & error_output);
	
	void Unload();
	
	const SOUNDINFO & GetSoundInfo() const
	{
		return info;
	}
	
	int GetSample16bit(const unsigned int channel, const unsigned int position) const
	{
		return ((short *)sound_buffer)[position * info.channels + (channel - 1) * (info.channels - 1)];
	}
	
	const std::string & GetName() const
	{
		return name;
	}
	
	bool GetLoaded() const
	{
		return (sound_buffer != NULL);
	}
	
private:
	SOUNDINFO info;
	unsigned int size;
	char * sound_buffer;
	std::string name;
};

#endif // SOUNDBUFFER_H
