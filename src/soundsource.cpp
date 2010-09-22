#include "soundsource.h"

#include "stdint.h"

SOUNDSOURCE::SOUNDSOURCE() :
	sample_pos(0),
	sample_pos_remainder(0.0f),
	playing(0),
	loop(false),
	autodelete(false),
	gain(1.0),
	pitch(1.0),
	last_pitch(1.0),
	computed_gain1(1.0),
	computed_gain2(1.0),
	last_computed_gain1(0.0),
	last_computed_gain2(0.0),
	enable3d(true),
	relative_gain(1.0),
	buffer(NULL) 
{
	// ctor
}

void SOUNDSOURCE::Compute3D(const MATHVECTOR<float, 3> & listener_position, const QUATERNION<float> & listener_rotation)
{
	if (enable3d)
	{
		MATHVECTOR <float, 3> relvec = this->GetPosition() - listener_position;
		float len = relvec.Magnitude();
		if (len < 0.1)
		{
			relvec[2] = 0.1;
			len = relvec.Magnitude();
		}
		listener_rotation.RotateVector(relvec);
		float cgain = log(1000.0 / pow((double)len, 1.3)) / log(100.0);
		if (cgain > 1.0)
			cgain = 1.0;
		if (cgain < 0.0)
			cgain = 0.0;
		float xcoord = -relvec.Normalize()[1];
		float pgain1 = -xcoord;
		if (pgain1 < 0)
			pgain1 = 0;
		float pgain2 = xcoord;
		if (pgain2 < 0)
			pgain2 = 0;
		
		computed_gain1 = cgain * GetGain() * (1.0 - pgain1);
		computed_gain2 = cgain * GetGain() * (1.0 - pgain2);
	}
	else
	{
		computed_gain1 = GetGain();
		computed_gain2 = GetGain();
	}
}

bool SOUNDSOURCE::Audible() const
{
	bool canhear = (playing == 1) && (GetGain() > 0);

	return canhear;
}

void SOUNDSOURCE::SetGainSmooth(const float newgain, const float dt)
{
	//float coeff = dt*40.0;
	if (dt <= 0)
		return;

	//low pass filter

	//rate limit
	float ndt = dt * 4.0;
	float delta = newgain - gain;
	if (delta > ndt)
		delta = ndt;
	if (delta < -ndt)
		delta = -ndt;
	gain = gain + delta;
}

void SOUNDSOURCE::SetPitchSmooth(const float newpitch, const float dt)
{
	//float coeff = dt*40.0;
	if (dt > 0)
	{
		//low pass filter

		//rate limit
		float ndt = dt * 4.0;
		float delta = newpitch - pitch;
		if (delta > ndt)
			delta = ndt;
		if (delta < -ndt)
			delta = -ndt;
		pitch = pitch + delta;
	}
}

SOUNDFILTER & SOUNDSOURCE::GetFilter(int num)
{
	int curnum = 0;
	for (std::list <SOUNDFILTER>::iterator i = filters.begin(); i != filters.end(); ++i)
	{
		if (num == curnum)
			return *i;
		curnum++;
	}

	//cerr << __FILE__ << "," << __LINE__ << "Asked for a non-existant filter" << endl;
	//UNRECOVERABLE_ERROR_FUNCTION(__FILE__,__LINE__,"Asked for a non-existant filter");
	assert(0);
	return *filters.begin();
}

