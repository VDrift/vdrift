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

#include "sound.h"
#include "coordinatesystem.h"
#include <SDL2/SDL.h>
#include <algorithm>
#include <cassert>

//static std::ofstream logso("logso.txt");
//static std::ofstream logsa("logsa.txt");

template <class T>
static inline T clamp(T val, T min, T max)
{
	return val > min ? (val < max ? val : max) : min;
}

// add item to a compactifying vector
template <class T>
static inline size_t AddItem(T & item, std::vector<T> & items, size_t & item_num)
{
	size_t id = item_num;
	if (id < items.size())
	{
		// reuse free slot
		size_t idn = items[id].id;
		if (idn != id)
		{
			// free slot is redirecting to other item
			assert(idn < id);

			// swap redirected item back
			items[id] = items[idn];

			// use now free slot
			id = idn;
		}
		items[id] = item;
	}
	else
	{
		// add item to new slot
		items.push_back(item);
	}
	items[id].id = id;
	++item_num;

	return id;
}

// remove item from a compactifying vector
template <class T>
static inline void RemoveItem(size_t id, std::vector<T> & items, size_t & item_num)
{
	assert(id < items.size());

	// get item true id
	size_t idn = items[id].id;
	assert(idn < item_num);

	// pop last item
	--item_num;

	// swap last item with current
	size_t idl = items[item_num].id;
	if (idl != item_num)
	{
		// move redirected last item into free slot
		items[idn] = items[item_num];

		// redirect to new item position
		items[idl].id = idn;

		// invalidate old redirection
		items[item_num].id = item_num;
	}
	else
	{
		// move last item into free slot
		items[idn] = items[item_num];

		// redirect to new item position
		items[item_num].id = idn;
	}
	if (id != idn)
	{
		// invalidate redirecting item
		items[id].id = id;
	}
}

template <class T>
static inline T & GetItem(size_t id, std::vector<T> & items, size_t item_num)
{
	assert(id < items.size());
	size_t idn = items[id].id;
	assert(idn < item_num);
	return items[idn];
}

template <class T>
static inline const T & GetItem(size_t id, const std::vector<T> & items, size_t item_num)
{
	assert(id < items.size());
	size_t idn = items[id].id;
	assert(idn < item_num);
	return items[idn];
}

bool Sound::SourceActive::operator<(const Sound::SourceActive & other) const
{
	// reverse op as partial sort sorts for the smallest elemets
	return this->gain > other.gain;
}

bool Sound::SamplersUpdate::empty() const
{
	return sset.empty() && sadd.empty() && sremove.empty();
}

Sound::Sound() :
	log_error(0),
	deviceinfo(0, 0, 0, 0),
	sound_volume(0),
	initdone(false),
	disable(false),
	set_pause(true),
	sampler_lock(0),
	source_lock(0),
	max_active_sources(64),
	sources_num(0),
	update_id(0),
	sources_pause(true),
	samplers_num(0),
	samplers_pause(true)
{
	attenuation[0] =  0.9146065;
	attenuation[1] =  0.2729276;
	attenuation[2] = -0.2313740;
	attenuation[3] = -0.2884304;

	sources.reserve(64);
	samplers.reserve(64);
}

Sound::~Sound()
{
	if (initdone)
		SDL_CloseAudio();

	if (sampler_lock)
		SDL_DestroyMutex(sampler_lock);

	if (source_lock)
		SDL_DestroyMutex(source_lock);
}

