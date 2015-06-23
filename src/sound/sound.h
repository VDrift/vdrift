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

#ifndef _SOUND_H
#define _SOUND_H

#include "soundbuffer.h"
#include "soundfilter.h"
#include "tripplebuffer.h"
#include "mathvector.h"
#include "quaternion.h"

#include <memory>
#include <iosfwd>
#include <vector>

struct SDL_mutex;

class Sound
{
public:
	Sound();

	~Sound();

	// init sound device
	bool Init(int buffersize, std::ostream & info, std::ostream & error);

	// get device info
	const SoundInfo & GetDeviceInfo() const;

	// return if sound enabled
	bool Enabled() const;

	// disable sound
	void Disable();

	// active sources limit can be adjusted at runtime
	void SetMaxActiveSources(size_t value);

	// attenuation: y = a * (x - b)^c + d
	void SetAttenuation(const float attenuation[4]);

	size_t AddSource(std::shared_ptr<SoundBuffer> buffer, float offset, bool is3d, bool loop);

	void RemoveSource(size_t id);

	void ResetSource(size_t id);

	bool GetSourcePlaying(size_t id) const;

	void SetSourceVelocity(size_t id, float x, float y, float z);

	void SetSourcePosition(size_t id, float x, float y, float z);

	void SetSourcePitch(size_t id, float value);

	void SetSourceGain(size_t id, float value);

	void SetListenerVelocity(float x, float y, float z);

	void SetListenerPosition(float x, float y, float z);

	void SetListenerRotation(float x, float y, float z, float w);

	void SetVolume(float value);

	// commit state changes
	void Update(bool pause);

private:
	std::ostream * log_error;
	SoundInfo deviceinfo;
	Vec3 listener_pos;
	Vec3 listener_vel;
	Quat listener_rot;
	float attenuation[4];
	float sound_volume;
	bool initdone;
	bool disable;
	bool set_pause;

	// state structs
	struct SourceActive
	{
		bool operator<(const SourceActive & other) const;
		int gain, id;
	};

	struct Source
	{
		std::shared_ptr<SoundBuffer> buffer;
		Vec3 position;
		Vec3 velocity;
		float offset;
		float pitch;
		float gain;
		bool is3d;
		bool playing;
		bool loop;
		size_t id;
	};

	struct Sampler
	{
		static const int denom = 32768;
		static const int max_gain_delta = (denom * 173) / 44100; // 256 samples from min to max gain
		const SoundBuffer * buffer;
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

	// message structs
	struct SamplerAdd
	{
		const SoundBuffer * buffer;
		int offset;
		bool loop;
		int id;
	};

	struct SamplerSet
	{
		int gain1, gain2, pitch;
	};

	struct SamplersUpdate
	{
		std::vector<SamplerSet> sset;
		std::vector<SamplerAdd> sadd;
		std::vector<size_t> sremove;
		size_t id;
		bool empty() const;
	};

	// sound thread message system
	TrippleBuffer<SamplersUpdate> samplers_update;
	TrippleBuffer<std::vector<size_t> > sources_stop;
	SDL_mutex * sampler_lock;
	SDL_mutex * source_lock;

	// sound sources state
	std::vector<SourceActive> sources_active;
	std::vector<size_t> sources_remove;
	std::vector<Source> sources;
	size_t max_active_sources;
	size_t sources_num;
	size_t update_id;
	bool sources_pause;

	// sound thread state
	std::vector<int> buffer1, buffer2;
	std::vector<Sampler> samplers;
	size_t samplers_num;
	bool samplers_pause;
	bool samplers_fade;

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

	void ProcessSamplers(unsigned char *stream, int len);

	void ProcessSamplerRemove();

	void ProcessSamplerAdd();

	void SetSourceChanges();

	void Callback16bitStereo(void *sound, unsigned char *stream, int len);

	static void CallbackWrapper(void *sound, unsigned char *stream, int len);

	static void SampleAndAdvanceWithPitch16bit(
		Sampler & sampler, int * chan1, int * chan2, int len);

	static void AdvanceWithPitch(Sampler & sampler, int len);
};

#endif