/*
#define PREFER_BRANCHING_OVER_MULTIPLY
#define FASTER_SOUNDBUFFER_ACCESS
#define FAST_SOUNDBUFFER_ACCESS

void SOUNDSOURCE::SampleAndAdvance16bit(int * chan1, int * chan2, int len)
{
	assert(buffer);
#ifdef FAST_SOUNDBUFFER_ACCESS
	//if (buffer->info.channels == 2)
	{
		unsigned int samples = buffer->info.GetSamples();

#ifdef FASTER_SOUNDBUFFER_ACCESS
		unsigned int remaining_len = len;
		unsigned int this_start = 0;
		while (remaining_len > 0)
		{
			unsigned int this_run = remaining_len;
			unsigned int this_end = this_start+this_run;

			if (playing)
			{
				if (sample_pos + remaining_len > samples)
				{
					this_run = samples - sample_pos;
					this_end = this_start+this_run;
				}

				if (buffer->info.GetChannels() == 2)
				{
					for (unsigned int i = this_start, n = sample_pos*2; i < this_end; i++,n+=2)
					{
						chan1[i] = ((short *)buffer->sound_buffer)[n];
						chan2[i] = ((short *)buffer->sound_buffer)[n+1];
						//#endif
					}
				}
				else //it is assumed that this means channels == 1
				{
					for (unsigned int i = this_start, n = sample_pos; i < this_end; i++, n++)
					{
						chan1[i] = chan2[i] = ((short *)buffer->sound_buffer)[n];
						//#endif
					}
				}
			}
			else
			{
				//cout << "Finishing run of " << this_run << endl;
				for (unsigned int i = this_start; i < this_end; i++)
				{
					chan1[i] = chan2[i] = 0;
				}
			}

			unsigned int newpos = sample_pos + this_run;
			if (newpos >= samples && !loop)
			{
				playing = 0;
				//cout << "Stopping play" << endl;
			}
			else
			{
				sample_pos = (newpos) % samples;
				//cout << newpos << "<" << samples << " loop: " << loop << endl;
			}

			//cout << remaining_len << " - " << this_run << endl;
			remaining_len -= this_run;
			this_start += this_run;
		}

		//cout << remaining_len << endl;
#else
		if (buffer->info.channels == 2)
		{
			for (int i = 0; i < len; i++)
			{
				if (playing)
				{
					int sample_idx = sample_pos*2;
					chan1[i] = ((short *)buffer->sound_buffer)[sample_idx];
					chan2[i] = ((short *)buffer->sound_buffer)[sample_idx+1];
					//#endif

					unsigned int newpos = sample_pos + 1;
					if (newpos >= samples && !loop)
						playing = 0;
					else
						sample_pos = (newpos) % samples;
				}
				else
				{
					chan1[i] = chan2[i] = 0;
				}
			}
		}
		else //channels == 1
		{
			for (int i = 0; i < len; i++)
			{
				if (playing)
				{
					chan1[i] = ((short *)buffer->sound_buffer)[sample_pos];
					chan2[i] = ((short *)buffer->sound_buffer)[sample_pos];
					//#endif

					int newpos = sample_pos + 1;
					if (!loop && newpos >= samples && playing)
						playing = 0;
					else
						sample_pos = (newpos) % samples;
				}
				else
				{
					chan1[i] = chan2[i] = 0;
				}
			}
		}
#endif
	}
#else
	for (int i = 0; i < len; i++)
	{
#ifdef PREFER_BRANCHING_OVER_MULTIPLY
		if (playing)
		{
			chan1[i] = (int) (buffer->GetSample16bit(1, sample_pos)*computed_gain1);
			chan2[i] = (int) (buffer->GetSample16bit(2, sample_pos)*computed_gain2);

			Increment(1);
		}
		else
		{
			chan1[i] = chan2[i] = 0;
		}
#else
		chan1[i] = (int) (buffer->GetSample16bit(1, sample_pos)*playing*computed_gain1);
		chan2[i] = (int) (buffer->GetSample16bit(2, sample_pos)*playing*computed_gain2);

		//stop playing if at the end of loop and not looping
		//this line is a complicated piece of engineering to avoid branching...
		//consider that (sample_pos + 1) / buffer.GetPtr()->GetSoundInfo().GetSamples() is 1 if we're at the end, 0 if we're not
		//let e = (sample_pos + 1) / buffer.GetPtr()->GetSoundInfo().GetSamples()
		//let l = loop
		//let p = playing
		//X means "don't care"
		//we want:
		//e=0 && l=X => p=1
		//e=1 && l=1 => p=1
		//e=1 && l=0 => p=0
		playing = 1 - ((sample_pos + 1) / samples)*(1-loop);

		sample_pos = (sample_pos + 1*playing) % samples;
#endif
	}
#endif
}

inline void SOUNDSOURCE::Increment(int num)
{
	int samples = buffer->GetSoundInfo().GetSamples()/buffer->GetSoundInfo().GetChannels();

	int newpos = sample_pos + num;
	if (newpos >= samples && !loop)
		playing = 0;
	else
		sample_pos = (newpos) % samples;
}

//#define RESAMPLE_QUALITY_LOWEST
//#define RESAMPLE_QUALITY_MEDIUM
//#define RESAMPLE_QUALITY_FAST_MEDIUM
#define RESAMPLE_QUALITY_FASTER_MEDIUM
//#define RESAMPLE_QUALITY_HIGH*/