bool Sound::Init(int buffersize, std::ostream & info_output, std::ostream & error_output)
{
	if (disable || initdone)
		return false;

	sampler_lock = SDL_CreateMutex();
	source_lock = SDL_CreateMutex();

	SDL_AudioSpec desired, obtained;

	desired.freq = 44100;
	desired.format = AUDIO_S16SYS;
	desired.samples = buffersize;
	desired.callback = Sound::CallbackWrapper;
	desired.userdata = this;
	desired.channels = 2;

	if (SDL_OpenAudio(&desired, &obtained) < 0)
	{
		error_output << "Error opening audio device, disabling sound." << std::endl;
		Disable();
		return false;
	}

	int frequency = obtained.freq;
	int channels = obtained.channels;
	int samples = obtained.samples;
	int bytespersample = 2;
	if (obtained.format == AUDIO_U8 || obtained.format == AUDIO_S8)
	{
		bytespersample = 1;
	}

	if (obtained.format != desired.format)
	{
		error_output << "Obtained audio format isn't the same as the desired format, disabling sound." << std::endl;
		Disable();
		return false;
	}

	std::ostringstream dout;
	dout << "Obtained audio device:" << std::endl;
	dout << "Frequency: " << frequency << std::endl;
	dout << "Format: " << obtained.format << std::endl;
	dout << "Bits per sample: " << bytespersample * 8 << std::endl;
	dout << "Channels: " << channels << std::endl;
	dout << "Silence: " << (int) obtained.silence << std::endl;
	dout << "Samples: " << samples << std::endl;
	dout << "Size: " << (int) obtained.size << std::endl;
	info_output << "Sound initialization information:" << std::endl << dout.str();
	if (bytespersample != 2 || obtained.channels != desired.channels || obtained.freq != desired.freq)
	{
		error_output << "Sound interface did not create a 44.1kHz, 16 bit, stereo device as requested.  Disabling sound." << std::endl;
		Disable();
		return false;
	}

	deviceinfo = SoundInfo(samples, frequency, channels, bytespersample);
	log_error = &error_output;
	initdone = true;
	SetVolume(1.0);

	// enable sound, run callback
	SDL_PauseAudio(false);

	return true;
}

const SoundInfo & Sound::GetDeviceInfo() const
{
	return deviceinfo;
}

bool Sound::Enabled() const
{
	return !disable;
}

void Sound::Disable()
{
	disable = true;
}

void Sound::SetMaxActiveSources(size_t value)
{
	max_active_sources = value;
}

void Sound::SetAttenuation(const float nattenuation[4])
{
	attenuation[0] = nattenuation[0];
	attenuation[1] = nattenuation[1];
	attenuation[2] = nattenuation[2];
	attenuation[3] = nattenuation[3];
}

size_t Sound::AddSource(std::shared_ptr<SoundBuffer> buffer, float offset, bool is3d, bool loop)
{
	Source src;
	src.buffer = buffer;
	src.position.Set(0, 0, 0);
	src.velocity.Set(0, 0, 0);
	src.offset = offset;
	src.pitch = 1.0;
	src.gain = 0.0;
	src.is3d = is3d;
	src.playing = true;
	src.loop = loop;
	size_t id = AddItem(src, sources, sources_num);

	// notify sound thread
	SamplerAdd ns;
	ns.buffer = buffer.get();
	ns.offset = offset * Sampler::denom;
	ns.loop = loop;
	ns.id = -1;
	samplers_update.getFirst().sadd.push_back(ns);

	//*log_error << "Add sound source: " << id << " " << buffer->GetName() << std::endl;
	return id;
}

void Sound::RemoveSource(size_t id)
{
	samplers_update.getFirst().sremove.push_back(id);
	sources_remove.push_back(id);
}

void Sound::ResetSource(size_t id)
{
	size_t idn = sources[id].id;
	Source & src = sources[idn];
	src.playing = true;

	// notify sound thread
	SamplerAdd ns;
	ns.buffer = src.buffer.get();
	ns.offset = src.offset * Sampler::denom;
	ns.loop = src.loop;
	ns.id = idn;
	samplers_update.getFirst().sadd.push_back(ns);
}

bool Sound::GetSourcePlaying(size_t id) const
{
	return GetItem(id, sources, sources_num).playing;
}

void Sound::SetSourceVelocity(size_t id, float x, float y, float z)
{
	GetItem(id, sources, sources_num).velocity.Set(x, y, z);
}

void Sound::SetSourcePosition(size_t id, float x, float y, float z)
{
	GetItem(id, sources, sources_num).position.Set(x, y, z);
}

void Sound::SetSourcePitch(size_t id, float value)
{
	GetItem(id, sources, sources_num).pitch = value;
}

void Sound::SetSourceGain(size_t id, float value)
{
	GetItem(id, sources, sources_num).gain = value;
}

void Sound::SetListenerVelocity(float x, float y, float z)
{
	listener_vel.Set(x, y, z);
}

void Sound::SetListenerPosition(float x, float y, float z)
{
	listener_pos.Set(x, y, z);
}

void Sound::SetListenerRotation(float x, float y, float z, float w)
{
	listener_rot.Set(x, y, z, w);
}

void Sound::SetVolume(float value)
{
	sound_volume = value;
}

