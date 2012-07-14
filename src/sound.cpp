#include "sound.h"

#include "coordinatesystem.h"
#include <SDL/SDL.h>
#include <algorithm>
#include <cassert>

template <class T>
static inline T clamp(T val, T min, T max)
{
	return val > min ? (val < max ? val : max) : min;
}

static inline void Lock(SDL_mutex * mutex)
{
	if (SDL_mutexP(mutex) == -1)
		assert(0 && "Couldn't lock mutex");
}

static inline void Unlock(SDL_mutex * mutex)
{
	if (SDL_mutexV(mutex) == -1)
		assert(0 && "Couldn't unlock mutex");
}

bool SOUND::SourceActive::operator<(const SOUND::SourceActive & other)
{
	// reverse op as partial sort sorts for the smallest elemets
	return this->gain > other.gain;
}

SOUND::SOUND() :
	deviceinfo(0, 0, 0, 0),
	initdone(false),
	disable(false),
	paused(true),
	sampler_lock(0),
	source_lock(0),
	max_active_sources(64),
	sources_num(0),
	samplers_num(0)
{
	volume_filter.SetFilterOrder0(1.0);
	sources.reserve(64);
	samplers.reserve(64);
}

SOUND::~SOUND()
{
	if (initdone)
		SDL_CloseAudio();

	if (sampler_lock)
		SDL_DestroyMutex(sampler_lock);

	if (source_lock)
		SDL_DestroyMutex(source_lock);
}

bool SOUND::Init(int buffersize, std::ostream & info_output, std::ostream & error_output)
{
	if (disable || initdone)
		return false;

	sampler_lock = SDL_CreateMutex();
	source_lock = SDL_CreateMutex();

	SDL_AudioSpec desired, obtained;

	desired.freq = 44100;
	desired.format = AUDIO_S16SYS;
	desired.samples = buffersize;
	desired.callback = SOUND::CallbackWrapper;
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

	std::stringstream dout;
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

	deviceinfo = SOUNDINFO(samples, frequency, channels, bytespersample);

	initdone = true;

	SetVolume(1.0);

	return true;
}

const SOUNDINFO & SOUND::GetDeviceInfo() const
{
	return deviceinfo;
}

bool SOUND::Enabled() const
{
	return !disable;
}

void SOUND::Disable()
{
	disable = true;
}

void SOUND::Pause(bool value)
{
	if (paused != value)
	{
		SDL_PauseAudio(value);
		paused = value;
	}
}

void SOUND::Update()
{
	if (disable) return;

	GetSourceChanges();

	ProcessSourceStop();

	ProcessSourceRemove();

	ProcessSources();

	SetSamplerChanges();

	// short circuit if paused(sound thread blocked)
	if (paused)
	{
		GetSamplerChanges();

		ProcessSamplerAdd();

		ProcessSamplerRemove();

		SetSourceChanges();

		GetSourceChanges();

		ProcessSourceStop();

		ProcessSourceRemove();
	}
}

void SOUND::SetMaxActiveSources(size_t value)
{
	max_active_sources = value;
}

// add item to a compactifying vector
template <class T>
inline size_t AddItem(T & item, std::vector<T> & items, size_t & item_num)
{
	size_t id = item_num;
	if (id < items.size())
	{
		size_t idn = items[id].id;
		if (idn != id)
		{
			// swap back redirected item
			assert(idn < id);
			items[id] = items[idn];
			id = idn;
		}
		items[id] = item;
	}
	else
	{
		items.push_back(item);
	}
	items[id].id = id;
	++item_num;

	return id;
}

// remove item from a compactifying vector
template <class T>
inline void RemoveItem(size_t id, std::vector<T> & items, size_t & item_num)
{
	assert(id < items.size());
	size_t idn = items[id].id;
	assert(idn < item_num);
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

size_t SOUND::AddSource(std::tr1::shared_ptr<SOUNDBUFFER> buffer, float offset, bool is3d, bool loop)
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
	sampler_add.getFirst().push_back(ns);

	//std::cout << "Add sound source: " << id << " " << buffer->GetName() << std::endl;
	return id;
}

void SOUND::RemoveSource(size_t id)
{
	// notify sound thread, it will notify main thread to remove the source
	//std::cout << "To be removed source: " << id << " " << sources[sources[id].id].buffer->GetName() << std::endl;
	sampler_remove.getFirst().push_back(id);
}

void SOUND::ResetSource(size_t id)
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
	sampler_add.getFirst().push_back(ns);
}

bool SOUND::GetSourcePlaying(size_t id) const
{
	return sources[sources[id].id].playing;
}

void SOUND::SetSourceVelocity(size_t id, float x, float y, float z)
{
	sources[sources[id].id].velocity.Set(x, y, z);
}

void SOUND::SetSourcePosition(size_t id, float x, float y, float z)
{
	sources[sources[id].id].position.Set(x, y, z);
}

void SOUND::SetSourceRotation(size_t id, float x, float y, float z, float w)
{
	//sources[sources[id].id].rotation.Set(x, y, z, w);
}

void SOUND::SetSourcePitch(size_t id, float value)
{
	sources[sources[id].id].pitch = value;
}

void SOUND::SetSourceGain(size_t id, float value)
{
	sources[sources[id].id].gain = value;
}

void SOUND::SetListenerVelocity(float x, float y, float z)
{
	listener_vel.Set(x, y, z);
}

void SOUND::SetListenerPosition(float x, float y, float z)
{
	listener_pos.Set(x, y, z);
}

void SOUND::SetListenerRotation(float x, float y, float z, float w)
{
	listener_rot.Set(x, y, z, w);
}

void SOUND::SetVolume(float value)
{
	volume_filter.SetFilterOrder0(clamp(value, 0.f, 1.f));
}

