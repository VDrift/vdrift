#ifndef SOUNDBUFFER_H
#define SOUNDBUFFER_H

#include "soundinfo.h"

#include <string>
#include <ostream>

class SOUNDBUFFER
{
public:
	SOUNDBUFFER() :
		info(0, 0, 0, 0),
		loaded(false),
		sound_buffer(0)
	{
		// ctor
	}

	~SOUNDBUFFER()
	{
		Unload();
	}

	bool Load(const std::string & filename, const SOUNDINFO & sound_device_info, std::ostream & error_output)
	{
		if (filename.find(".wav") != std::string::npos)
			return LoadWAV(filename, sound_device_info, error_output);
		else if (filename.find(".ogg") != std::string::npos)
			return LoadOGG(filename, sound_device_info, error_output);
		else
		{
			error_output << "Unable to determine file type from filename: " << filename << std::endl;
			return false;
		}
	}

	void Unload()
	{
		if (loaded && sound_buffer)
			delete [] sound_buffer;
		sound_buffer = 0;
	}

	const SOUNDINFO & GetInfo() const
	{
		return info;
	}

	inline int GetSample16bit(const unsigned int channel, const unsigned int position) const
	{
		return ((short *)sound_buffer)[position * info.channels + (channel - 1) * (info.channels - 1)];
	}

	const char * GetRawBuffer() const
	{
		return sound_buffer;
	}

	const std::string & GetName() const
	{
		return name;
	}

	bool GetLoaded() const
	{
		return loaded;
	}

private:
	SOUNDINFO info;
	unsigned int size;
	bool loaded;
	char * sound_buffer;
	std::string name;

	bool LoadWAV(const std::string & filename, const SOUNDINFO & sound_device_info, std::ostream & error_output);

	bool LoadOGG(const std::string & filename, const SOUNDINFO & sound_device_info, std::ostream & error_output);

};

#endif // SOUNDBUFFER_H
