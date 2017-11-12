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
#include "minmax.h"
#include "coordinatesystem.h"
#include <SDL2/SDL_audio.h>
#include <algorithm>
#include <cassert>

//static std::ofstream logso("logso.txt");
//static std::ofstream logsa("logsa.txt");

#define FRACTIONBITS (15)
#define FRACTIONONE  (1<<FRACTIONBITS)
#define FRACTIONMASK (FRACTIONONE-1)
#define MAXGAINDELTA (FRACTIONONE * 173 / 44100) // 256 samples from min to max gain

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
	deviceinfo(0, 0, 0, 0),
	sound_volume(0),
	initdone(false),
	disable(false),
	max_active_sources(64),
	sources_num(0),
	update_id(0),
	sources_pause(true),
	samplers_num(0),
	samplers_pause(true),
	samplers_fade(false)
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
}

bool Sound::Init(unsigned short buffersize, std::ostream & info_output, std::ostream & error_output)
{
	if (disable || initdone)
		return false;

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

	unsigned frequency = obtained.freq;
	unsigned channels = obtained.channels;
	unsigned samples = obtained.samples;
	unsigned bytespersample = 1;
	if (obtained.format == AUDIO_S16SYS)
	{
		bytespersample = 2;
	}
	else if (obtained.format == AUDIO_F32SYS)
	{
		bytespersample = 4;
	}

	std::ostringstream dout;
	dout << "Obtained audio device:" << std::endl;
	dout << "Frequency: " << frequency << std::endl;
	dout << "Format: " << obtained.format << std::endl;
	dout << "Bits per sample: " << bytespersample * 8 << std::endl;
	dout << "Channels: " << channels << std::endl;
	dout << "Silence: " << (unsigned)obtained.silence << std::endl;
	dout << "Samples: " << samples << std::endl;
	dout << "Size: " << obtained.size;
	info_output << dout.str() << std::endl;

	if (((obtained.format != AUDIO_S16SYS) && (obtained.format != AUDIO_F32SYS)) ||
		(obtained.channels != desired.channels))
	{
		error_output << "Audio device has unsupported format or channel count. Disabling sound." << std::endl;
		Disable();
		return false;
	}

	deviceinfo = SoundInfo(samples, frequency, channels, bytespersample);
	buffer[0].reserve(samples);
	buffer[1].reserve(samples);
	initdone = true;
	SetVolume(1);

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
	src.pitch = 1;
	src.gain = 0;
	src.is3d = is3d;
	src.playing = true;
	src.loop = loop;
	size_t id = AddItem(src, sources, sources_num);

	// notify sound thread
	SamplerAdd ns;
	ns.buffer = buffer.get();
	ns.offset = offset * FRACTIONONE;
	ns.loop = loop;
	ns.id = -1;
	samplers_update.back().sadd.push_back(ns);

	return id;
}

void Sound::RemoveSource(size_t id)
{
	samplers_update.back().sremove.push_back(id);
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
	ns.offset = src.offset * FRACTIONONE;
	ns.loop = src.loop;
	ns.id = idn;
	samplers_update.back().sadd.push_back(ns);
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

	sources_pause = pause;

	// process source stop messages
	ProcessSourceStop();

	// ProcessSourceAdd is implicit

	// calculate sampler changes from sources
	ProcessSources();

	// commit sampler changes to sound thread
	SetSamplerChanges();
}

void Sound::ProcessSourceStop()
{
	if (!sources_stop.swap_front())
		return;

	auto & sstop = sources_stop.front();
	for (auto id : sstop)
	{
		auto idn = sources[id].id;
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
	for (auto id : sources_remove)
	{
		RemoveItem(id, sources, sources_num);
	}
	sources_remove.clear();
}

void Sound::ProcessSources()
{
	auto & sset = samplers_update.back().sset;
	sset.resize(sources_num);

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
				cgain = Clamp(cgain, 0.0f, 1.0f);

				// directional attenuation
				// maximum at 0.75 (source on opposite side)
				relvec = relvec * (1.0f / len);
				(-listener_rot).RotateVector(relvec);
				float xcoord = relvec.dot(Direction::Right) * 0.75f;
				float pgain1 = Max(xcoord, 0.0f);  // left attenuation
				float pgain2 = Max(-xcoord, 0.0f); // right attenuation

				gain1 = cgain * src.gain * (1 - pgain1);
				gain2 = cgain * src.gain * (1 - pgain2);
			}
			else
			{
				gain1 = gain2 = src.gain;
			}

			unsigned maxgain = Max(gain1, gain2) * FRACTIONONE;
			if (maxgain > 0)
			{
				SourceActive sa;
				sa.gain = maxgain;
				sa.id = i;
				sources_active.push_back(sa);
			}
		}

		// fade sound volume
		float volume = sources_pause ? 0 : sound_volume;

		sset[i].gain1 = volume * gain1 * FRACTIONONE;
		sset[i].gain2 = volume * gain2 * FRACTIONONE;

		auto info = src.buffer->GetInfo();
		auto base_pitch = FRACTIONONE * info.frequency / deviceinfo.frequency;
		sset[i].pitch = src.pitch * base_pitch;
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
	auto & sset = samplers_update.back().sset;
	for (size_t i = max_active_sources; i < sources_active.size(); ++i)
	{
		sset[sources_active[i].id].gain1 = 0;
		sset[sources_active[i].id].gain2 = 0;
	}
}

