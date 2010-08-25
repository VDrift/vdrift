#ifndef SOUNDINFO_H
#define SOUNDINFO_H

#include <ostream>

struct SOUNDINFO
{
	unsigned int samples;
	unsigned int frequency;
	unsigned int bytespersample;
	unsigned int channels;
	
	SOUNDINFO() :
		samples(0),
		frequency(0),
		bytespersample(0),
		channels(0)
	{
		// ctor
	}
	
	bool operator!=(const SOUNDINFO & other) const
	{
		return (samples != other.samples) ||
			(frequency != other.frequency) ||
			(bytespersample != other.bytespersample) ||
			(channels != other.channels);
	}
	
	void DebugPrint(std::ostream & out) const
	{
		out << "Samples: " << samples << "\n";
		out << "Frequency: " << frequency << "\n";
		out << "Channels: " << channels << "\n";
		out << "Bits per sample: " << bytespersample * 8 << "\n";
	}
};

#endif // SOUNDINFO_H
