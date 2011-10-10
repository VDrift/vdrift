#include "sound.h"
#include "unittest.h"

#include <SDL/SDL.h>

#include <cassert>
#include <sstream>
#include <iostream>
#include <string>
#include <list>

SOUND::SOUND() :
	initdone(false),
	paused(true),
	deviceinfo(0,0,0,0),
	gain_estimate(1.0),
	disable(false),
	sourcelistlock(0)
{
	volume_filter.SetFilterOrder0(1.0);
}

SOUND::~SOUND()
{
	if (initdone)
	{
		SDL_CloseAudio();
	}

	if (sourcelistlock)
		SDL_DestroyMutex(sourcelistlock);
}

void SOUND_CallbackWrapper(void *soundclass, Uint8 *stream, int len)
{
	((SOUND*)soundclass)->Callback16bitstereo(soundclass, stream, len);
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
		//string error = SDL_GetError();
		//UNRECOVERABLE_ERROR_FUNCTION(__FILE__,__LINE__,"Error opening audio device.");
		error_output << "Error opening audio device, disabling sound." << std::endl;
		//throw EXCEPTION(__FILE__, __LINE__, "Error opening audio device: " + error);
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
		//cout << "Warning: obtained audio format isn't the same as the desired format!" << std::endl;
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
	//cout << dout.str() << std::endl;
	if (bytespersample != 2 || obtained.channels != desired.channels || obtained.freq != desired.freq)
	{
		//throw EXCEPTION(__FILE__, __LINE__, "Sound did not create a 44.1kHz, 16 bit, stereo device as requested.");
		//cerr << __FILE__ << "," << __LINE__ << ": Sound interface did not create a 44.1kHz, 16 bit, stereo device as requested.  Disabling game.sound." << std::endl;
		error_output << "Sound interface did not create a 44.1kHz, 16 bit, stereo device as requested.  Disabling sound." << std::endl;
		Disable();
		return false;
	}

	deviceinfo = SOUNDINFO(samples, frequency, channels, bytespersample);

	initdone = true;

	SetMasterVolume(1.0);

	return true;
}

void SOUND::Pause(const bool pause_on)
{
	if (paused == pause_on) //take no action if no change
		return;

	paused = pause_on;
	if (pause_on)
	{
		//cout << "sound pause on" << std::endl;
		SDL_PauseAudio(1);
	}
	else
	{
		//cout << "sound pause off" << std::endl;
		SDL_PauseAudio(0);
	}
}

void SOUND::Callback16bitstereo(void *myself, Uint8 *stream, int len)
{
	assert(this == myself);
	assert(initdone);

	std::list <SOUNDSOURCE *> active_sourcelist;
	std::list <SOUNDSOURCE *> inactive_sourcelist;

	LockSourceList();

	DetermineActiveSources(active_sourcelist, inactive_sourcelist);
	Compute3DEffects(active_sourcelist, lpos, lrot);//, cam.GetPosition().ScaleR(-1), cam.GetRotation());

	//increment inactive sources
	for (std::list <SOUNDSOURCE *>::iterator s = inactive_sourcelist.begin(); s != inactive_sourcelist.end(); s++)
	{
		(*s)->IncrementWithPitch(len/4);
	}

	int * buffer1 = new int[len/4];
	int * buffer2 = new int[len/4];
	for (std::list <SOUNDSOURCE *>::iterator s = active_sourcelist.begin(); s != active_sourcelist.end(); s++)
	{
		SOUNDSOURCE * src = *s;
		src->SampleAndAdvanceWithPitch16bit(buffer1, buffer2, len/4);
		for (int f = 0; f < src->NumFilters(); f++)
		{
			src->GetFilter(f).Filter(buffer1, buffer2, len/4);
		}
		volume_filter.Filter(buffer1, buffer2, len/4);
		if (s == active_sourcelist.begin())
		{
			for (int i = 0; i < len/4; i++)
			{
				int pos = i*2;
				((short *) stream)[pos] = (buffer1[i]);
				((short *) stream)[pos+1] = (buffer2[i]);
			}
		}
		else
		{
			for (int i = 0; i < len/4; i++)
			{
				int pos = i*2;
				((short *) stream)[pos] += (buffer1[i]);
				((short *) stream)[pos+1] += (buffer2[i]);
			}
		}
	}
	delete [] buffer1;
	delete [] buffer2,

	UnlockSourceList();

	//cout << active_sourcelist.size() << "," << inactive_sourcelist.size() << std::endl;

	if (active_sourcelist.empty())
	{
		for (int i = 0; i < len/4; i++)
		{
			int pos = i*2;
			((short *) stream)[pos] = ((short *) stream)[pos+1] = 0;
		}
	}

	CollectGarbage();

	//cout << "Callback: " << len << std::endl;
}