void Sound::SetSamplerChanges()
{
	auto & su = samplers_update.back();
	if (su.empty())
		return;

	su.pause = sources_pause;
	su.id = update_id;

	if (samplers_update.swap_back())
	{
		ProcessSourceRemove();
		update_id++;
	}
}

void Sound::GetSamplerChanges()
{
	if (samplers_update.swap_front())
	{
		auto & su = samplers_update.front();
		samplers_fade = (samplers_pause != su.pause);
		samplers_pause = su.pause;
	}
}

void Sound::ProcessSamplerUpdate()
{
	auto & sset = samplers_update.front().sset;
	if (sset.empty())
		return;

	assert(samplers_num == sset.size());
	for (size_t i = 0; i < samplers_num; ++i)
	{
		samplers[i].gain1 = sset[i].gain1;
		samplers[i].gain2 = sset[i].gain2;
		samplers[i].pitch = sset[i].pitch;
	}
	sset.clear();
}

template <typename stream_type, typename buffer_type, int vmin, int vmax>
void Sound::ProcessSamplers(unsigned char stream[], unsigned len)
{
	// clear stream
	memset(stream, 0, len);

	// pause sampling
	if (samplers_pause && !samplers_fade)
		return;

	auto & sstop = sources_stop.back();

	// init sampling buffers
	auto samples = len / (2 * sizeof(stream_type));
	buffer[0].resize(samples);
	buffer[1].resize(samples);

	// run samplers
	auto sstream = (stream_type*)stream;
	auto buffer0 = (buffer_type*)&buffer[0][0];
	auto buffer1 = (buffer_type*)&buffer[1][0];
	for (size_t i = 0; i < samplers_num; ++i)
	{
		Sampler & smp = samplers[i];
		if (!smp.playing)
			continue;

		if (smp.gain1 | smp.gain2 | smp.last_gain1 | smp.last_gain2)
		{
			SampleAndAdvanceWithPitch<stream_type>(smp, buffer0, buffer1, samples);

			for (unsigned n = 0; n < samples; ++n)
			{
				unsigned pos = n * 2;
				buffer_type val1 = sstream[pos] + buffer0[n];
				buffer_type val2 = sstream[pos + 1] + buffer1[n];

				val1 = Clamp<buffer_type>(val1, vmin, vmax);
				val2 = Clamp<buffer_type>(val2, vmin, vmax);

				sstream[pos] = val1;
				sstream[pos + 1] = val2;
			}
		}
		else
		{
			AdvanceWithPitch(smp, samples);
		}

		if (!smp.playing)
			sstop.push_back(smp.id);
	}
}

void Sound::ProcessSamplerRemove()
{
	auto & sremove = samplers_update.front().sremove;
	for (auto id : sremove)
	{
		assert(id < samplers.size());
		RemoveItem(id, samplers, samplers_num);
	}
	sremove.clear();
}

void Sound::ProcessSamplerAdd()
{
	auto & sadd = samplers_update.front().sadd;
	for (const auto & sa : sadd)
	{
		auto info = sa.buffer->GetInfo();
		auto base_pitch = FRACTIONONE * info.frequency / deviceinfo.frequency;
		auto samples_per_channel = info.samples / info.channels;

		Sampler smp;
		smp.buffer = sa.buffer;
		smp.samples_per_channel = samples_per_channel;
		smp.sample_pos = sa.offset;
		smp.sample_pos_remainder = 0;
		smp.pitch = base_pitch;
		smp.gain1 = 0;
		smp.gain2 = 0;
		smp.last_gain1 = 0;
		smp.last_gain2 = 0;
		smp.playing = true;
		smp.loop = sa.loop;

		if (sa.id == -1)
		{
			AddItem(smp, samplers, samplers_num);
		}
		else
		{
			smp.id = samplers[sa.id].id;
			samplers[sa.id] = smp;
		}
	}
	sadd.clear();
}

void Sound::SetSourceChanges()
{
	if (sources_stop.back().empty())
		return;

	sources_stop.swap_back();
}

template <typename stream_type, typename buffer_type, int vmin, int vmax>
void Sound::CallbackStereo(void * myself, unsigned char stream[], int len)
{
	assert(this == myself);
	assert(initdone);
	assert(len > 0);

	GetSamplerChanges();

	ProcessSamplerAdd();

	ProcessSamplerUpdate();

	ProcessSamplers<stream_type, buffer_type, vmin, vmax>(stream, len);

	ProcessSamplerRemove();

	SetSourceChanges();
}