void Sound::Update(bool pause)
{
	if (disable) return;

	set_pause = pause;

	// get source stop messages from sound thread
	GetSourceChanges();

	// process source stop messages
	ProcessSourceStop();

	// ProcessSourceAdd is implicit

	// calculate sampler changes from sources
	ProcessSources();
/*
	logso << "id: " <<samplers_update.getFirst().id;
	logso << " add: " << samplers_update.getFirst().sadd.size();
	logso << " del: " << samplers_update.getFirst().sremove.size();
	logso << " set: " << samplers_update.getFirst().sset.size();
	logso << " sources: " << sources_num;
	logso << std::endl;
*/
	size_t old_update_id = update_id;

	// commit sampler changes to sound thread
	SetSamplerChanges();

	// use update id to determine whether sampler updates have been comitted
	if (old_update_id != update_id)
	{
		ProcessSourceRemove();
	}
}

void Sound::GetSourceChanges()
{
	SDL_LockMutex(source_lock);
	if (!sources_stop.getSecond().empty())
	{
		sources_stop.swapFirst();
	}
	SDL_UnlockMutex(source_lock);
}

void Sound::ProcessSourceStop()
{
	std::vector<size_t> & sstop = sources_stop.getFirst();
	for (size_t i = 0; i < sstop.size(); ++i)
	{
		size_t id = sstop[i];
		size_t idn = sources[id].id;
		if (idn < sources_num)
		{
			// if source is still there, stop playing
			// this still might cause issues if the source has been replaced
			// will need unique identifiers eventually
			sources[idn].playing = false;
		}
	}
	sstop.clear();
}

void Sound::ProcessSourceRemove()
{
	for (size_t i = 0; i < sources_remove.size(); ++i)
	{
		size_t id = sources_remove[i];
		RemoveItem(id, sources, sources_num);
	}
	sources_remove.clear();
}

void Sound::ProcessSources()
{
	std::vector<SamplerSet> & supdate = samplers_update.getFirst().sset;
	supdate.resize(sources_num);

	sources_active.clear();
	for (size_t i = 0; i < sources_num; ++i)
	{
		Source & src = sources[i];
		if (!src.playing) continue;

		float gain1 = 0.0, gain2 = 0.0;
		if (src.gain > 0)
		{
			if (src.is3d)
			{
				Vec3 relvec = src.position - listener_pos;
				float len = relvec.Magnitude();
				if (len < 0.1f) len = 0.1f;

				// distance attenuation
				// y = a * (x - b)^c + d
				float cgain = attenuation[0] * powf(len - attenuation[1], attenuation[2]) + attenuation[3];
				cgain = clamp(cgain, 0.0f, 1.0f);

				// directional attenuation
				// maximum at 0.75 (source on opposite side)
				relvec = relvec * (1.0f / len);
				(-listener_rot).RotateVector(relvec);
				float xcoord = relvec.dot(Direction::Right) * 0.75f;
				float pgain1 = xcoord;			// left attenuation
				float pgain2 = -xcoord;			// right attenuation
				if (pgain1 < 0) pgain1 = 0;
				if (pgain2 < 0) pgain2 = 0;

				gain1 = cgain * src.gain * (1 - pgain1);
				gain2 = cgain * src.gain * (1 - pgain2);
			}
			else
			{
				gain1 = gain2 = src.gain;
			}

			int maxgain = std::max(gain1, gain2) * Sampler::denom;
			if (maxgain > 0)
			{
				SourceActive sa;
				sa.gain = maxgain;
				sa.id = i;
				sources_active.push_back(sa);
			}
		}

		// fade sound volume
		float volume = set_pause ? 0 : sound_volume;

		supdate[i].gain1 = volume * gain1 * Sampler::denom;
		supdate[i].gain2 = volume * gain2 * Sampler::denom;
		supdate[i].pitch = src.pitch * Sampler::denom;
	}

	LimitActiveSources();
}

void Sound::LimitActiveSources()
{
	// limit active sources to max active sources
	if (sources_active.size() <= max_active_sources)
		return;

	// get loudest max_active_sources
	std::partial_sort(
		sources_active.begin(),
		sources_active.begin() + max_active_sources,
		sources_active.end());

	// mute remaining sources
	std::vector<SamplerSet> & supdate = samplers_update.getFirst().sset;
	for (size_t i = max_active_sources; i < sources_active.size(); ++i)
	{
		supdate[sources_active[i].id].gain1 = 0;
		supdate[sources_active[i].id].gain2 = 0;
	}
}

