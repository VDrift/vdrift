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

#ifndef SOUNDINFO_H
#define SOUNDINFO_H

#include <string>
#include <ostream>

struct SoundInfo
{
	std::string name;
	int samples;
	int frequency;
	int bytespersample;
	int channels;

	SoundInfo(int numsamples, int freq, int chan, int bytespersamp) :
		samples(numsamples),
		frequency(freq),
		bytespersample(bytespersamp),
		channels(chan)
	{
		//ctor
	}

	void DebugPrint(std::ostream & out) const
	{
		out << "Samples: " << samples << std::endl;
		out << "Frequency: " << frequency << std::endl;
		out << "Channels: " << channels << std::endl;
		out << "Bits per sample: " << bytespersample*8 << std::endl;
	}

	bool operator==(const SoundInfo & other) const
	{
		return (samples == other.samples &&
				frequency == other.frequency &&
				channels == other.channels &&
				bytespersample == other.bytespersample);
	}
};

#endif // SOUNDINFO_H
