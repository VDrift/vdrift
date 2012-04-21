#include "soundsource.h"
#include "stdint.h"

template <class T>
static inline T clamp(T val, T min, T max)
{
	return val > min ? (val < max ? val : max) : min;
}

SOUNDSOURCE::SOUNDSOURCE() :
	samples_per_channel(0),
	samples(0),
	sample_pos(0),
	sample_pos_remainder(0),
	pitch(denom),
	computed_gain1(denom),
	computed_gain2(denom),
	last_computed_gain1(0),
	last_computed_gain2(0),
	playing(false),
	loop(false),
	autodelete(false),
	effects3d(true),
	gain(1.0)
{
	// ctor
}

void SOUNDSOURCE::SampleAndAdvanceWithPitch16bit(int * chan1, int * chan2, int len)
{
	if (playing)
	{
		assert(buffer);
		assert(len > 0);

		int chan = buffer->info.channels;
		int chaninc = chan - 1;
		int nr = sample_pos_remainder;
		int ni = sample_pos;
		int16_t * buf = (int16_t *)buffer->sound_buffer;

		for (int i = 0; i < len; ++i)
		{
			// limit gain change rate
			const int maxrate = (denom * 100) / 44100;
			int gaindiff1 = computed_gain1 - last_computed_gain1;
			int gaindiff2 = computed_gain2 - last_computed_gain2;
			gaindiff1 = clamp(gaindiff1, -maxrate, maxrate);
			gaindiff2 = clamp(gaindiff2, -maxrate, maxrate);
			last_computed_gain1 += gaindiff1;
			last_computed_gain2 += gaindiff2;

			if (!loop)
			{
				// finish playing the buffer if looping is not enabled
				chan1[i] = chan2[i] = 0;
				playing = (ni < samples_per_channel);
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
				int val1 = (nr * samp20 + (denom - nr) * samp10) / denom;
				int val2 = (nr * samp21 + (denom - nr) * samp11) / denom;
				val1 = (val1 * last_computed_gain1) / denom;
				val2 = (val2 * last_computed_gain2) / denom;

				// fill output buffers
				chan1[i] = val1;
				chan2[i] = val2;

				// advance playback position
				nr += pitch;
				int ninc = nr / denom;
				ni += ninc;
				nr -= ninc * denom;
			}
		}

		sample_pos = ni;
		sample_pos_remainder = nr;
		if (!loop)
		{
			// no loop, finish playing
			playing = (sample_pos < samples_per_channel);
		}
		else
		{
			// loop buffer
			sample_pos = sample_pos % samples_per_channel;
		}
	}
	else
	{
		// not playing, fill output buffers with silence
		for (int i = 0; i < len; ++i)
		{
			chan1[i] = chan2[i] = 0;
		}
	}
}

void SOUNDSOURCE::IncrementWithPitch(int num)
{
	sample_pos_remainder += num * pitch;
	int delta = sample_pos_remainder / denom;
	sample_pos_remainder -= delta * denom;
	sample_pos += delta;

	if (!loop)
	{
		playing = (sample_pos < samples_per_channel);
	}
	else
	{
		sample_pos = sample_pos % samples_per_channel;
	}
}