///output buffers chan1 (left) and chan2 (right) will be filled with "len" 16-bit samples
void SOUNDSOURCE::SampleAndAdvanceWithPitch16bit(int * chan1, int * chan2, int len)
{
	assert(buffer);
	int samples = buffer->info.samples; //the number of 16-bit samples in the buffer with left and right channels SUMMED (so for stereo signals the number of samples per channel is samples/2)

	float n_remain = sample_pos_remainder; //the fractional portion of the current playback position for this soundsource
	int n = sample_pos; //the integer portion of the current playback position for this soundsource PER CHANNEL (i.e., this will range from 0 to samples/2)

	const int chan = buffer->info.channels;

	samples -= samples % chan; //correct the number of samples in odd situations where we have stereo channels but an odd number of channels

	const int samples_per_channel = samples / chan; //how many 16-bit samples are in a channel of audio

	assert((int)sample_pos <= samples_per_channel);

	last_pitch = pitch; //this bypasses pitch rate limiting, because it isn't a very useful effect, turns out.

	if (playing)
	{
		assert(len > 0);

		int samp1[2]; //sample(s) to the left of the floating point sample position
		int samp2[2]; //sample(s) to the right of the floating point sample position

		int idx = n*chan; //the integer portion of the current 16-bit playback position in the input buffer, accounting for duplication of samples for stereo waveforms

		const float maxrate = 1.0/(44100.0*0.01); //the maximum allowed rate of gain change per sample
		const float negmaxrate = -maxrate;
		//const float maxpitchrate = maxrate; //the maximum allowed rate of pitch change per sample
		//const float negmaxpitchrate = -maxpitchrate;
		int16_t * buf = (int16_t *)buffer->sound_buffer; //access to the 16-bit sound input buffer
		const int chaninc = chan - 1; //the offset to use when fetching the second channel from the sound input buffer (yes, this will be zero for a mono sound buffer)

		float gaindiff1(0), gaindiff2(0);//, pitchdiff(0); //allocate memory here so we don't have to in the loop
		
		float samplecount = 0; //floating point record of how far the playback position has increased during this callback
		
		for (int i = 0; i  < len; ++i)
		{
			//do gain change rate limiting
			gaindiff1 = computed_gain1 - last_computed_gain1;
			gaindiff2 = computed_gain2 - last_computed_gain2;
			if (gaindiff1 > maxrate)
				gaindiff1 = maxrate;
			else if (gaindiff1 < negmaxrate)
				gaindiff1 = negmaxrate;
			if (gaindiff2 > maxrate)
				gaindiff2 = maxrate;
			else if (gaindiff2 < negmaxrate)
				gaindiff2 = negmaxrate;
			last_computed_gain1 = last_computed_gain1 + gaindiff1;
			last_computed_gain2 = last_computed_gain2 + gaindiff2;

			//do pitch change rate limiting
			/*pitchdiff = pitch - last_pitch;
			if (pitchdiff > maxpitchrate)
				pitchdiff = maxpitchrate;
			else if (pitchdiff < negmaxpitchrate)
				pitchdiff = negmaxpitchrate;
			last_pitch = last_pitch + pitchdiff;*/

			if (n >= samples_per_channel && !loop) //end playback if we've finished playing the buffer and looping is not enabled
			{
				//stop playback
				chan1[i] = chan2[i] = 0;
				playing = 0;
			}
			else //if not at the end of a non-looping sample, or if the sample is looping
			{
				idx = chan*(n % samples_per_channel); //recompute the buffer position accounting for looping

				assert(idx+chaninc < samples); //make sure we don't read past the end of the buffer

				samp1[0] = buf[idx]; //the sample to the left of the playback position, channel 0
				samp1[1] = buf[idx+chaninc]; //the sample to the left of the playback position, channel 1

				idx = (idx + chan) % samples;

				assert(idx+chaninc < samples); //make sure we don't read past the end of the buffer

				samp2[0] = buf[idx]; //the sample to the right of the playback position, channel 0
				samp2[1] = buf[idx+chaninc]; //the sample to the right of the playback position, channel 1

				//samp2[0] = samp1[0]; //disable interpolation, for debug purposes
				//samp2[1] = samp1[1];

				chan1[i] = (int) ((n_remain*(samp2[0] - samp1[0]) + samp1[0])*last_computed_gain1); //set the output buffer to the linear interpolation between the left and right samples for channel 0
				chan2[i] = (int) ((n_remain*(samp2[1] - samp1[1]) + samp1[1])*last_computed_gain2); //set the output buffer to the linear interpolation between the left and right samples for channel 1

				n_remain += last_pitch; //increment the playback position
				const unsigned int ninc = (unsigned int) n_remain;
				n += ninc; //update the integer portion of the playback position
				n_remain -= (float) ninc; //update the fractional portion of the playback position

				samplecount += last_pitch; //increment the playback delta position counter.  this will eventually be added to the soundsource playback position variables.
			}
		}

		double newpos = sample_pos + sample_pos_remainder + samplecount; //calculate a floating point new playback position based on where we started plus how many samples we just played

		if (newpos >= samples_per_channel && !loop) //end playback if we've finished playing the buffer and looping is not enabled
			playing = 0;
		else //if not at the end of a non-looping sample, or if the sample is looping
		{
			while (newpos >= samples_per_channel) //this while loop is like a floating point modulo operation that sets newpos = newpos % samples
				newpos -= samples_per_channel;
			sample_pos = (unsigned int) newpos; //save the integer portion of the current playback position back to the soundsource
			sample_pos_remainder = newpos - sample_pos; //save the fractional portion of the current playback position back to the soundsource
		}
		
		if (playing)
			assert((int)sample_pos <= samples_per_channel);
		//assert(0);
	}
	else //if not playing
	{
		for (int i = 0; i  < len; ++i)
		{
			chan1[i] = chan2[i] = 0; //fill output buffers with silence
		}
	}
}