void Sound::SetSamplerChanges()
{
	if (samplers_update.getFirst().empty())
		return;

	SDL_LockMutex(sampler_lock);
	if (samplers_update.getSecond().empty())
	{
		samplers_update.getFirst().id = update_id++;
		samplers_update.swapFirst();
	}
	sources_pause = set_pause;
	SDL_UnlockMutex(sampler_lock);
}

void Sound::GetSamplerChanges()
{
	SDL_LockMutex(sampler_lock);
	if (!samplers_update.getSecond().empty())
	{
		samplers_update.swapLast();
	}
	samplers_fade = (samplers_pause != sources_pause);
	samplers_pause = sources_pause;
	SDL_UnlockMutex(sampler_lock);
}

void Sound::ProcessSamplerUpdate()
{
	std::vector<SamplerSet> & supdate = samplers_update.getLast().sset;
	if (supdate.empty())
		return;

	assert(samplers_num == supdate.size());
	for (size_t i = 0; i < samplers_num; ++i)
	{
		samplers[i].gain1 = supdate[i].gain1;
		samplers[i].gain2 = supdate[i].gain2;
		samplers[i].pitch = supdate[i].pitch;
	}
	supdate.clear();
}

void Sound::ProcessSamplers(unsigned char *stream, int len)
{
	// clear stream
	memset(stream, 0, len);

	// pause sampling
	if (samplers_pause && !samplers_fade)
		return;

	// init sampling buffers
	int len4 = len / 4;
	buffer1.resize(len4);
	buffer2.resize(len4);

	// run samplers
	short * sstream = (short*)stream;
	for (size_t i = 0; i < samplers_num; ++i)
	{
		Sampler & smp = samplers[i];

		if (!smp.playing)
			continue;

		if (smp.gain1 | smp.gain2 | smp.last_gain1 | smp.last_gain2)
		{
			SampleAndAdvanceWithPitch16bit(smp, &buffer1[0], &buffer2[0], len4);

			for (int n = 0; n < len4; ++n)
			{
				int pos = n * 2;
				int val1 = sstream[pos] + buffer1[n];
				int val2 = sstream[pos + 1] + buffer2[n];

				val1 = clamp(val1, -32768, 32767);
				val2 = clamp(val2, -32768, 32767);

				sstream[pos] = val1;
				sstream[pos + 1] = val2;
			}
		}
		else
		{
			AdvanceWithPitch(smp, len);
		}

		if (!smp.playing)
			sources_stop.getLast().push_back(smp.id);
	}
}

void Sound::ProcessSamplerRemove()
{
	std::vector<size_t> & sremove = samplers_update.getLast().sremove;
	for (size_t i = 0; i < sremove.size(); ++i)
	{
		size_t id = sremove[i];
		assert(id < samplers.size());
		RemoveItem(id, samplers, samplers_num);
	}
	sremove.clear();
}

void Sound::ProcessSamplerAdd()
{
	std::vector<SamplerAdd> & sadd = samplers_update.getLast().sadd;
	for (size_t i = 0; i < sadd.size(); ++i)
	{
		Sampler smp;
		smp.buffer = sadd[i].buffer;
		smp.samples_per_channel = smp.buffer->GetInfo().samples / smp.buffer->GetInfo().channels;
		smp.sample_pos = sadd[i].offset;
		smp.sample_pos_remainder = 0;
		smp.pitch = smp.denom;
		smp.gain1 = 0;
		smp.gain2 = 0;
		smp.last_gain1 = 0;
		smp.last_gain2 = 0;
		smp.playing = true;
		smp.loop = sadd[i].loop;

		if (sadd[i].id == -1)
		{
			AddItem(smp, samplers, samplers_num);
		}
		else
		{
			smp.id = samplers[sadd[i].id].id;
			samplers[sadd[i].id] = smp;
		}
	}
	sadd.clear();
}

void Sound::SetSourceChanges()
{
	if (sources_stop.getLast().empty())
		return;

	SDL_LockMutex(source_lock);
	sources_stop.swapLast();
	SDL_UnlockMutex(source_lock);
}