void SOUND::CollectGarbage()
{
	if (disable)
		return;

	std::list <SOUNDSOURCE *> todel;
	for (std::list <SOUNDSOURCE *>::iterator i = sourcelist.begin(); i != sourcelist.end(); ++i)
	{
		if (!(*i)->Audible() && (*i)->GetAutoDelete())
		{
			todel.push_back(*i);
		}
	}

	for (std::list <SOUNDSOURCE *>::iterator i = todel.begin(); i != todel.end(); ++i)
	{
		RemoveSource(*i);
	}

	//cout << sourcelist.size() << std::endl;
}

void SOUND::DetermineActiveSources(std::list <SOUNDSOURCE *> & active_sourcelist, std::list <SOUNDSOURCE *> & inaudible_sourcelist) const
{
	active_sourcelist.clear();
	inaudible_sourcelist.clear();
	//int sourcenum = 0;
	for (std::list <SOUNDSOURCE *>::const_iterator i = sourcelist.begin(); i != sourcelist.end(); ++i)
	{
		if ((*i)->Audible())
		{
			active_sourcelist.push_back(*i);
			//cout << "Tick: " << &(*i) << std::endl;
			//cout << "Source is audible: " << i->GetName() << ", " << i->GetGain() << ":" << i->ComputedGain(1) << "," << i->ComputedGain(2) << std::endl;
			//cout << "Source " << sourcenum << " is audible: " << i->GetName() << std::endl;
		}
		else
		{
			inaudible_sourcelist.push_back(*i);
		}
		//sourcenum++;
	}
	//cout << "sounds active: " << active_sourcelist.size() << ", sounds inactive: " << inaudible_sourcelist.size() << std::endl;
}

void SOUND::RemoveSource(SOUNDSOURCE * todel)
{
	if (disable)
		return;

	assert(todel);

	std::list <SOUNDSOURCE *>::iterator delit = sourcelist.end();
	for (std::list <SOUNDSOURCE *>::iterator i = sourcelist.begin(); i != sourcelist.end(); ++i)
	{
		if (*i == todel)
			delit = i;
	}

	//assert(delit != sourcelist.end()); //can't find source to delete //update: don't assert, just do a check

	LockSourceList();
	if (delit != sourcelist.end())
		sourcelist.erase(delit);
	UnlockSourceList();
}

void SOUND::Compute3DEffects(std::list <SOUNDSOURCE *> & sources, const MATHVECTOR <float, 3> & listener_pos, const QUATERNION <float> & listener_rot) const
{
	for (std::list <SOUNDSOURCE *>::iterator i = sources.begin(); i != sources.end(); ++i)
	{
		if ((*i)->Get3DEffects())
		{
			MATHVECTOR <float, 3> relvec = (*i)->GetPosition() - listener_pos;
			//std::cout << "sound pos: " << (*i)->GetPosition() << std::endl;;
			//std::cout << "listener pos: " << listener_pos << std::endl;;
			//cout << "listener pos: ";listener_pos.DebugPrint();
			//cout << "camera pos: ";cam.GetPosition().ScaleR(-1.0).DebugPrint();
			float len = relvec.Magnitude();
			if (len < 0.1)
			{
				relvec[2] = 0.1;
				len = relvec.Magnitude();
			}
			listener_rot.RotateVector(relvec);
			float cgain = log(1000.0 / pow((double)len, 1.3)) / log(100.0);
			if (cgain > 1.0)
				cgain = 1.0;
			if (cgain < 0.0)
				cgain = 0.0;
			float xcoord = -relvec.Normalize()[1];
			//std::cout << (*i)->GetPosition() << " || " << listener_pos << " || " << xcoord << std::endl;
			float pgain1 = -xcoord;
			if (pgain1 < 0)
				pgain1 = 0;
			float pgain2 = xcoord;
			if (pgain2 < 0)
				pgain2 = 0;
			//cout << cgain << std::endl;
			//cout << xcoord << std::endl;
			(*i)->SetComputationResults(cgain*(*i)->GetGain()*(1.0-pgain1), cgain*(*i)->GetGain()*(1.0-pgain2));
		}
		else
		{
			(*i)->SetComputationResults((*i)->GetGain(), (*i)->GetGain());
		}
	}
}

void SOUND::LockSourceList()
{
	if (SDL_mutexP(sourcelistlock)==-1){
		assert(0 && "Couldn't lock mutex");
	}
}

void SOUND::UnlockSourceList()
{
	if (SDL_mutexV(sourcelistlock)==-1){
		assert(0 && "Couldn't unlock mutex");
	}
}