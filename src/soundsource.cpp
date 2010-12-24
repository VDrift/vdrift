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
	effects3d(true),
	relative_gain(1.0)
{
	// ctor
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
	assert(0);
	SOUNDFILTER * nullfilt = NULL;
	return *nullfilt;
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

///output buffers chan1 (left) and chan2 (right) will be filled with "len" 16-bit samples
void SOUNDSOURCE::SampleAndAdvanceWithPitch16bit(int * chan1, int * chan2, int len)
{
	assert(buffer);
	int samples = buffer->info.samples; //the number of 16-bit samples in the buffer with left and right channels SUMMED (so for stereo signals the number of samples per channel is samples/2)

	float n_remain = sample_pos_remainder; //the fractional portion of the current playback position for this soundsource
	int n = sample_pos; //the integer portion of the current playback position for this soundsource PER CHANNEL (i.e., this will range from 0 to samples/2)

	float samplecount = 0; //floating point record of how far the playback position has increased during this callback

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
	int samples = buffer->GetSoundInfo().samples/buffer->GetSoundInfo().channels;
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