void Sound::Callback16bitStereo(void *myself, Uint8 *stream, int len)
{
	assert(this == myself);
	assert(initdone);

	GetSamplerChanges();
/*
	logsa << "id: " << samplers_update.getLast().id;
	logsa << " add: " << samplers_update.getLast().sadd.size();
	logsa << " del: " << samplers_update.getLast().sremove.size();
	logsa << " set: " << samplers_update.getLast().sset.size();
	logsa << " samplers: " << samplers_num;
	logsa << std::endl;
*/
	ProcessSamplerAdd();

	ProcessSamplerUpdate();

	ProcessSamplers(stream, len);

	ProcessSamplerRemove();

	SetSourceChanges();
}

void Sound::CallbackWrapper(void *sound, unsigned char *stream, int len)
{
	static_cast<Sound*>(sound)->Callback16bitStereo(sound, stream, len);
}

void Sound::SampleAndAdvanceWithPitch16bit(
	Sampler & sampler, int * chan1, int * chan2, int len)
{
	assert(len > 0);
	assert(sampler.buffer);

	// if not playing, fill output buffers with silence
	if (!sampler.playing)
	{
		// should be dealt with before getting here
		assert(0);
		for (int i = 0; i < len; ++i)
		{
			chan1[i] = chan2[i] = 0;
		}
		return;
	}

	// start samlping
	int chan = sampler.buffer->GetInfo().channels;
	int samples = sampler.samples_per_channel * chan;
	int chaninc = chan - 1;
	int nr = sampler.sample_pos_remainder;
	int ni = sampler.sample_pos;
	const int16_t * buf = (const int16_t *)sampler.buffer->GetRawBuffer();

	for (int i = 0; i < len; ++i)
	{
		// limit gain change rate
		int gain_delta1 = sampler.gain1 - sampler.last_gain1;
		int gain_delta2 = sampler.gain2 - sampler.last_gain2;
		gain_delta1 = clamp(gain_delta1, -sampler.max_gain_delta, sampler.max_gain_delta);
		gain_delta2 = clamp(gain_delta2, -sampler.max_gain_delta, sampler.max_gain_delta);
		sampler.last_gain1 += gain_delta1;
		sampler.last_gain2 += gain_delta2;

		if (ni >= sampler.samples_per_channel && !sampler.loop)
		{
			// finish playing the buffer if looping is not enabled
			chan1[i] = chan2[i] = 0;
			sampler.playing = false;
		}
		else
		{
			// the sample to the left of the playback position, channel 0 and 1
			int id1 = (ni * chan) % samples;
			int samp10 = buf[id1];
			int samp11 = buf[id1 + chaninc];

			// the sample to the right of the playback position, channel 0 and 1
			int id2 = (id1 + chan) % samples;
			int samp20 = buf[id2];
			int samp21 = buf[id2 + chaninc];

			// interpolated sample at playback position
			int val1 = (nr * samp20 + (sampler.denom - nr) * samp10) / sampler.denom;
			int val2 = (nr * samp21 + (sampler.denom - nr) * samp11) / sampler.denom;
			val1 = (val1 * sampler.last_gain1) / sampler.denom;
			val2 = (val2 * sampler.last_gain2) / sampler.denom;

			// fill output buffers
			chan1[i] = val1;
			chan2[i] = val2;

			// advance playback position
			nr += sampler.pitch;
			int ninc = nr / sampler.denom;
			nr -= ninc * sampler.denom;
			ni += ninc;
		}
	}

	sampler.sample_pos = ni;
	sampler.sample_pos_remainder = nr;
	if (!sampler.loop)
	{
		sampler.playing = (sampler.sample_pos < sampler.samples_per_channel);
	}
	else
	{
		sampler.sample_pos = sampler.sample_pos % sampler.samples_per_channel;
	}
}

void Sound::AdvanceWithPitch(Sampler & sampler, int len)
{
	// advance playback position
	sampler.sample_pos_remainder += len * sampler.pitch;
	int delta = sampler.sample_pos_remainder / sampler.denom;
	sampler.sample_pos_remainder -= delta * sampler.denom;
	sampler.sample_pos += delta;

	// loop buffer
	if (!sampler.loop)
	{
		sampler.playing = (sampler.sample_pos < sampler.samples_per_channel);
	}
	else
	{
		sampler.sample_pos = sampler.sample_pos % sampler.samples_per_channel;
	}
}
