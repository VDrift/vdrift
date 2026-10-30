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

#ifndef SOUNDBUFFER_H
#define SOUNDBUFFER_H

#include "soundinfo.h"

#include <iosfwd>
#include <memory>
#include <string>

class SoundBuffer
{
public:
	SoundBuffer();

	bool Load(const std::string & filename, const SoundInfo & sound_device_info, std::ostream & error_output);

	const SoundInfo & GetInfo() const
	{
		return info;
	}

	const char * GetRawBuffer() const
	{
		return sound_buffer.get();
	}

	const std::string & GetName() const
	{
		return name;
	}

private:
	SoundInfo info;
	std::unique_ptr<char[]> sound_buffer;
	std::string name;

	bool LoadWAV(const std::string & filename, const SoundInfo & sound_device_info, std::ostream & error_output);

	bool LoadOGG(const std::string & filename, const SoundInfo & sound_device_info, std::ostream & error_output);

};

#endif // SOUNDBUFFER_H
