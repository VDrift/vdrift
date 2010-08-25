#include "soundbuffer.h"

#include <fstream>
#include <cstdio>
#include <cstring>

#include "endian_utility.h"
#ifdef __APPLE__
#include <Vorbis/vorbisfile.h>
#else
#include <vorbis/vorbisfile.h>
#endif

SOUNDBUFFER::SOUNDBUFFER() : sound_buffer(NULL)
{
	// ctor
}

SOUNDBUFFER::~SOUNDBUFFER()
{
	Unload();
}

class SOUNDFILE
{
public:
	virtual bool open(const std::string & filename, SOUNDINFO & info, std::ostream & error_output) = 0;
	virtual bool read(char & sound_buffer, unsigned int size) = 0;
};

class OGGFILE : public SOUNDFILE
{
public:
	virtual bool open(const std::string & filename, SOUNDINFO & info, std::ostream & error_output)
	{
		FILE * fp = fopen(filename.c_str(), "rb");
		if (!fp)
		{
			error_output << "Can't open sound file: " << filename << std::endl;
			return false;
		}
		
		ov_open_callbacks(fp, &oggFile, NULL, 0, OV_CALLBACKS_DEFAULT);
		vorbis_info * pInfo = ov_info(&oggFile, -1);
		unsigned int samples = ov_pcm_total(&oggFile, -1);
		
		info.samples = samples * pInfo->channels;
		info.frequency = pInfo->rate;
		info.channels = pInfo->channels;
		info.bytespersample = 2;	// assume ogg is always 16-bit (2 bytes per sample) -Venzon
		
		return true;
	}
	
	virtual bool read(char & sound_buffer, unsigned int size)
	{
		int bitstream;
		int endian = 0; //0 for Little-Endian, 1 for Big-Endian
		int wordsize = 2; //again, assuming ogg is always 16-bits
		int issigned = 1; //use signed data
		int bytes = 1;
		unsigned int bufpos = 0;
		
		while (bytes > 0)
		{
			bytes = ov_read(&oggFile, &sound_buffer+bufpos, size-bufpos, endian, wordsize, issigned, &bitstream);
			bufpos += bytes;
		}
		
		return true;
	}
	
	OGGFILE()
	{
		oggFile.datasource = NULL;
	}
	
	~OGGFILE()
	{
		if (oggFile.datasource != NULL) 
		{
			ov_clear(&oggFile);
		}
	}
	
private:
	OggVorbis_File oggFile;
};

