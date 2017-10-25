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

#include "soundbuffer.h"
#include "endian_utility.h"

#ifdef __APPLE__
#define __MACOSX__
#include <Vorbis/vorbisfile.h>
#else
#include <vorbis/vorbisfile.h>
#endif

#include <fstream>
#include <cstdio>
#include <cstring>

SoundBuffer::SoundBuffer() :
	info(0, 0, 0, 0),
	loaded(false),
	sound_buffer(0)
{
	// ctor
}

SoundBuffer::~SoundBuffer()
{
	Unload();
}

bool SoundBuffer::Load(const std::string & filename, const SoundInfo & sound_device_info, std::ostream & error_output)
{
	if (filename.find(".wav") != std::string::npos)
		return LoadWAV(filename, sound_device_info, error_output);
	else if (filename.find(".ogg") != std::string::npos)
		return LoadOGG(filename, sound_device_info, error_output);
	else
	{
		error_output << "Unable to determine file type from filename: " << filename << std::endl;
		return false;
	}
}

void SoundBuffer::Unload()
{
	if (loaded && sound_buffer)
		delete [] sound_buffer;
	sound_buffer = 0;
}

bool SoundBuffer::LoadWAV(const std::string & filename, const SoundInfo & sound_device_info, std::ostream & error_output)
{
	if (loaded)
		Unload();

	name = filename;

	std::ifstream file(filename.c_str(), std::ifstream::binary);
	if (!file)
	{
		error_output << "Failed to open sound file: " << filename << std::endl;
		return false;
	}

	// read in first four bytes
	char id[5] = {'\0'};
	file.read(id, 4);
	if (!file)
	{
		error_output << "Failed to read sound file RIFF header: " << filename << std::endl;
		return false;
	}
	if (strcmp(id, "RIFF"))
	{
		error_output << "Sound file doesn't have RIFF header: " << filename << std::endl;
		return false;
	}

	// read in 32bit size value
	unsigned int size = 0;
	file.read((char*)&size, sizeof(unsigned int));
	size = ENDIAN_SWAP_32(size);
	if (!file)
	{
		error_output << "Failed to read sound file size: " << filename << std::endl;
		return false;
	}

	// read in 4 bytes "WAVE"
	file.read(id, 4);
	if (!file)
	{
		error_output << "Failed to read WAVE header: " << filename << std::endl;
		return false;
	}
	if (strcmp(id, "WAVE"))
	{
		error_output << "Sound file doesn't have WAVE header: " << filename << std::endl;
		return false;
	}

	// read in 4 bytes "fmt ";
	file.read(id, 4);
	if (!file)
	{
		error_output << "Failed to read \"fmt\" header: " << filename << std::endl;
		return false;
	}
	if (strcmp(id, "fmt "))
	{
		error_output << "Sound file doesn't have \"fmt\" header: " << filename << std::endl;
		return false;
	}

	// read sound format
	unsigned int format_length, sample_rate, avg_bytes_sec;
	short format_tag, channels, block_align, bits_per_sample;

	file.read((char*)&format_length, sizeof(unsigned int));
	file.read((char*)&format_tag, sizeof(short));
	file.read((char*)&channels, sizeof(short));
	file.read((char*)&sample_rate, sizeof(unsigned int));
	file.read((char*)&avg_bytes_sec, sizeof(unsigned int));
	file.read((char*)&block_align, sizeof(short));
	file.read((char*)&bits_per_sample, sizeof(short));

	if (!file)
	{
		error_output << "Failed to read sound format: " << filename << std::endl;
		return false;
	}

	format_length = ENDIAN_SWAP_32(format_length);
	format_tag = ENDIAN_SWAP_16(format_tag);
	channels = ENDIAN_SWAP_16(channels);
	sample_rate = ENDIAN_SWAP_32(sample_rate);
	avg_bytes_sec = ENDIAN_SWAP_32(avg_bytes_sec);
	block_align = ENDIAN_SWAP_16(block_align);
	bits_per_sample = ENDIAN_SWAP_16(bits_per_sample);

	// Currently we only supper 16 bit samples
	if (bits_per_sample != 16)
	{
		error_output << "Sound file with " << bits_per_sample << " bits per sample not supported" << std::endl;
		return false;
	}

	// find data chunk
	bool found_data_chunk = false;
	long filepos = format_length + 4 + 4 + 4 + 4 + 4;
	int chunknum = 0;
	while (!found_data_chunk && chunknum < 10)
	{
		// seek to the next chunk
		file.seekg(filepos);

		// read in 'data'
		file.read(id, 4);
		if (!file) return false;

		// how many bytes of sound data we have
		file.read((char*)&size, sizeof(unsigned int));
		if (!file) return false;

		size = ENDIAN_SWAP_32(size);

		if (!strcmp(id, "data"))
			found_data_chunk = true;
		else
			filepos += size + 4 + 4;

		chunknum++;
	}

	if (chunknum >= 10)
	{
		error_output << "Couldn't find wave data in first 10 chunks of " << filename << std::endl;
		return false;
	}

	// read in our whole sound data chunk
	sound_buffer = new char[size];
	file.read(sound_buffer, size);
	if (!file)
	{
		error_output << "Failed to read wave data " << filename << std::endl;
		return false;
	}

#if SDL_BYTEORDER == SDL_BIG_ENDIAN
	for (unsigned int i = 0; i < size / 2; i++)
	{
		((short *)sound_buffer)[i] = ENDIAN_SWAP_16(((short *)sound_buffer)[i]);
	}
#endif

	unsigned int bytespersample = bits_per_sample / 8;
	unsigned int samples = size / bytespersample;
	if (sound_device_info.bytespersample == 4)
	{
		// convert to float
		bytespersample = 4;
		float * sound_buffer_float = new float[samples];
		for (unsigned int i = 0; i < samples; i++)
		{
			sound_buffer_float[i] = ((short *)sound_buffer)[i] * (1.0f / 32767);
		}
		delete [] sound_buffer;
		sound_buffer = (char*)sound_buffer_float;
	}

	info = SoundInfo(samples, sample_rate, channels, bytespersample);
	loaded = true;
	return true;
}

