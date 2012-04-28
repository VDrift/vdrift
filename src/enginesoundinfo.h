#ifndef _ENGINESOUNDINFO_H
#define _ENGINESOUNDINFO_H

struct ENGINESOUNDINFO
{
	float minrpm, maxrpm, naturalrpm, fullgainrpmstart, fullgainrpmend;
	enum { POWERON, POWEROFF, BOTH } power;
	size_t sound_source;

	ENGINESOUNDINFO() :
		minrpm(1.0),
		maxrpm(100000.0),
		naturalrpm(7000.0),
		fullgainrpmstart(minrpm),
		fullgainrpmend(maxrpm),
		power(BOTH),
		sound_source(0)
	{
		// ctor
	}

	bool operator < (const ENGINESOUNDINFO & other) const
	{
		return minrpm < other.minrpm;
	}
};

#endif // _ENGINESOUNDINFO_H
