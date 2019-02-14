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

struct SoundInfo
{
	unsigned int samples;
	unsigned int frequency;
	unsigned char channels;
	unsigned char bytespersample;

	SoundInfo(unsigned int numsamples, unsigned int freq, unsigned char chan, unsigned char bytespersamp) :
		samples(numsamples),
		frequency(freq),
		channels(chan),
		bytespersample(bytespersamp)
	{
		//ctor
	}

	template <class Stream>
	void DebugPrint(Stream & out) const
	{
		out << "Samples: " << samples << "\n";
		out << "Frequency: " << frequency << "\n";
		out << "Channels: " << (unsigned)channels << "\n";
		out << "Bits per sample: " << (unsigned)bytespersample*8 << "\n";
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