bool SoundBuffer::LoadOGG(const std::string & filename, const SoundInfo & sound_device_info, std::ostream & error_output)
{
	if (loaded)
		Unload();

	name = filename;

	FILE * fp = fopen(filename.c_str(), "rb");
	if (!fp)
	{
		error_output << "Can't open sound file: " + filename << std::endl;
		return false;
	}

	int bytespersample = sound_device_info.bytespersample;
	if (bytespersample != 2 && bytespersample != 4)
	{
		error_output << "Sound buffer with " << bytespersample << " bytes per sample not supported" << std::endl;
		return false;
	}

	OggVorbis_File oggFile;
	ov_open_callbacks(fp, &oggFile, NULL, 0, OV_CALLBACKS_DEFAULT);

	vorbis_info * pInfo = ov_info(&oggFile, -1);
	unsigned int samples = ov_pcm_total(&oggFile, -1);
	info = SoundInfo(samples * pInfo->channels, pInfo->rate, pInfo->channels, bytespersample);

	// allocate space
	unsigned int size = info.samples * info.bytespersample;
	sound_buffer = new char[size];

	if (bytespersample == 2)
	{
		int bitstream;
		int endian = 0; // 0 for Little-Endian, 1 for Big-Endian
		int wordsize = 2; // 16 bit
		int issigned = 1; // signed data
		unsigned int bufpos = 0; // total bytes read
		while (bufpos < size)
		{
			int bytes_read = ov_read(&oggFile, sound_buffer + bufpos, size - bufpos, endian, wordsize, issigned, &bitstream);
			if (bytes_read <= 0)
			{
				error_output << "Error decoding " + filename << std::endl;
				delete [] sound_buffer;
				ov_clear(&oggFile);
				return false;
			}
			bufpos += bytes_read;
		}
	}
	else
	{
		float * buffer = (float*)sound_buffer;
		int bitstream;
		unsigned int bufpos = 0; // total samples read
		while (bufpos < samples)
		{
			float **pcm;
			int samples_read = ov_read_float(&oggFile, &pcm, samples - bufpos, &bitstream);
			if (samples_read <= 0)
			{
				error_output << "Error decoding " + filename << std::endl;
				delete [] sound_buffer;
				ov_clear(&oggFile);
				return false;
			}

			if (info.channels > 1)
			{
				// interleaving left and right channels
				for (int i = 0; i < samples_read; i++)
				{
					unsigned int n = (bufpos + i) * 2;
					buffer[n] = pcm[0][i];
					buffer[n + 1] = pcm[1][i];
				}
			}
			else
			{
				memcpy(buffer + bufpos, pcm[0], samples_read * sizeof(float));
			}

			bufpos += samples_read;
		}
	}

	// note: no need to call fclose(); ov_clear does it for us
	ov_clear(&oggFile);

	loaded = true;
	return true;
}
