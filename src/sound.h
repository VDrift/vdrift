#ifndef _SOUND_H
#define _SOUND_H

#include "soundbuffer.h"
#include "soundfilter.h"
#include "tripplebuffer.h"
#include "mathvector.h"
#include "quaternion.h"
#include "memory.h"

#include <vector>
#include <string>
#include <ostream>
#include "stdint.h"

struct SDL_mutex;

struct SOUNDSOURCE
{
	std::tr1::shared_ptr<SOUNDBUFFER> buffer;
	MATHVECTOR<float, 3> position;
	MATHVECTOR<float, 3> velocity;
	float offset;
	float pitch;
	float gain;
	bool is3d;
	bool playing;
	bool loop;
	size_t id;
};

struct SOUNDSAMPLER
{
	static const int denom = 1 << 15;
	static const int max_gain_delta = (denom * 100) / 44100;
	const SOUNDBUFFER * buffer;
	int samples_per_channel;
	int sample_pos;
	int sample_pos_remainder;
	int pitch;
	int gain1;
	int gain2;
	int last_gain1;
	int last_gain2;
	bool playing;
	bool loop;
	size_t id;
};

class SOUND
{
public:
	SOUND();

	~SOUND();

	// init sound device
	bool Init(int buffersize, std::ostream & info, std::ostream & error);

	// get device info
	const SOUNDINFO & GetDeviceInfo() const;

	// return if sound enabled
	bool Enabled() const;

	// disable sound
	void Disable();

	// pause sound
	void Pause(bool value);

	// commit state changes
	void Update();

	// active sources limit can be adjusted at runtime
	void SetMaxActiveSources(size_t value);

	size_t AddSource(std::tr1::shared_ptr<SOUNDBUFFER> buffer, float offset, bool is3d, bool loop);

	void RemoveSource(size_t id);

	void ResetSource(size_t id);

	bool GetSourcePlaying(size_t id) const;

	void SetSourceVelocity(size_t id, float x, float y, float z);

	void SetSourcePosition(size_t id, float x, float y, float z);

	void SetSourceRotation(size_t id, float x, float y, float z, float w);

	void SetSourcePitch(size_t id, float value);

	void SetSourceGain(size_t id, float value);

	void SetListenerVelocity(float x, float y, float z);

	void SetListenerPosition(float x, float y, float z);

	void SetListenerRotation(float x, float y, float z, float w);

	void SetVolume(float value);

private:
	SOUNDINFO deviceinfo;
	SOUNDFILTER volume_filter;
	MATHVECTOR<float, 3> listener_pos;
	MATHVECTOR<float, 3> listener_vel;
	QUATERNION<float> listener_rot;
	bool initdone;
	bool disable;
	bool paused;

	// sound thread message system
	struct GainPitch { int gain1, gain2, pitch; };
	struct NewSampler { const SOUNDBUFFER * buffer; int offset; bool loop; int id; };
	TrippleBuffer<GainPitch> sampler_update;
	TrippleBuffer<NewSampler> sampler_add;
	TrippleBuffer<size_t> sampler_remove;
	TrippleBuffer<size_t> source_remove;
	TrippleBuffer<size_t> source_stop;
	SDL_mutex * sampler_lock;
	SDL_mutex * source_lock;

	// sound sources state
	std::vector<size_t> sources_active;
	std::vector<SOUNDSOURCE> sources;
	size_t max_active_sources;
	size_t sources_num;

	// sound thread state
	std::vector<int> buffer1, buffer2;
	std::vector<SOUNDSAMPLER> samplers;
	size_t samplers_num;

	// main thread methods
	void GetSourceChanges();

	void ProcessSourceStop();

	void ProcessSourceRemove();

	void ProcessSources();

	void LimitActiveSources();

	void SetSamplerChanges();

	// sound thread methods
	void GetSamplerChanges();

	void ProcessSamplerUpdate();

	void ProcessSamplers(uint8_t *stream, int len);

	void ProcessSamplerRemove();

	void ProcessSamplerAdd();

	void SetSourceChanges();

	void Callback16bitStereo(void *sound, uint8_t *stream, int len);

	static void CallbackWrapper(void *sound, uint8_t *stream, int len);
};

#endif
