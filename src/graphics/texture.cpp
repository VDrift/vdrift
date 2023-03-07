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

#include "texture.h"
#include "glcore.h"
#include "glutil.h"
#include "bcndecode.h"
#include "dds.h"
#include "png.h"

#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include <cassert>

// texformat[bytespp - 1]
static const int texformat[4] =
{
	GL_RED, GL_RG, GL_RGB, GL_RGBA
};

// itexformat[compressed][srgb][bytespp-1]
static const int itexformat[2][2][4] =
{
	{{GL_RED, GL_RG, GL_RGB, GL_RGBA},
	 {GL_RED, GL_RG, GL_SRGB8, GL_SRGB8_ALPHA8}},
	{{GL_COMPRESSED_RED, GL_COMPRESSED_RG, GL_COMPRESSED_RGB, GL_COMPRESSED_RGBA},
	 {GL_COMPRESSED_RED, GL_COMPRESSED_RG, GL_COMPRESSED_SRGB, GL_COMPRESSED_SRGB_ALPHA}}
};

// averaging downsampler
// bytespp is the size of a pixel (number of channels)
// dst width/height are multiples of src width, height in pixels
// pitch is size of a pixel row in bytes
template <unsigned bytespp>
static void SampleDownAvg(
	const unsigned src_width,
	const unsigned src_height,
	const unsigned src_pitch,
	const unsigned char src[],
	const unsigned dst_width,
	const unsigned dst_height,
	const unsigned dst_pitch,
	unsigned char dst[])
{
	const unsigned scalex = src_width / dst_width;
	const unsigned scaley = src_height / dst_height;
	const unsigned div = scalex * scaley;
	assert(scalex * dst_width == src_width);
	assert(scaley * dst_height == src_height);

	unsigned acc[bytespp];
	for (unsigned y = 0; y < dst_height; ++y)
	{
		unsigned char * dp = dst + y * dst_pitch;
		const unsigned char * spy = src + y * src_pitch * scaley;
		for (unsigned x = 0; x < dst_width; ++x)
		{
			const unsigned char * sp = spy + x * scalex * bytespp;
			for (unsigned i = 0; i < bytespp; ++i)
				acc[i] = 0;
			for (unsigned dy = 0; dy < scaley; ++dy)
			{
				for (unsigned dx = 0; dx < scalex; ++dx)
				{
					for (unsigned i = 0; i < bytespp; ++i, ++sp)
						acc[i] += *sp;
				}
				sp += (src_pitch - scalex * bytespp);
			}
			for (unsigned i = 0; i < bytespp; ++i, ++dp)
				*dp = acc[i] / div;
		}
	}
}

static void SampleDownAvg(
	const unsigned bytespp,
	const unsigned src_width,
	const unsigned src_height,
	const unsigned src_pitch,
	const unsigned char src[],
	const unsigned dst_width,
	const unsigned dst_height,
	const unsigned dst_pitch,
	unsigned char dst[])
{
	if (bytespp == 1)
	{
		SampleDownAvg<1>(
			src_width, src_height, src_pitch, src,
			dst_width, dst_height, dst_pitch, dst);
	}
	else if (bytespp == 2)
	{
		SampleDownAvg<2>(
			src_width, src_height, src_pitch, src,
			dst_width, dst_height, dst_pitch, dst);
	}
	else if (bytespp == 3)
	{
		SampleDownAvg<3>(
			src_width, src_height, src_pitch, src,
			dst_width, dst_height, dst_pitch, dst);
	}
	else if (bytespp == 4)
	{
		SampleDownAvg<4>(
			src_width, src_height, src_pitch, src,
			dst_width, dst_height, dst_pitch, dst);
	}
	else
	{
		assert(0);
	}
}

inline unsigned char* DownSample(char size, unsigned bytespp, unsigned & w, unsigned & h,
								unsigned char * pixels, std::vector<unsigned char> & pixelsd)
{
	unsigned wd = w;
	unsigned hd = h;
	if (size == TextureInfo::SMALL)
	{
		if (w > 256)
		{
			if (!(w & 3)) wd = w / 4;
		}
		else if (w > 128)
		{
			if (!(w & 1)) wd = w / 2;
		}
		if (h > 256)
		{
			if (!(h & 3)) hd = h / 4;
		}
		else if (h > 128)
		{
			if (!(h & 1)) hd = h / 2;
		}
	}
	else if (size == TextureInfo::MEDIUM)
	{
		if (w > 256)
		{
			if (!(w & 1)) wd = w / 2;
		}
		if (h > 256)
		{
			if (!(h & 1)) hd = h / 2;
		}
	}
	if (wd < w || hd < h)
	{
		pixelsd.resize(wd * hd * bytespp);

		SampleDownAvg(
			bytespp, w, h, w * bytespp, pixels,
			wd, hd, wd * bytespp, pixelsd.data());

		w = wd;
		h = hd;
		return pixelsd.data();
	}
	return pixels;
}

