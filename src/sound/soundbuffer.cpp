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
	size(0),
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

	FILE *fp;

	unsigned int size;

	fp = fopen(filename.c_str(), "rb");
	if (fp)
	{
		char id[5]; //four bytes to hold 'RIFF'

		if (fread(id,sizeof(char),4,fp) != 4) return false; //read in first four bytes
		id[4] = '\0';
		if (!strcmp(id,"RIFF"))
		{ //we had 'RIFF' let's continue
			if (fread(&size,sizeof(unsigned int),1,fp) != 1) return false; //read in 32bit size value
			size = ENDIAN_SWAP_32(size);
			if (fread(id,sizeof(char),4,fp)!= 4) return false; //read in 4 byte string now
			if (!strcmp(id,"WAVE"))
			{ //this is probably a wave file since it contained "WAVE"
				if (fread(id,sizeof(char),4,fp)!= 4) return false; //read in 4 bytes "fmt ";
				if (!strcmp(id,"fmt "))
				{
					unsigned int format_length, sample_rate, avg_bytes_sec;
					short format_tag, channels, block_align, bits_per_sample;

					if (fread(&format_length, sizeof(unsigned int),1,fp) != 1) return false;
					format_length = ENDIAN_SWAP_32(format_length);
					if (fread(&format_tag, sizeof(short), 1, fp) != 1) return false;
					format_tag = ENDIAN_SWAP_16(format_tag);
					if (fread(&channels, sizeof(short),1,fp) != 1) return false;
					channels = ENDIAN_SWAP_16(channels);
					if (fread(&sample_rate, sizeof(unsigned int), 1, fp) != 1) return false;
					sample_rate = ENDIAN_SWAP_32(sample_rate);
					if (fread(&avg_bytes_sec, sizeof(unsigned int), 1, fp) != 1) return false;
					avg_bytes_sec = ENDIAN_SWAP_32(avg_bytes_sec);
					if (fread(&block_align, sizeof(short), 1, fp) != 1) return false;
					block_align = ENDIAN_SWAP_16(block_align);
					if (fread(&bits_per_sample, sizeof(short), 1, fp) != 1) return false;
					bits_per_sample = ENDIAN_SWAP_16(bits_per_sample);


					//new wave seeking code
					//find data chunk
					bool found_data_chunk = false;
					long filepos = format_length + 4 + 4 + 4 + 4 + 4;
					int chunknum = 0;
					while (!found_data_chunk && chunknum < 10)
					{
						fseek(fp, filepos, SEEK_SET); //seek to the next chunk
						if (fread(id, sizeof(char), 4, fp) != 4) return false; //read in 'data'
						if (fread(&size, sizeof(unsigned int), 1, fp) != 1) return false; //how many bytes of sound data we have
						size = ENDIAN_SWAP_32(size);
						if (!strcmp(id,"data"))
						{
							found_data_chunk = true;
						}
						else
						{
							filepos += size + 4 + 4;
						}

						chunknum++;
					}

					if (chunknum >= 10)
					{
						//cerr << __FILE__ << "," << __LINE__ << ": Sound file contains more than 10 chunks before the data chunk: " + filename << std::endl;
						error_output << "Couldn't find wave data in first 10 chunks of " << filename << std::endl;
						return false;
					}

					sound_buffer = new char[size];

					if (fread(sound_buffer, sizeof(char), size, fp) != size) return false; //read in our whole sound data chunk

#if SDL_BYTEORDER == SDL_BIG_ENDIAN
					if (bits_per_sample == 16)
					{
						for (unsigned int i = 0; i < size/2; i++)
						{
							//cout << "preswap i: " << sound_buffer[i] << "preswap i+1: " << sound_buffer[i+1] << std::endl;
							//short preswap = ((short *)sound_buffer)[i];
							((short *)sound_buffer)[i] = ENDIAN_SWAP_16(((short *)sound_buffer)[i]);
							//cout << "postswap i: " << sound_buffer[i] << "postswap i+1: " << sound_buffer[i+1] << std::endl;
							//cout << (int) i << "/" << (int) size << std::endl;
							//short postswap = ((short *)sound_buffer)[i];
							//cout << preswap << "/" << postswap << std::endl;

						}
					}
					//else if (bits_per_sample != 8)
					else
					{
						error_output << "Sound file with " << bits_per_sample << " bits per sample not supported" << std::endl;
						return false;
					}
#endif

					info = SoundInfo(size/(bits_per_sample/8), sample_rate, channels, bits_per_sample/8);
					SoundInfo original_info(size/(bits_per_sample/8), sample_rate, channels, bits_per_sample/8);

					loaded = true;
					SoundInfo desired_info(original_info.samples, sound_device_info.frequency, original_info.channels, sound_device_info.bytespersample);
					//ConvertTo(desired_info);
					if (desired_info == original_info)
					{

					}
					else
					{
						error_output << "SOUND FORMAT:" << std::endl;
						original_info.DebugPrint(error_output);
						error_output << "DESIRED FORMAT:" << std::endl;
						desired_info.DebugPrint(error_output);
						//throw EXCEPTION(__FILE__, __LINE__, "Sound file isn't in desired format: " + filename);
						//cerr << __FILE__ << "," << __LINE__ << ": Sound file isn't in desired format: " + filename << std::endl;
						error_output << "Sound file isn't in desired format: "+filename << std::endl;
						return false;
					}
				}
				else
				{
					//throw EXCEPTION(__FILE__, __LINE__, "Sound file doesn't have \"fmt \" header: " + filename);
					//cerr << __FILE__ << "," << __LINE__ << ": Sound file doesn't have \"fmt \" header: " + filename << std::endl;
					error_output << "Sound file doesn't have \"fmt\" header: "+filename << std::endl;
					return false;
				}
			}
			else
			{
				//throw EXCEPTION(__FILE__, __LINE__, "Sound file doesn't have WAVE header: " + filename);
				//cerr << __FILE__ << "," << __LINE__ << ": Sound file doesn't have WAVE header: " + filename << std::endl;
				error_output << "Sound file doesn't have WAVE header: "+filename << std::endl;
				return false;
			}
		}
		else
		{
			//throw EXCEPTION(__FILE__, __LINE__, "Sound file doesn't have RIFF header: " + filename);
			//cerr << __FILE__ << "," << __LINE__ << ": Sound file doesn't have WAVE header: " + filename << std::endl;
			error_output << "Sound file doesn't have RIFF header: "+filename << std::endl;
			return false;
		}
		fclose(fp);
	}
	else
	{
		//throw EXCEPTION(__FILE__, __LINE__, "Can't open sound file: " + filename);
		//cerr << __FILE__ << "," << __LINE__ << ": Can't open sound file: " + filename << std::endl;
		error_output << "Can't open sound file: "+filename << std::endl;
		return false;
	}

	//cout << size << std::endl;
	return true;
}