void SOUNDSOURCE::IncrementWithPitch(int num)
{
	int samples = buffer->GetSoundInfo().samples / buffer->GetSoundInfo().channels;
	double newpos = sample_pos + sample_pos_remainder + (num)*pitch;
	if (newpos >= samples && !loop)
		playing = 0;
	else
	{
		while (newpos >= samples)
			newpos -= samples;
		sample_pos = (unsigned int) newpos;
		sample_pos_remainder = newpos - sample_pos;
	}
}

/*void SOUNDSOURCE::Sample16bit(unsigned int peekoffset, int & chan1, int & chan2)
{
	//lines below are used for performance testing to find bottlenecks
	//chan1 = 123;
	//chan2 = 123;
	//chan1 = buffer->GetSample16bit(1, peekoffset);
	//chan2 = buffer->GetSample16bit(2, peekoffset);

	int samples = buffer->GetSoundInfo().GetSamples();

#ifdef PREFER_BRANCHING_OVER_MULTIPLY
	if (playing)
	{
		int newsamplepos = (sample_pos + peekoffset);
		if (newsamplepos < samples)
		{
			chan1 = (int) (buffer->GetSample16bit(1, newsamplepos)*computed_gain1);
			chan2 = (int) (buffer->GetSample16bit(2, newsamplepos)*computed_gain2);
		}
		else if (loop)
		{
			int peekpos = (sample_pos + peekoffset) % samples;
			chan1 = (int) (buffer->GetSample16bit(1, peekpos)*computed_gain1);
			chan2 = (int) (buffer->GetSample16bit(2, peekpos)*computed_gain2);
		}
		else
		{
			chan1 = chan2 = 0;
		}
	}
	else
	{
		chan1 = chan2 = 0;
	}
#else
	int peekplaying = 1 - ((sample_pos + peekoffset) / samples)*(1-loop);
	peekplaying *= playing;
	int peekpos = (sample_pos + peekoffset*peekplaying) % samples;
	chan1 = (int) (buffer->GetSample16bit(1, peekpos)*computed_gain1*peekplaying);
	chan2 = (int) (buffer->GetSample16bit(2, peekpos)*computed_gain2*peekplaying);
#endif
}

void SOUNDSOURCE::Advance(unsigned int offset)
{
	playing = 1 - ((sample_pos + offset) / buffer->GetSoundInfo().GetSamples())*(1-loop);

	sample_pos = (sample_pos + offset*playing) % buffer->GetSoundInfo().GetSamples();
}*/
