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
	void SetBuffer(std::tr1::shared_ptr<SOUNDBUFFER> newbuf) {buffer=newbuf;Loop(false);Stop();}
	void SampleAndAdvanceWithPitch16bit(int * chan1, int * chan2, int len);
	void IncrementWithPitch(int num);
	void Increment(int num);
	void SampleAndAdvance16bit(int * chan1, int * chan2, int len);
	void Sample16bit(unsigned int peekoffset, int & chan1, int & chan2);
	void Advance(unsigned int offset);
	void SeekToSample(const unsigned int newpos) {assert(buffer);assert((int)newpos < buffer->info.samples/buffer->info.channels);sample_pos = newpos;sample_pos_remainder=0;}
	void SetAutoDelete(const bool newauto) {autodelete = newauto;}
	bool GetAutoDelete() const {return autodelete;}
	void SetGain(const float newgain) {gain = newgain;}
	void SetPitch(const float newpitch) {pitch = newpitch;}
	void SetGainSmooth(const float newgain, const float dt);
	void SetPitchSmooth(const float newpitch, const float dt);
	void SetPosition(float x, float y, float z) {position[0]=x;position[1]=y;position[2]=z;}
	const MATHVECTOR <float, 3> & GetPosition() const {return position;}
	void SetVelocity(float x, float y, float z) {velocity[0]=x;velocity[1]=y;velocity[2]=z;}
	const MATHVECTOR <float, 3> & GetVelocity() const {return velocity;}
	void Enable3D(bool new3d) {effects3d = new3d;}
	bool Get3DEffects() const {return effects3d;}
	void SetComputationResults(float cpg1, float cpg2) //{last_computed_gain1 = computed_gain1;last_computed_gain2 = computed_gain2;computed_gain1 = cpg1;computed_gain2 = cpg2;last_sample_pos = sample_pos;}
		{computed_gain1 = cpg1;computed_gain2 = cpg2;}
	float GetGain() const {return gain;}
	float ComputedGain(const int channel) const {if (channel == 1) return computed_gain1; else return computed_gain2;}
	void SetRelativeGain(const float relgain) {relative_gain = relgain;}
	float GetRelativeGain() {return relative_gain;}

	void Loop(const bool newloop) {if (newloop) loop = 1; else loop = 0;}
	void Reset() {sample_pos = 0; sample_pos_remainder=0;}
	void Stop() {playing = 0;Reset();}
	void Pause() {playing = 0;}
	void Play() {playing = 1;}
	bool Audible() const;
	const std::string GetName() const {if (buffer == NULL) return "NULL"; else return buffer->GetName();}

	const SOUNDBUFFER & GetSoundBuffer() const {return *buffer;}
	SOUNDFILTER & AddFilter() {{SOUNDFILTER newfilt;filters.push_back(newfilt);}return filters.back();}
	SOUNDFILTER & GetFilter(int num);
	int NumFilters() const {return filters.size();}
	void ClearFilters() {filters.clear();}

private:
	unsigned int sample_pos;
	float sample_pos_remainder;
	int playing;
	int loop;
	bool autodelete;
	float gain;
	float pitch;
	float last_pitch;
	MATHVECTOR <float, 3> position;
	MATHVECTOR <float, 3> velocity;

	float computed_gain1;
	float computed_gain2;

	float last_computed_gain1;
	float last_computed_gain2;

	bool effects3d;

	float relative_gain;

	std::tr1::shared_ptr<SOUNDBUFFER> buffer;
	std::list <SOUNDFILTER> filters;
};

#endif // SOUNDSOURCE_H