bool SoundBuffer::LoadOGG(const std::string & filename, const SoundInfo & sound_device_info, std::ostream & error_output)
{
	if (loaded)
		Unload();

	name = filename;

	FILE *fp;

	unsigned int samples;

	fp = fopen(filename.c_str(), "rb");
	if (fp)
	{
		vorbis_info *pInfo;
		OggVorbis_File oggFile;

		ov_open_callbacks(fp, &oggFile, NULL, 0, OV_CALLBACKS_DEFAULT);

		pInfo = ov_info(&oggFile, -1);

		//I assume ogg is always 16-bit (2 bytes per sample) -Venzon
		samples = ov_pcm_total(&oggFile,-1);
		info = SoundInfo(samples*pInfo->channels, pInfo->rate, pInfo->channels, 2);

		SoundInfo desired_info(info.samples, sound_device_info.frequency, info.channels, sound_device_info.bytespersample);

		if (!(desired_info == info))
		{
			error_output << "SOUND FORMAT:" << std::endl;
			info.DebugPrint(error_output);
			error_output << "DESIRED FORMAT:" << std::endl;
			desired_info.DebugPrint(error_output);

			error_output << "Sound file isn't in desired format: "+filename << std::endl;
			ov_clear(&oggFile);
			return false;
		}

		//allocate space
		unsigned int size = info.samples*info.channels*info.bytespersample;
		sound_buffer = new char[size];
		int bitstream;
		int endian = 0; //0 for Little-Endian, 1 for Big-Endian
		int wordsize = 2; //again, assuming ogg is always 16-bits
		int issigned = 1; //use signed data

		int bytes = 1;
		unsigned int bufpos = 0;
		while (bytes > 0)
		{
			bytes = ov_read(&oggFile, sound_buffer+bufpos, size-bufpos, endian, wordsize, issigned, &bitstream);
			bufpos += bytes;
			//cout << bytes << "...";
		}

		loaded = true;

		//note: no need to call fclose(); ov_clear does it for us
		ov_clear(&oggFile);

		return true;
	}
	else
	{
		error_output << "Can't open sound file: "+filename << std::endl;
		return false;
	}
}
