#ifndef SOUNDSOURCE_H
#define SOUNDSOURCE_H

#include "soundbuffer.h"
#include "soundfilter.h"
#include "mathvector.h"
#include "quaternion.h"
#include "memory.h"
#include <cassert>

class SOUNDSOURCE
{
public:
	SOUNDSOURCE();

	void SetBuffer(std::tr1::shared_ptr<SOUNDBUFFER> newbuf)
	{
		buffer = newbuf;
		samples_per_channel = buffer->info.samples / buffer->info.channels;
		samples = samples_per_channel * buffer->info.channels;
		Loop(false);
		Stop();
	}

	void SampleAndAdvanceWithPitch16bit(int * chan1, int * chan2, int len);

	void IncrementWithPitch(int len);

	void SeekToSample(int n)
	{
		assert(buffer);
		assert(n * buffer->info.channels < buffer->info.samples);
		sample_pos = n;
		sample_pos_remainder = 0;
	}

	void SetAutoDelete(bool value)
	{
		autodelete = value;
	}

	bool GetAutoDelete() const
	{
		return autodelete;
	}

	void SetGain(float value)
	{
		gain = value;
	}

	void SetPitch(float value)
	{
		pitch = value * denom;
	}

	void SetPosition(float x, float y, float z)
	{
		position[0] = x;
		position[1] = y;
		position[2] = z;
	}

	const MATHVECTOR <float, 3> & GetPosition() const
	{
		return position;
	}

	void SetVelocity(float x, float y, float z)
	{
		velocity[0] = x;
		velocity[1] = y;
		velocity[2] = z;
	}

	const MATHVECTOR <float, 3> & GetVelocity() const
	{
		return velocity;
	}

	void Enable3D(bool value)
	{
		effects3d = value;
	}

	bool Get3DEffects() const
	{
		return effects3d;
	}

	void SetComputedGain(float cpg1, float cpg2)
	{
		computed_gain1 = cpg1 * denom;
		computed_gain2 = cpg2 * denom;
	}

	float GetGain() const
	{
		return gain;
	}

	void Loop(bool value)
	{
		loop = value;
	}

	void Reset()
	{
		sample_pos = sample_pos_remainder = 0;
	}

	void Stop()
	{
		Pause();
		Reset();
	}

	void Pause()
	{
		playing = false;
	}

	void Play()
	{
		playing = true;
	}

	bool Audible() const
	{
		return playing && (gain > 0);
	}

	const std::string GetName() const
	{
		if (buffer == NULL)
			return "NULL";
		else
			return buffer->GetName();
	}

	const SOUNDBUFFER & GetSoundBuffer() const
	{
		return *buffer;
	}

	SOUNDFILTER & AddFilter()
	{
		filters.push_back(SOUNDFILTER());
		return filters.back();
	}

	SOUNDFILTER & GetFilter(unsigned n)
	{
		assert(n < filters.size());
		return filters[n];
	}

	int NumFilters() const
	{
		return filters.size();
	}

	void ClearFilters()
	{
		filters.clear();
	}

	bool operator<(const SOUNDSOURCE & other) const
	{
		return std::max(last_computed_gain1, last_computed_gain2) <
			std::max(other.last_computed_gain1, last_computed_gain2);
	}

private:
	static const int denom = 1 << 15;
	static const int max_gain_delta = (denom * 100) / 44100;

	int samples_per_channel;
	int samples;
	int sample_pos;
	int sample_pos_remainder;
	int pitch;
	int computed_gain1;
	int computed_gain2;
	int last_computed_gain1;
	int last_computed_gain2;

	bool playing;
	bool loop;
	bool autodelete;
	bool effects3d;

	MATHVECTOR <float, 3> position;
	MATHVECTOR <float, 3> velocity;
	float gain;

	std::tr1::shared_ptr<SOUNDBUFFER> buffer;
	std::vector<SOUNDFILTER> filters;
};

#endif // SOUNDSOURCE_H
