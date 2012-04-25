#include "sound.h"

#include <SDL/SDL.h>
#include <algorithm>
#include <cassert>

static void SOUND_CallbackWrapper(void *soundclass, Uint8 *stream, int len)
{
	((SOUND*)soundclass)->Callback16bitstereo(soundclass, stream, len);
}

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
	activemax(0),
	inactivemax(0),
	sourcelistlock(0)
{
	volume_filter.SetFilterOrder0(1.0);
}

SOUND::~SOUND()
{
	if (initdone)
		SDL_CloseAudio();

	if (sourcelistlock)
		SDL_DestroyMutex(sourcelistlock);

	//std::cout << "Max active/inactive sound sources: " << activemax << "/" << inactivemax << std::endl;
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
	desired.callback = SOUND_CallbackWrapper;
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

void SOUND::Callback16bitstereo(void *myself, Uint8 *stream, int len)
{
	assert(this == myself);
	assert(initdone);

	// set buffers, clear stream
	int len4 = len / 4;
	buffer1.resize(len4);
	buffer2.resize(len4);
	memset(stream, 0, len);

	// sample active sources
	LockSourceList();
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
	UnlockSourceList();
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

void SOUND::DetermineActiveSources()
{
	LockSourceList();
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
	UnlockSourceList();
}

void SOUND::Compute3DEffects() const
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

void SOUND::LimitActiveSources()
{
	if (sources_active.size() <= max_active_sources)
		return;

	std::vector<SOUNDSOURCE*> active = sources_active;
	std::partial_sort(
		active.begin(),
		active.begin() + max_active_sources,
		active.end(),
		CompareSource);

	LockSourceList();

	sources_active.assign(
		active.begin(),
		active.begin() + max_active_sources);

	sources_inactive.insert(
		sources_inactive.end(),
		active.begin() + max_active_sources,
		active.end());

	UnlockSourceList();

	activemax = activemax < sources_active.size() ? sources_active.size() : activemax;
	inactivemax = inactivemax < sources_inactive.size() ? sources_inactive.size() : inactivemax;
	//std::cout << sources_active.size() << '/' << sources_inactive.size() << '\n';
	//std::cout << "gain " << sources_active[0]->GetComputedGain() << '/';
	//std::cout << sources_active.back()->GetComputedGain() << '\n';
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