void SOUND::GetSourceChanges()
{
	Lock(source_lock);
	source_stop.swapFirst();
	source_remove.swapFirst();
	Unlock(source_lock);
}

void SOUND::ProcessSourceStop()
{
	std::vector<size_t> & sstop = source_stop.getFirst();
	for (size_t i = 0; i < sstop.size(); ++i)
	{
		size_t id = sstop[i];
		assert(id < sources_num);
		sources[id].playing = false;
	}
	sstop.clear();
}

void SOUND::ProcessSourceRemove()
{
	std::vector<size_t> & sremove = source_remove.getFirst();
	for (size_t i = 0; i < sremove.size(); ++i)
	{
		size_t id = sremove[i];
		assert(id < sources.size());
		size_t idn = sources[id].id;
		assert(idn < sources_num);
		//std::cout << "Remove sound source: " << id << " " << sources[idn].buffer->GetName() << std::endl;
		RemoveItem(id, sources, sources_num);
	}
	sremove.clear();
}

void SOUND::ProcessSources()
{
	std::vector<SamplerUpdate> & supdate = sampler_update.getFirst();
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
				MATHVECTOR <float, 3> relvec = src.position - listener_pos;
				float len = relvec.Magnitude();
				if (len < 0.1f) len = 0.1f;

				// distance attenuation
				float cgain = 0.25f / log(100.f) * (log(1000.f) - 1.5f * log(len));
				cgain = clamp(cgain, 0.0f, 1.0f);

				// directional attenuation
				// maximum at 0.75 (source on opposite side)
				relvec = relvec * (1.0f / len);
				(-listener_rot).RotateVector(relvec);
				float xcoord = relvec.dot(direction::Right) * 0.75f;
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

		supdate[i].gain1 = gain1 * Sampler::denom;
		supdate[i].gain2 = gain2 * Sampler::denom;
		supdate[i].pitch = src.pitch * Sampler::denom;
	}

	LimitActiveSources();
}

void SOUND::LimitActiveSources()
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
	std::vector<SamplerUpdate> & supdate = sampler_update.getFirst();
	for (size_t i = max_active_sources; i < sources_active.size(); ++i)
	{
		supdate[sources_active[i].id].gain1 = 0;
		supdate[sources_active[i].id].gain2 = 0;
	}
}

void SOUND::SetSamplerChanges()
{
	Lock(sampler_lock);
	if (sampler_update.getFirst().size()) sampler_update.swapFirst();
	if (sampler_add.getFirst().size()) sampler_add.swapFirst();
	if (sampler_remove.getFirst().size()) sampler_remove.swapFirst();
	Unlock(sampler_lock);
}

void SOUND::GetSamplerChanges()
{
	Lock(sampler_lock);
	sampler_update.swapLast();
	sampler_remove.swapLast();
	sampler_add.swapLast();
	Unlock(sampler_lock);
}

void SOUND::ProcessSamplerUpdate()
{
	std::vector<SamplerUpdate> & supdate = sampler_update.getLast();
	if (samplers_num == supdate.size())
	{
		for (size_t i = 0; i < samplers_num; ++i)
		{
			samplers[i].gain1 = supdate[i].gain1;
			samplers[i].gain2 = supdate[i].gain2;
			samplers[i].pitch = supdate[i].pitch;
		}
	}
	supdate.clear();
}

void SOUND::ProcessSamplers(unsigned char *stream, int len)
{
	// set buffers and clear stream
	int len4 = len / 4;
	buffer1.resize(len4);
	buffer2.resize(len4);
	memset(stream, 0, len);

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

			volume_filter.Filter(&buffer1[0], &buffer2[0], len4);

			for (int n = 0; n < len4; ++n)
			{
				int pos = n * 2;
				sstream[pos] = clamp(sstream[pos] + buffer1[n], -32768, 32767);
				sstream[pos + 1] = clamp(sstream[pos + 1] + buffer2[n], -32768, 32767);
			}
		}
		else
		{
			AdvanceWithPitch(smp, len);
		}

		if (!smp.playing)
			source_stop.getLast().push_back(i);
	}
}

void SOUND::ProcessSamplerRemove()
{
	std::vector<size_t> & sremove = sampler_remove.getLast();
	if (!sremove.empty())
	{
		for (size_t i = 0; i < sremove.size(); ++i)
		{
			size_t id = sremove[i];
			assert(id < samplers.size());
			RemoveItem(id, samplers, samplers_num);
		}
		source_remove.getLast() = sremove;
		sremove.clear();
	}
}

void SOUND::ProcessSamplerAdd()
{
	std::vector<SamplerAdd> & sadd = sampler_add.getLast();
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

void SOUND::SetSourceChanges()
{
	Lock(source_lock);
	if (source_stop.getLast().size()) source_stop.swapLast();
	if (source_remove.getLast().size()) source_remove.swapLast();
	Unlock(source_lock);
}

void SOUND::Callback16bitStereo(void *myself, Uint8 *stream, int len)
{
	assert(this == myself);
	assert(initdone);

	GetSamplerChanges();

	ProcessSamplerUpdate();

	ProcessSamplers(stream, len);

	ProcessSamplerAdd();

	ProcessSamplerRemove();

	SetSourceChanges();
}

void SOUND::CallbackWrapper(void *sound, unsigned char *stream, int len)
{
	static_cast<SOUND*>(sound)->Callback16bitStereo(sound, stream, len);
}

void SOUND::SampleAndAdvanceWithPitch16bit(
	Sampler & sampler, int * chan1, int * chan2, int len)
{
	assert(len > 0);
	assert(sampler.buffer);

	// if not playing, fill output buffers with silence, should not happen
	if (!sampler.playing)
	{
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

void SOUND::AdvanceWithPitch(Sampler & sampler, int len)
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