static void SetSampler(const TextureInfo & info, bool hasmiplevels = false)
{
	unsigned int target = info.cube ? GL_TEXTURE_CUBE_MAP : GL_TEXTURE_2D;

	unsigned int wraps = info.repeatu ? GL_REPEAT : GL_CLAMP_TO_EDGE;
	unsigned int wrapt = info.repeatv ? GL_REPEAT : GL_CLAMP_TO_EDGE;

	unsigned int magfilter = info.nearest ? GL_NEAREST : GL_LINEAR;
	unsigned int minfilter = info.nearest ?
		(info.mipmap ? GL_NEAREST_MIPMAP_NEAREST : GL_NEAREST) :
		(info.mipmap ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);

	glTexParameteri(target, GL_TEXTURE_WRAP_S, wraps);
	glTexParameteri(target, GL_TEXTURE_WRAP_T, wrapt);
	glTexParameteri(target, GL_TEXTURE_WRAP_R, wrapt); // set to the same value as v here

	glTexParameteri(target, GL_TEXTURE_MIN_FILTER, minfilter);
	glTexParameteri(target, GL_TEXTURE_MAG_FILTER, magfilter);

	// automatic mipmap generation fallback
	if (!hasmiplevels && info.mipmap && !GLC_ARB_framebuffer_object)
		glTexParameteri(target, GL_GENERATE_MIPMAP, GL_TRUE);

	if (info.anisotropy > 1)
		glTexParameterf(target, GL_TEXTURE_MAX_ANISOTROPY_EXT, (float)info.anisotropy);
}

Texture::Texture()
{
	// ctor
}

Texture::~Texture()
{
	Unload();
}

bool Texture::Load(const TextureData & data, const TextureInfo & info, std::ostream & error)
{
	if (data.data == 0 || data.width == 0 || data.height == 0)
	{
		error << "Invalid texture data: " << data.data << " " << data.width << " x " << data.height << std::endl;
		return false;
	}

	if (data.bytespp < 1 || data.bytespp > 4)
	{
		error << "Unsupported bytes per pixel: " << data.bytespp << std::endl;
		return false;
	}

	unsigned char * pixels;
	std::vector<unsigned char> buffer;
	if (info.cube)
	{
		const unsigned htiles = 6;
		width = data.width;
		height = data.height / htiles;
		pixels = data.data;
		target = GL_TEXTURE_CUBE_MAP;
		if (height * htiles != (unsigned)data.height)
		{
			error << "Cube map image height not divisible by 6: " << data.height << std::endl;
			return false;
		}
	}
	else
	{
		width = data.width;
		height = data.height;
		pixels = DownSample(info.maxsize, data.bytespp, width, height, data.data, buffer);
		target = GL_TEXTURE_2D;
	}

	// gen texture
	assert(!texid);
	glGenTextures(1, &texid);
	CheckForOpenGLErrors("Texture ID generation", error);

	// setup texture
	glBindTexture(target, texid);
	SetSampler(info);

	// get texture format
	bool compress = info.compress && (width > 512 || height > 512);
	int format = texformat[data.bytespp - 1];
	int iformat = itexformat[compress][info.srgb][data.bytespp - 1];

	// upload texture data
	if (info.cube)
	{
		const unsigned itarget = GL_TEXTURE_CUBE_MAP_POSITIVE_X;
		const unsigned ilen = width * height * data.bytespp;
		for (int i = 0; i < 6; ++i)
		{
			const char * idata = (const char *)pixels + ilen * i;
			glTexImage2D(itarget + i, 0, iformat, width, height, 0, format, GL_UNSIGNED_BYTE, idata);
		}
	}
	else
	{
		glTexImage2D(target, 0, iformat, width, height, 0, format, GL_UNSIGNED_BYTE, pixels);
	}

	CheckForOpenGLErrors("Texture creation", error);

	// If we support generatemipmap, go ahead and do it regardless of the info.mipmap setting.
	// In the GL3 renderer the sampler decides whether or not to do mip filtering,
	// so we conservatively make mipmaps available for all textures.
	if (GLC_ARB_framebuffer_object)
		glGenerateMipmap(target);

	return true;
}