class WAVFILE : public SOUNDFILE
{
public:
	virtual bool open(const std::string & filename, SOUNDINFO & info, std::ostream & error_output)
	{
		fp = fopen(filename.c_str(), "rb");
		if (!fp) 
		{
			error_output << "Can't open sound file: " << filename << std::endl;
			return false;
		}
		
		// read riff header
		char id[5];
		id[4] = '\0';
		if (fread(id, sizeof(char), 4, fp) != 4) return false;
		if (strcmp(id, "RIFF"))
		{
			error_output << "Sound file doesn't have RIFF header: " << filename << std::endl;
			return false;
		}
		
		// read size
		unsigned int size;
		if (fread(&size, sizeof(unsigned int), 1, fp) != 1) return false;
		size = ENDIAN_SWAP_32(size);
		
		// read wave header
		if (fread(id, sizeof(char), 4, fp) != 4) return false;
		if (strcmp(id, "WAVE"))	
		{
			error_output << "Sound file doesn't have WAVE header: " << filename << std::endl;
			return false;
		}
		
		// read fmt header
		if (fread(id, sizeof(char), 4, fp) != 4) return false;
		if (strcmp(id, "fmt "))
		{
			error_output << "Sound file doesn't have \"fmt\" header: " << filename << std::endl;
			return false;
		}
		
		unsigned int format_length(0);
		unsigned int sample_rate(0);
		unsigned int avg_bytes_sec(0);
		short format_tag(0);
		short channels(0);
		short block_align(0);
		short bits_per_sample(0);
		
		if (fread(&format_length, sizeof(unsigned int), 1, fp) != 1) return false;
		format_length = ENDIAN_SWAP_32(format_length);
		
		if (fread(&format_tag, sizeof(short), 1, fp) != 1) return false;
		format_tag = ENDIAN_SWAP_16(format_tag);
		
		if (fread(&channels, sizeof(short), 1, fp) != 1) return false;
		channels = ENDIAN_SWAP_16(channels);
		
		if (fread(&sample_rate, sizeof(unsigned int), 1, fp) != 1) return false;
		sample_rate = ENDIAN_SWAP_32(sample_rate);
		
		if (fread(&avg_bytes_sec, sizeof(unsigned int), 1, fp) != 1) return false;
		avg_bytes_sec = ENDIAN_SWAP_32(avg_bytes_sec);
		
		if (fread(&block_align, sizeof(short), 1, fp) != 1) return false;
		block_align = ENDIAN_SWAP_16(block_align);
		
		if (fread(&bits_per_sample, sizeof(short), 1, fp) != 1) return false;
		bits_per_sample = ENDIAN_SWAP_16(bits_per_sample);
		
		// find data chunk
		bool found_data_chunk = false;
		long filepos = format_length + 4 + 4 + 4 + 4 + 4;
		int chunknum = 0;
		while (!found_data_chunk && chunknum < 10)
		{
			// seek to the next chunk
			fseek(fp, filepos, SEEK_SET);
			
			// read in 'data'
			if (fread(id, sizeof(char), 4, fp) != 4) return false; 
			
			// how many bytes of sound data we have
			if (fread(&size, sizeof(unsigned int), 1, fp) != 1) return false; 
			size = ENDIAN_SWAP_32(size);
			
			if (!strcmp(id, "data"))
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
			error_output << "Couldn't find wave data in first 10 chunks of " << filename << std::endl;
			return false;
		}
		
		info.channels = channels;
		info.bytespersample = bits_per_sample / 8;
		info.frequency = sample_rate;
		info.samples = size / (info.bytespersample * info.channels);
		
		return true;
	}
	
	virtual bool read(char & sound_buffer, unsigned int size)
	{
		if (fread(&sound_buffer, sizeof(char), size, fp) != size)
		{
			return false;
		}
		
		#if SDL_BYTEORDER == SDL_BIG_ENDIAN
		for (unsigned int i = 0; i < size / 2; i++)
		{
			((short *)&sound_buffer)[i] = ENDIAN_SWAP_16(((short *)&sound_buffer)[i]);
		}
		#endif
		
		return true;
	}
	
	WAVFILE()
	{
		fp = NULL;
	}
	
	~WAVFILE()
	{
		if (fp != NULL) fclose(fp);
	}
	
private:
	FILE * fp;
};

bool SOUNDBUFFER::Load(const std::string & filename, const SOUNDINFO & device_info, std::ostream & error_output)
{
	name = filename;
	
	SOUNDFILE * soundfile = NULL;
	WAVFILE wavfile;
	OGGFILE oggfile;
	
	if (filename.find(".wav") != std::string::npos)
	{
		soundfile = &wavfile;
	}
	else if (filename.find(".ogg") != std::string::npos)
	{
		soundfile = &oggfile;
	}
	else
	{
		error_output << "Unable to determine file type from filename: " << filename << std::endl;
	}
	
	if(!soundfile->open(filename, info, error_output))
	{
		return false;
	}
	
	SOUNDINFO desired_info;
	desired_info.samples = info.samples;
	desired_info.frequency = device_info.frequency;
	desired_info.channels = info.channels;
	desired_info.bytespersample = device_info.bytespersample;
	if (desired_info != info)
	{
		error_output << "SOUND FORMAT:" << std::endl;
		info.DebugPrint(error_output);
		error_output << "DESIRED FORMAT:" << std::endl;
		desired_info.DebugPrint(error_output);
		error_output << "Sound file isn't in desired format: " << filename << std::endl;
		return false;
	}
	
	unsigned int size = info.samples * info.channels * info.bytespersample;
	sound_buffer = new char[size];
	
	return soundfile->read(*sound_buffer, size);
}

void SOUNDBUFFER::Unload()
{
	if (sound_buffer != NULL)
	{
		delete [] sound_buffer;
		sound_buffer = NULL;
	}
}
