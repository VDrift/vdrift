#include "sound.h"

#include <SDL/SDL.h>
#include <algorithm>
#include <cassert>

static inline bool CompareSource(SOUNDSOURCE * i, SOUNDSOURCE * j)
{
	return !(*i < *j);
}

SOUND::SOUND() :
	initdone(false),
	paused(true),
	disable(false),
	deviceinfo(0, 0, 0, 0),
	max_active_sources(64),
	sourcelistlock(0)
{
	volume_filter.SetFilterOrder0(1.0);
	sources_active_p = &sources_active_1;
	sources_inactive_p = &sources_inactive_1;
}

SOUND::~SOUND()
{
	if (initdone)
		SDL_CloseAudio();

	if (sourcelistlock)
		SDL_DestroyMutex(sourcelistlock);
}

bool SOUND::Init(int buffersize, std::ostream & info_output, std::ostream & error_output)
{
	if (disable || initdone)
		return false;

	sourcelistlock = SDL_CreateMutex();

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

	SetMasterVolume(1.0);

	return true;
}

void SOUND::Pause(bool value)
{
	if (paused != value)
	{
		paused = value;
		SDL_PauseAudio(paused);
	}
}

void SOUND::AddSources(std::list<SOUNDSOURCE *> & sources)
{
	if (disable) return;
	sourcelist.splice(sourcelist.end(), sources);
}

void SOUND::AddSource(SOUNDSOURCE & source)
{
	if (disable) return;
	sourcelist.push_back(&source);
}

void SOUND::RemoveSource(SOUNDSOURCE * todel)
{
	if (disable) return;

	assert(todel);
	for (std::list <SOUNDSOURCE *>::iterator i = sourcelist.begin(); i != sourcelist.end(); ++i)
	{
		if (*i == todel)
		{
			sourcelist.erase(i);
			return;
		}
	}
}

void SOUND::Clear()
{
	sourcelist.clear();
}

void  SOUND::Update()
{
	// get source lists to work with
	std::vector <SOUNDSOURCE *> & sources_active = (sources_active_p != &sources_active_1) ? sources_active_1 : sources_active_2;
	std::vector <SOUNDSOURCE *> & sources_inactive = (sources_inactive_p != &sources_inactive_1) ? sources_inactive_1 : sources_inactive_2;

	CollectGarbage();
	
	DetermineActiveSources(sources_active, sources_inactive);
	
	Compute3DEffects(sources_active);
	
	LimitActiveSources(sources_active, sources_inactive);

	// commit processed source lists
	LockSourceList();
	sources_active_p = &sources_active;
	sources_inactive_p = &sources_inactive;
	UnlockSourceList();
}

void SOUND::DetermineActiveSources(std::vector <SOUNDSOURCE *> & sources_active, std::vector <SOUNDSOURCE *> & sources_inactive)
{
	sources_active.clear();
	sources_inactive.clear();
	for (std::list <SOUNDSOURCE *>::const_iterator i = sourcelist.begin(); i != sourcelist.end(); ++i)
	{
		if ((*i)->Audible())
		{
			sources_active.push_back(*i);
		}
		else
		{
			sources_inactive.push_back(*i);
		}
	}
}

void SOUND::Compute3DEffects(std::vector <SOUNDSOURCE *> & sources_active) const
{
	for (std::vector <SOUNDSOURCE *>::const_iterator i = sources_active.begin(); i != sources_active.end(); ++i)
	{
		if ((*i)->Get3DEffects())
		{
			MATHVECTOR <float, 3> relvec = (*i)->GetPosition() - listener_pos;
			float len = relvec.Magnitude();
			if (len < 0.1)
			{
				relvec[2] = 0.1;
				len = relvec.Magnitude();
			}
			listener_rot.RotateVector(relvec);

			float cgain = log(1000.0 / pow((double)len, 1.3)) / log(100.0);
			if (cgain > 1.0) cgain = 1.0;
			if (cgain < 0.0) cgain = 0.0;

			float xcoord = -relvec.Normalize()[1];
			float pgain1 = -xcoord;
			float pgain2 = xcoord;
			if (pgain1 < 0) pgain1 = 0;
			if (pgain2 < 0) pgain2 = 0;

			float gain1 = cgain * (*i)->GetGain() * (1.0 - pgain1);
			float gain2 = cgain * (*i)->GetGain() * (1.0 - pgain2);
			(*i)->SetComputedGain(gain1, gain2);
		}
		else
		{
			(*i)->SetComputedGain((*i)->GetGain(), (*i)->GetGain());
		}
	}
}

void SOUND::LimitActiveSources(std::vector <SOUNDSOURCE *> & sources_active, std::vector <SOUNDSOURCE *> & sources_inactive)
{
	if (sources_active.size() <= max_active_sources)
		return;

	std::partial_sort(
		sources_active.begin(),
		sources_active.begin() + max_active_sources,
		sources_active.end(),
		CompareSource);
	
	sources_inactive.insert(
		sources_inactive.end(),
		sources_active.begin() + max_active_sources,
		sources_active.end());

	sources_active.resize(max_active_sources);
}

void SOUND::CollectGarbage()
{
	if (disable) return;

	for (std::list <SOUNDSOURCE *>::iterator i = sourcelist.begin(); i != sourcelist.end(); ++i)
	{
		if (!(*i)->Audible() && (*i)->GetAutoDelete())
		{
			i = sourcelist.erase(i);
		}
	}
}

void SOUND::LockSourceList()
{
	if (SDL_mutexP(sourcelistlock) == -1){
		assert(0 && "Couldn't lock mutex");
	}
}

void SOUND::UnlockSourceList()
{
	if (SDL_mutexV(sourcelistlock) == -1){
		assert(0 && "Couldn't unlock mutex");
	}
}

void SOUND::Callback16bitStereo(void *myself, Uint8 *stream, int len)
{
	assert(this == myself);
	assert(initdone);

	// get sources
	LockSourceList();
	std::vector <SOUNDSOURCE *> & sources_active = *sources_active_p;
	std::vector <SOUNDSOURCE *> & sources_inactive = *sources_inactive_p;
	UnlockSourceList();

	// set buffers, clear stream
	int len4 = len / 4;
	buffer1.resize(len4);
	buffer2.resize(len4);
	memset(stream, 0, len);

	// sample active sources
	for (std::vector <SOUNDSOURCE *>::iterator s = sources_active.begin(); s != sources_active.end(); ++s)
	{
		SOUNDSOURCE * src = *s;
		src->SampleAndAdvanceWithPitch16bit(&buffer1[0], &buffer2[0], len4);
		for (int f = 0; f < src->NumFilters(); ++f)
		{
			src->GetFilter(f).Filter(&buffer1[0], &buffer2[0], len4);
		}
		volume_filter.Filter(&buffer1[0], &buffer2[0], len4);

		for (int i = 0; i < len4; ++i)
		{
			int pos = i * 2;
			((short *) stream)[pos] += buffer1[i];
			((short *) stream)[pos + 1] += buffer2[i];
		}
	}

	// increment inactive sources
	for (std::vector <SOUNDSOURCE *>::iterator s = sources_inactive.begin(); s != sources_inactive.end(); ++s)
	{
		(*s)->AdvanceWithPitch(len4);
	}
}

void SOUND::CallbackWrapper(void *sound, uint8_t *stream, int len)
{
	((SOUND*)sound)->Callback16bitStereo(sound, stream, len);
}
