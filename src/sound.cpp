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
	gain_estimate(1.0),
	disable(false),
	sourcelistlock(NULL)
{
	volume_filter.SetFilterOrder0(1.0);
}

void CallbackWrapper(void *soundclass, Uint8 *stream, int len)
{
	((SOUND*)soundclass)->Callback16bitStereo(soundclass, stream, len);
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
	desired.callback = CallbackWrapper;
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

	deviceinfo.samples = samples;
	deviceinfo.frequency = frequency;
	deviceinfo.channels = channels;
	deviceinfo.bytespersample = bytespersample;

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

void SOUND::Callback16bitStereo(void *myself, unsigned char *stream, int len)
{
	assert(this == myself);
	assert(initdone);

	std::list <SOUNDSOURCE *> active_sourcelist;
	std::list <SOUNDSOURCE *> inactive_sourcelist;
	
	LockSourceList();
	
	DetermineActiveSources(active_sourcelist, inactive_sourcelist);
	Compute3DEffects(active_sourcelist, lpos, lrot);

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

SOUND::~SOUND()
{
	if (initdone)
	{
		SDL_CloseAudio();
	}
	
	if (sourcelistlock)
	{
		SDL_DestroyMutex(sourcelistlock);
	}
}

void SOUND::CollectGarbage()
{
	if (disable) return;

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
	if (disable) return;

	assert(todel);

	std::list <SOUNDSOURCE *>::iterator delit = sourcelist.end();
	for (std::list <SOUNDSOURCE *>::iterator i = sourcelist.begin(); i != sourcelist.end(); ++i)
	{
		if (*i == todel)
			delit = i;
	}

	LockSourceList();
	if (delit != sourcelist.end())
	{
		sourcelist.erase(delit);
	}
	UnlockSourceList();
}

void SOUND::Compute3DEffects(std::list <SOUNDSOURCE *> & sources, const MATHVECTOR <float, 3> & listener_pos, const QUATERNION <float> & listener_rot) const
{
	for (std::list <SOUNDSOURCE *>::iterator i = sources.begin(); i != sources.end(); ++i)
	{
		SOUNDSOURCE & source = **i;
		source.Compute3D(listener_pos, listener_rot);
	}
}

void SOUND::LockSourceList()
{
	if(SDL_mutexP(sourcelistlock) == -1)
	{
		assert(0 && "Couldn't lock mutex");
	}
}

void SOUND::UnlockSourceList()
{
	if(SDL_mutexV(sourcelistlock) == -1)
	{
		assert(0 && "Couldn't unlock mutex");
	}
}

/*
const TESTER & SOUND::Test()
{
	int points = 0;

	//test loading a wave file
	SOUNDBUFFER testbuffer;
	SOUNDINFO sinfo;
	sinfo.Set(4096, 44100, 2, 2);

	string testogg = "test/oggtest";
	testbuffer.Load(PATHMANAGER_FUNCTION_CALL(testogg+".ogg"), sinfo);
	points = 0;
	if (testbuffer.GetSoundInfo().GetFrequency() == 44100) points++;
	if (testbuffer.GetSoundInfo().GetBytesPerSample() == 2) points++;
	if (testbuffer.GetSoundInfo().GetChannels() == 2) points++;
	if (testbuffer.GetSoundInfo().GetSamples() == 1412399) points++;
	if (testbuffer.GetSample16bit(1,500000) == -4490) points++;
	//cout << testbuffer.GetSample16bit(1,500000) << std::endl;
	mytest.SubTestComplete("OGG Vorbis loading", points, 5);

	string testwav = "test/44k_s16_c2";
	testbuffer.Load(PATHMANAGER_FUNCTION_CALL(testwav+".wav"), sinfo);
	points = 0;
	if (testbuffer.GetSoundInfo().GetFrequency() == 44100) points++;
	if (testbuffer.GetSoundInfo().GetBytesPerSample() == 2) points++;
	if (testbuffer.GetSoundInfo().GetChannels() == 2) points++;
	if (testbuffer.GetSoundInfo().GetSamples() == 35952) points++;
	if (testbuffer.GetSample16bit(1,1024) == 722) points++;
	mytest.SubTestComplete("Wave loading 44k s16 c2", points, 5);

	SOUNDSOURCE testsrc;
	testsrc.SetBuffer(testbuffer);
	points = 0;
	testsrc.SeekToSample(1024);
	int out1, out2;
	testsrc.SampleAndAdvance16bit(&out1, &out2, 1);
	//cout << out1 << "," << out2 << std::endl;
	if (out1 == 0 && out2 == 0) points++;// else cout << "fail1" << std::endl;
	testsrc.Play();
	testsrc.SeekToSample(1014);
	testsrc.Sample16bit(10, out1, out2);
	if (out1 == 722 && out2 == -224) points++;// else cout << "fail2" << std::endl;
	testsrc.Advance(10);
	testsrc.SampleAndAdvance16bit(&out1, &out2, 1);
	if (out1 == 722 && out2 == -224) points++;// else cout << "fail3" << std::endl;
	//cout << out1 << "," << out2 << std::endl;
	mytest.SubTestComplete("Sound source testing", points, 3);

	SDL_Init(0);
	{
		points = 0;
		testsrc.SeekToSample(0);
		testsrc.Loop(true);
		unsigned int numsources = 32;
		unsigned int startticks = SDL_GetTicks();
		for (unsigned int i = 0; i < 44100; i++)
		{
			int accum1 = 0;
			int accum2 = 0;
			for (unsigned int s = 0; s < numsources; s++)
			{
				//testsrc.SampleAndAdvance16bit(out1, out2);
				testsrc.Sample16bit(i, out1, out2);
				accum1 += out1;
				accum2 += out2;
			}
		}
		unsigned int endticks = SDL_GetTicks();
		stringstream msg;
		msg << "Performance testing Sample16bit (" << endticks - startticks << "ms for 1 second of 44.1khz stereo audio with 32 sources)";
		if (endticks - startticks < 500)
			points++;
		mytest.SubTestComplete(msg.str(), points, 1);
	}
	{
		points = 0;
		testsrc.SeekToSample(0);
		//testsrc.Loop(false);
		//testsrc.Loop(true);
		unsigned int numsources = 32;
		unsigned int startticks = SDL_GetTicks();
		short stream[44100*2];
		int buffer1[44100];
		int buffer2[44100];
		for (unsigned int s = 0; s < numsources; s++)
		{
			testsrc.SampleAndAdvance16bit(buffer1, buffer2, 44100);
			if (s == 0)
			{
				for (int i = 0; i < 44100; i++)
				{
					int pos = i*2;
					((short *) stream)[pos] = buffer1[i];
					((short *) stream)[pos+1] = buffer2[i];
				}
			}
		}
		unsigned int endticks = SDL_GetTicks();
		stringstream msg;
		msg << "Performance testing SampleAndAdvance16bit (" << endticks - startticks << "ms for 1 second of 44.1khz stereo audio with 32 sources)";
		if (endticks - startticks < 500)
			points++;
		mytest.SubTestComplete(msg.str(), points, 1);
		testsrc.Loop(true);
		//cout << "Done" << std::endl;
	}
	{
		points = 0;
		testsrc.SeekToSample(0);
		testsrc.Loop(true);
		testsrc.SetPitch(0.7897878);
		//testsrc.SetPitch(0.1);
		unsigned int numsources = 32;
		unsigned int startticks = SDL_GetTicks();
		short stream[44100*2];
		int buffer1[44100];
		int buffer2[44100];
		for (unsigned int s = 0; s < numsources; s++)
		{
			testsrc.SampleAndAdvanceWithPitch16bit(buffer1, buffer2, 44100);
			if (s == 0)
			{
				for (int i = 0; i < 44100; i++)
				{
					int pos = i*2;
					((short *) stream)[pos] = buffer1[i];
					((short *) stream)[pos+1] = buffer2[i];
				}
			}
		}

		unsigned int endticks = SDL_GetTicks();
		stringstream msg;
		msg << "Performance testing SampleAndAdvanceWithPitch16bit (" << endticks - startticks << "ms for 1 second of 44.1khz stereo audio with 32 sources)";
		if (endticks - startticks < 500)
			points++;
		mytest.SubTestComplete(msg.str(), points, 1);
	}
	{
		points = 0;
		testsrc.SeekToSample(0);
		testsrc.Loop(true);
		testsrc.SetPitch(0.7897878);
		SOUNDFILTER filter;
		filter.SetFilterOrder1(0.1,0,0.9);
		unsigned int numsources = 32;
		unsigned int startticks = SDL_GetTicks();
		short stream[44100*2];
		int buffer1[44100];
		int buffer2[44100];
		for (unsigned int s = 0; s < numsources; s++)
		{
			testsrc.SampleAndAdvanceWithPitch16bit(buffer1, buffer2, 44100);
			filter.Filter(buffer1, buffer2, 44100);
			if (s == 0)
			{
				for (int i = 0; i < 44100; i++)
				{
					int pos = i*2;
					((short *) stream)[pos] = buffer1[i];
					((short *) stream)[pos+1] = buffer2[i];
				}
			}
		}

		unsigned int endticks = SDL_GetTicks();
		stringstream msg;
		msg << "Performance testing SampleAndAdvanceWithPitch16bit with 1st order filter (" << endticks - startticks << "ms for 1 second of 44.1khz stereo audio with 32 sources)";
		if (endticks - startticks < 1000)
			points++;
		mytest.SubTestComplete(msg.str(), points, 1);
	}
	SDL_Quit();

	testsrc.SeekToSample(1014);
	testsrc.Sample16bit(10, out1, out2);
	deviceinfo = sinfo;
	LoadBuffer(testwav);
	SOUNDSOURCE & testsrc2 = NewSource(testwav);
	testsrc2.SetPitch(0.7);
	testsrc2.Loop(true);
	testsrc2.Play();
	{
		int numsamp = 512;
		Uint8 stream[numsamp];
		loaded = true;
		initdone = true;
		testsrc.Play();
		Callback16bitstereo(NULL, stream, numsamp);
		testsrc.Stop();
		initdone = false;
		loaded = false;
		ofstream testdump;
		testdump.open(PATHMANAGER_FUNCTION_CALL("test/testdump.txt").c_str());
		ifstream testgoal;
		testgoal.open(PATHMANAGER_FUNCTION_CALL("test/testgoal.txt").c_str());
		//points = 256*2;
		points = 0;
		for (int i = 0; i < numsamp; i++)
		{
			short goal;
			testgoal >> goal;
			testdump << dec << (short)stream[i] << std::endl;
			if (stream[i] - goal < 2 && goal - stream[i] < 2)
				points++;
		//	else
		//		cout << (short)goal << " != " << (short)stream[i] << std::endl;
		}
		testdump.close();
		mytest.SubTestComplete("Sound callback testing", points, numsamp);
	}
	points = 0;
	testsrc2.SeekToSample(1024);
	int out3, out4;
	testsrc2.SampleAndAdvance16bit(&out3, &out4, 1);
	//cout << out1 << "," << out2 << std::endl;
	if (out1 == out3 && out2 == out4) points++;
	SOUNDSOURCE & testsrc3 = NewSource(testwav);
	testsrc3.Play();
	std::list <SOUNDSOURCE *> active_sourcelist;
	std::list <SOUNDSOURCE *> inactive_sourcelist;
	DetermineActiveSources(active_sourcelist, inactive_sourcelist);
	if (active_sourcelist.size() == 2) points++;
	testsrc3.Stop();
	DetermineActiveSources(active_sourcelist, inactive_sourcelist);
	if (active_sourcelist.size() == 1 && inactive_sourcelist.size() == 1) points++;
	testsrc2.Stop();
	DetermineActiveSources(active_sourcelist, inactive_sourcelist);
	if (active_sourcelist.size() == 0 && inactive_sourcelist.size() == 2) points++;
	DeleteSource(&testsrc3);
	if (sourcelist.size() == 1) points++;
	DeleteSource(&testsrc2);
	if (sourcelist.size() == 0) points++;
	mytest.SubTestComplete("Sound management testing", points, 6);

	points = 0;
	SOUNDSOURCE & testsrc4 = NewSource(testwav);
	active_sourcelist.clear();
	active_sourcelist.push_back(&testsrc4);
	testsrc4.Enable3Ds(false);
	testsrc4.SetGain(0.5);
	VERTEX spos;
	spos.Set(300,0,0);
	testsrc4.SetPosition(spos);
	VERTEX lpos;
	QUATERNION lrot;
	Compute3DEffects(active_sourcelist, lpos, lrot);
	if (testsrc4.ComputedGain(1) == 0.5 && testsrc4.ComputedGain(2) == 0.5) points++;
	testsrc4.Enable3D(true);
	Compute3DEffects(active_sourcelist, lpos, lrot);
	if (testsrc4.ComputedGain(2) == 0.0) points++;
	if (testsrc4.ComputedGain(1) < 0.1) points++;
	mytest.SubTestComplete("Sound effects testing", points, 3);

	{
		points = 0;
		SOUNDFILTER filt1;
		int size = 10;
		int data1[size];
		int data2[size];
		for (int i = 0; i < size; i++)
		{
			if (i < 3)
			{
				data1[i] = data2[i] = 0;
			}
			else
			{
				data1[i] = 1000;
				data2[i] = 0;
			}
		}
		float xc[2];
		float yc[2];
		xc[0] = 0.1; //simple low-pass filter
		xc[1] = 0;
		yc[0] = 0;
		yc[1] = 0.9;
		filt1.SetFilter(1, xc, yc);
		filt1.Filter(data1,data2,5);
		if (data1[3] == 100 && data2[3] == 0) points++;
		filt1.Filter(&(data1[5]),&(data2[5]),5);
		if (data1[6] == 342 && data1[9] == 519 && data2[9] == 0) points++;
		//cout << data1[6] << std::endl;
		//for (int i = 0; i < 10; i++) cout << i << ". " << data1[i] << std::endl;

		mytest.SubTestComplete("Sound filter testing", points, 2);
	}

	return mytest;
}*/