bool Texture::Load(const std::string & path, const TextureInfo & info, std::ostream & error)
{
	if (path.empty())
	{
		error << "Tried to load a texture with an empty name" << std::endl;
		return false;
	}

	if (LoadDDS(path, info, error))
	{
		return true;
	}

	// load image
	std::vector<unsigned char> img;
	TextureData data;
	unsigned pret = LoadPNG(path.c_str(), img, data.width, data.height, data.bytespp);
	if (pret)
	{
		error << "Error loading texture file: " << path << "\nLoadPNG: " << LoadPNGError(pret) << std::endl;
		return false;
	}
	data.data = img.data();

	// load texture
	return Load(data, info, error);
}

void Texture::Unload()
{
	if (texid)
		glDeleteTextures(1, &texid);
	texid = 0;
}

bool Texture::LoadDDS(const std::string & path, const TextureInfo & info, std::ostream & error)
{
	std::ifstream file(path.c_str(), std::ifstream::in | std::ifstream::binary);
	if (!file)
		return false;

	// test for dds magic value
	char magic[4];
	file.read(magic, 4);
	if (!IsDDS(magic, 4))
		return false;

	// get length of file:
	file.seekg (0, file.end);
	const unsigned long length = file.tellg();
	file.seekg (0, file.beg);

	// read file into memory
	std::vector<char> data(length);
	file.read(data.data(), length);

	// load dds
	const char * texdata(0);
	unsigned long texlen(0);
	unsigned format(0);
	unsigned levels(0);
	if (!ReadDDS(
		(void*)data.data(), length,
		(const void*&)texdata, texlen,
		format, target,
		width, height, levels))
	{
		error << "Failed ReadDDS " << path << std::endl;
		return false;
	}

	// load texture
	assert(!texid);
	glGenTextures(1, &texid);
	CheckForOpenGLErrors("Texture ID generation", error);

	glBindTexture(target, texid);
	SetSampler(info, levels > 1);

	// gl3 renderer expects srgb
	unsigned iformat = format;
	if (info.srgb)
	{
		if (format == GL_BGR)
			iformat = GL_SRGB8;
		else if (format == GL_BGRA)
			iformat = GL_SRGB8_ALPHA8;
		else if (format == GL_COMPRESSED_RGBA_S3TC_DXT1_EXT)
			iformat = GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT;
		else if (format == GL_COMPRESSED_RGBA_S3TC_DXT3_EXT)
			iformat = GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT;
		else if (format == GL_COMPRESSED_RGBA_S3TC_DXT5_EXT)
			iformat = GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT;
	}

	// handle the case s3tc is not supported
	std::vector<char> cdata;
	unsigned cformat = info.srgb ? GL_SRGB8_ALPHA8 : GL_RGBA;
	unsigned ctype = 0;
	switch (format) {
		case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT: ctype = 1; break;
		case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT: ctype = 2; break;
		case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT: ctype = 3; break;
	}

	unsigned faces = 1;
	unsigned itarget = target;
	if (target == GL_TEXTURE_CUBE_MAP)
	{
		faces = 6;
		itarget = GL_TEXTURE_CUBE_MAP_POSITIVE_X;
	}
	const char * idata = texdata;
	const unsigned blocklen = 16 * texlen / (width * height);
	for (unsigned j = 0; j < faces; ++j)
	{
		unsigned iw = width;
		unsigned ih = height;
		for (unsigned i = 0; i < levels; ++i)
		{
			unsigned ilen;
			if (format == GL_BGR || format == GL_BGRA)
			{
				ilen = iw * ih * blocklen / 16;
				glTexImage2D(itarget, i, iformat, iw, ih, 0, format, GL_UNSIGNED_BYTE, idata);
			}
			else
			{
				ilen = std::max(1u, iw / 4) * std::max(1u, ih / 4) * blocklen;
				if (GLC_EXT_texture_compression_s3tc)
				{
					glCompressedTexImage2D(itarget, i, iformat, iw, ih, 0, ilen, idata);
				}
				else
				{
					cdata.resize(iw * ih * 4);
					if (BcnDecode(cdata.data(), cdata.size(), idata, ilen, iw, ih, ctype, 0, 0) < 0)
					{
						error << "Failed BcnDecode " << path << std::endl;
						glBindTexture(target, 0);
						Unload();
						return false;
					}
					glTexImage2D(itarget, i, cformat, iw, ih, 0, cformat, GL_UNSIGNED_BYTE, cdata.data());
				}
			}
			CheckForOpenGLErrors("Texture creation", error);

			idata += ilen;
			iw = std::max(1u, iw / 2);
			ih = std::max(1u, ih / 2);
		}
		itarget++;
	}

	// force mipmaps for GL3
	if (levels == 1 && GLC_ARB_framebuffer_object)
		glGenerateMipmap(target);

	return true;
}