void Sound::CallbackWrapper(void * sound, unsigned char stream[], int len)
{
	auto bytespersample = static_cast<Sound*>(sound)->deviceinfo.bytespersample;
	if (bytespersample == 2)
	{
		static_cast<Sound*>(sound)->CallbackStereo<short, int, -32768, 32767>(sound, stream, len);
	}
	else if (bytespersample == 4)
	{
		static_cast<Sound*>(sound)->CallbackStereo<float, float, -1, 1>(sound, stream, len);
	}
}

template <typename T0, typename T1> T0 Cast(T1 v);
template <> inline float Cast<float, unsigned>(unsigned v) { return v * (1.0f / FRACTIONONE); }
template <> inline float Cast<float, int>(int v) { return v * (1.0f / FRACTIONONE); }
template <> inline unsigned Cast<unsigned, float>(float v) { return v * FRACTIONONE; }
template <> inline int Cast<int, float>(float v) { return v * FRACTIONONE; }
template <> inline unsigned Cast<unsigned, int>(int v) { return v; }
template <> inline int Cast<int, unsigned>(unsigned v) { return v; }
template <> inline int Cast<int, int>(int v) { return v; }

template <typename T> T Scale(T v, T s);
template <> inline int Scale<int>(int v, int s) { return v * s / FRACTIONONE; }
template <> inline float Scale<float>(float v, float s) { return v * s; }

template <typename sample_type, typename buffer_type>
void Sound::SampleAndAdvanceWithPitch(Sampler & sampler, buffer_type chan1[], buffer_type chan2[], unsigned len)
{
	assert(sampler.buffer);
	assert(sampler.playing);

	// start samlping
	auto channels = sampler.buffer->GetInfo().channels;
	auto chaninc = channels - 1;
	auto samples = sampler.samples_per_channel * channels;
	auto nr = sampler.sample_pos_remainder;
	auto ni = sampler.sample_pos;

	auto buf = (const sample_type *)sampler.buffer->GetRawBuffer();
	auto gain1 = Cast<buffer_type>(sampler.gain1);
	auto gain2 = Cast<buffer_type>(sampler.gain2);
	auto last_gain1 = Cast<buffer_type>(sampler.last_gain1);
	auto last_gain2 = Cast<buffer_type>(sampler.last_gain2);
	auto max_gain_delta = Cast<buffer_type>(MAXGAINDELTA);

	for (unsigned i = 0; i < len; ++i)
	{
		// limit gain change rate
		auto gain_delta1 = gain1 - last_gain1;
		auto gain_delta2 = gain2 - last_gain2;
		gain_delta1 = Clamp(gain_delta1, -max_gain_delta, max_gain_delta);
		gain_delta2 = Clamp(gain_delta2, -max_gain_delta, max_gain_delta);
		last_gain1 += gain_delta1;
		last_gain2 += gain_delta2;

		if (ni >= sampler.samples_per_channel && !sampler.loop)
		{
			// finish playing the buffer if looping is not enabled
			chan1[i] = chan2[i] = 0;
			sampler.playing = false;
		}
		else
		{
			// the sample to the left of the playback position, channel 0 and 1
			auto id1 = (ni * channels) % samples;
			buffer_type samp10 = buf[id1];
			buffer_type samp11 = buf[id1 + chaninc];

			// the sample to the right of the playback position, channel 0 and 1
			auto id2 = (id1 + channels) % samples;
			buffer_type samp20 = buf[id2];
			buffer_type samp21 = buf[id2 + chaninc];

			// interpolated sample at playback position
			auto f = Cast<buffer_type>(nr);
			auto val1 = samp10 + Scale(samp20 - samp10, f);
			auto val2 = samp11 + Scale(samp21 - samp11, f);

			// fill output buffers
			chan1[i] = Scale(val1, last_gain1);
			chan2[i] = Scale(val2, last_gain2);

			// advance playback position
			nr += sampler.pitch;
			ni += nr >> FRACTIONBITS;
			nr &= FRACTIONMASK;
		}
	}

	sampler.last_gain1 = Cast<unsigned>(last_gain1);
	sampler.last_gain2 = Cast<unsigned>(last_gain2);
	sampler.sample_pos = ni;
	sampler.sample_pos_remainder = nr;

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

void Sound::AdvanceWithPitch(Sampler & sampler, unsigned len)
{
	// advance playback position
	auto nr = sampler.sample_pos_remainder;
	auto ni = sampler.sample_pos;
	nr += sampler.pitch * len;
	ni += nr >> FRACTIONBITS;
	nr &= FRACTIONMASK;
	sampler.sample_pos = ni;
	sampler.sample_pos_remainder = nr;

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
