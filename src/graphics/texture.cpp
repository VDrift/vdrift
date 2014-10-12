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
#include "dds.h"

#ifdef __APPLE__
#include <SDL2_image/SDL_image.h>
#else
#include <SDL2/SDL_image.h>
#endif

#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include <cassert>

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

static void GetTextureFormat(
	const SDL_Surface * surface,
	const TextureInfo & info,
	int & internalformat,
	int & format)
{
	bool compress = info.compress && (surface->w > 512 || surface->h > 512);
	bool srgb = info.srgb;

	internalformat = compress ? (srgb ? GL_COMPRESSED_SRGB : GL_COMPRESSED_RGB) : (srgb ? GL_SRGB8 : GL_RGB);
	switch (surface->format->BytesPerPixel)
	{
		case 1:
			internalformat = compress ? GL_COMPRESSED_RED : GL_RED;
			format = GL_RED;
			break;
		case 2:
			internalformat = compress ? GL_COMPRESSED_RG : GL_RG;
			format = GL_RG;
			break;
		case 3:
			internalformat = compress ? (srgb ? GL_COMPRESSED_SRGB : GL_COMPRESSED_RGB) : (srgb ? GL_SRGB8 : GL_RGB);
#ifdef __APPLE__
			format = GL_BGR;
#else
			format = GL_RGB;
#endif
			break;
		case 4:
			internalformat = compress ? (srgb ? GL_COMPRESSED_SRGB_ALPHA : GL_COMPRESSED_RGBA) : (srgb ? GL_SRGB8_ALPHA8 : GL_RGBA);
#ifdef __APPLE__
			format = GL_BGRA;
#else
			format = GL_RGBA;
#endif
			break;
		default:
#ifdef __APPLE__
			format = GL_BGR;
#else
			format = GL_RGB;
#endif
			break;
	}
}

static void SetSampler(const TextureInfo & info, bool hasmiplevels = false)
{
	if (info.repeatu)
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	else
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);

	if (info.repeatv)
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	else
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	if (info.mipmap)
	{
		if (info.nearest)
		{
			glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST_MIPMAP_NEAREST);
			glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
		}
		else
		{
			glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_LINEAR);
			glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
		}
		// automatic mipmap generation fallback
		if (!hasmiplevels && info.mipmap && !GLC_ARB_framebuffer_object)
			glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE);
	}
	else
	{
		if (info.nearest)
		{
			glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
		}
		else
		{
			glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
		}
	}

	if (info.anisotropy > 1)
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, (float)info.anisotropy);
}

Texture::Texture()
{
	// ctor
}

Texture::~Texture()
{
	Unload();
}

bool Texture::Load(const std::string & path, const TextureInfo & info, std::ostream & error)
{

	if (texid)
	{
		error << "Tried to double load texture " << path << std::endl;
		return false;
	}

	if (!info.data && path.empty())
	{
		error << "Tried to load a texture with an empty name" << std::endl;
		return false;
	}

	if (!info.data && LoadDDS(path, info, error))
	{
		return true;
	}

	if (info.cube)
	{
		return LoadCube(path, info, error);
	}

	SDL_Surface * surface = 0;
	if (info.data)
	{
		Uint32 rmask, gmask, bmask, amask;
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
		rmask = 0xff000000;
		gmask = 0x00ff0000;
		bmask = 0x0000ff00;
		amask = 0x000000ff;
#else
		rmask = 0x000000ff;
		gmask = 0x0000ff00;
		bmask = 0x00ff0000;
		amask = 0xff000000;
#endif
		surface = SDL_CreateRGBSurfaceFrom(
			info.data, info.width, info.height,
			info.bytespp * 8, info.width * info.bytespp,
			rmask, gmask, bmask, amask);
	}
	else
	{
		surface = IMG_Load(path.c_str());
	}

	if (!surface)
	{
		error << "Error loading texture file: " << path << std::endl;
		error << IMG_GetError() << std::endl;
		return false;
	}

	const unsigned char * pixels = (const unsigned char *)surface->pixels;
	const unsigned bytespp = surface->format->BytesPerPixel;
	unsigned pitch = surface->pitch;
	unsigned w = surface->w;
	unsigned h = surface->h;

	// downsample if requested by application
	std::vector<unsigned char> pixelsd;
	unsigned wd = w;
	unsigned hd = h;
	if (info.maxsize == TextureInfo::SMALL)
	{
		if (w > 256)
			wd = w / 4;
		else if (w > 128)
			wd = w / 2;

		if (h > 256)
			hd = h / 4;
		else if (h > 128)
			hd = h / 2;
	}
	else if (info.maxsize == TextureInfo::MEDIUM)
	{
		if (w > 256)
			wd = w / 2;

		if (h > 256)
			hd = h / 2;
	}
	if (wd < w || hd < h)
	{
		pixelsd.resize(wd * hd * bytespp);

		SampleDownAvg(
			bytespp, w, h, pitch, pixels,
			wd, hd, wd * bytespp, &pixelsd[0]);

		pixels = &pixelsd[0];
		pitch = wd * bytespp;
		w = wd;
		h = hd;
	}

	// store dimensions
	width = w;
	height = h;

	target = GL_TEXTURE_2D;

	// gen texture
	glGenTextures(1, &texid);
	CheckForOpenGLErrors("Texture ID generation", error);

	// setup texture
	glBindTexture(GL_TEXTURE_2D, texid);
	SetSampler(info);

	int internalformat, format;
	GetTextureFormat(surface, info, internalformat, format);

	// upload texture data
	glTexImage2D(GL_TEXTURE_2D, 0, internalformat, w, h, 0, format, GL_UNSIGNED_BYTE, pixels);
	CheckForOpenGLErrors("Texture creation", error);

	// If we support generatemipmap, go ahead and do it regardless of the info.mipmap setting.
	// In the GL3 renderer the sampler decides whether or not to do mip filtering,
	// so we conservatively make mipmaps available for all textures.
	if (GLC_ARB_framebuffer_object)
		glGenerateMipmap(GL_TEXTURE_2D);

	SDL_FreeSurface(surface);

	return true;
}

void Texture::Unload()
{
	if (texid)
		glDeleteTextures(1, &texid);
	texid = 0;
}

bool Texture::LoadCubeVerticalCross(const std::string & path, const TextureInfo & info, std::ostream & error)
{
	SDL_Surface * surface = IMG_Load(path.c_str());
	if (!surface)
	{
		error << "Error loading texture file: " + path << std::endl;
		error << IMG_GetError() << std::endl;
		return false;
	}

	target = GL_TEXTURE_CUBE_MAP;

	glGenTextures(1, &texid);
	CheckForOpenGLErrors("Cubemap ID generation", error);

	glBindTexture(GL_TEXTURE_CUBE_MAP, texid);

	// set sampler
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	if (info.mipmap)
	{
		glTexParameteri(GL_TEXTURE_CUBE_MAP,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_LINEAR);
		// automatic mipmap generation fallback
		if (!GLC_ARB_framebuffer_object)
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_GENERATE_MIPMAP, GL_TRUE);
	}

	width = surface->w / 3;
	height = surface->h / 4;

	// upload texture
	unsigned bytespp = surface->format->BytesPerPixel;
	std::vector<unsigned char> cubeface(width * height * bytespp);
	for (int i = 0; i < 6; ++i)
	{
		// detect channels
		int format = GL_RGB;
		switch (bytespp)
		{
			case 1:
				format = GL_RED;
				break;
			case 2:
				format = GL_RG;
				break;
			case 3:
				format = GL_RGB;
				break;
			case 4:
				format = GL_RGBA;
				break;
			default:
				error << "Texture has unknown format: " + path << std::endl;
				return false;
				break;
		}

		int offsetx = 0;
		int offsety = 0;

		GLenum targetparam;
		if (i == 0)
		{
			targetparam = GL_TEXTURE_CUBE_MAP_NEGATIVE_X;
			offsetx = 0;
			offsety = height;
		}
		else if (i == 1)
		{
			targetparam = GL_TEXTURE_CUBE_MAP_POSITIVE_X;
			offsetx = width*2;
			offsety = height;
		}
		else if (i == 2)
		{
			targetparam = GL_TEXTURE_CUBE_MAP_NEGATIVE_Y;
			offsetx = width;
			offsety = height*2;
		}
		else if (i == 3)
		{
			targetparam = GL_TEXTURE_CUBE_MAP_POSITIVE_Y;
			offsetx = width;
			offsety = 0;
		}
		else if (i == 4)
		{
			targetparam = GL_TEXTURE_CUBE_MAP_NEGATIVE_Z;
			offsetx = width;
			offsety = height*3;
		}
		else if (i == 5)
		{
			targetparam = GL_TEXTURE_CUBE_MAP_POSITIVE_Z;
			offsetx = width;
			offsety = height;
		}
		else
		{
			error << "Texture has unknown format: " + path << std::endl;
			return false;
		}

		if (i == 4) //special case for negative z
		{
			for (unsigned yi = 0; yi < height; yi++)
			{
				for (unsigned xi = 0; xi < width; xi++)
				{
					for (unsigned ci = 0; ci < bytespp; ci++)
					{
						int idx1 = ((height - yi - 1) + offsety) * surface->w * bytespp + (width - xi - 1 + offsetx) * bytespp + ci;
						int idx2 = yi * width * bytespp + xi * bytespp + ci;
						cubeface[idx2] = ((unsigned char *)(surface->pixels))[idx1];
					}
				}
			}
		}
		else
		{
			for (unsigned yi = 0; yi < height; yi++)
			{
				for (unsigned xi = 0; xi < width; xi++)
				{
					for (unsigned ci = 0; ci < bytespp; ci++)
					{
						int idx1 = (yi + offsety) * surface->w * bytespp + (xi + offsetx) * bytespp + ci;
						int idx2 = yi * width * bytespp + xi * bytespp + ci;
						cubeface[idx2] = ((unsigned char *)(surface->pixels))[idx1];
					}
				}
			}
		}
		glTexImage2D(targetparam, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, &cubeface[0]);
	}

	if (info.mipmap && GLC_ARB_framebuffer_object)
		glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

	CheckForOpenGLErrors("Cubemap creation", error);

	SDL_FreeSurface(surface);

	return true;
}

bool Texture::LoadCube(const std::string & path, const TextureInfo & info, std::ostream & error)
{
	if (info.verticalcross)
	{
		return LoadCubeVerticalCross(path, info, error);
	}

	std::string cubefiles[6];
	cubefiles[0] = path+"-xp.png";
	cubefiles[1] = path+"-xn.png";
	cubefiles[2] = path+"-yn.png";
	cubefiles[3] = path+"-yp.png";
	cubefiles[4] = path+"-zn.png";
	cubefiles[5] = path+"-zp.png";

	target = GL_TEXTURE_CUBE_MAP;

	glGenTextures(1, &texid);
	CheckForOpenGLErrors("Cubemap texture ID generation", error);

	glBindTexture(GL_TEXTURE_CUBE_MAP, texid);

	for (int i = 0; i < 6; ++i)
	{
		SDL_Surface * surface = IMG_Load(cubefiles[i].c_str());
		if (!surface)
		{
			error << "Error loading texture file: " + path + " (" + cubefiles[i] + ")" << std::endl;
			error << IMG_GetError() << std::endl;
			return false;
		}

		// store dimensions
		if (i != 0 && ((width != (unsigned)surface->w) || (height != (unsigned)surface->h)))
		{
			error << "Cube map sides aren't equal sizes" << std::endl;
			return false;
		}
		width = surface->w;
		height = surface->h;

		// detect channels
		int format = GL_RGB;
		switch (surface->format->BytesPerPixel)
		{
			case 1:
				format = GL_RED;
				break;
			case 2:
				format = GL_RG;
				break;
			case 3:
				format = GL_RGB;
				break;
			case 4:
				format = GL_RGBA;
				break;
			default:
				error << "Texture has unknown format: " + path + " (" + cubefiles[i] + ")" << std::endl;
				return false;
				break;
		}

		// Create MipMapped Texture
		GLenum targetparam;
		if (i == 0)
			targetparam = GL_TEXTURE_CUBE_MAP_NEGATIVE_X;
		else if (i == 1)
			targetparam = GL_TEXTURE_CUBE_MAP_POSITIVE_X;
		else if (i == 2)
			targetparam = GL_TEXTURE_CUBE_MAP_NEGATIVE_Y;
		else if (i == 3)
			targetparam = GL_TEXTURE_CUBE_MAP_POSITIVE_Y;
		else if (i == 4)
			targetparam = GL_TEXTURE_CUBE_MAP_NEGATIVE_Z;
		else if (i == 5)
			targetparam = GL_TEXTURE_CUBE_MAP_POSITIVE_Z;
		else
		{
			error << "Iterated too far: " + path + " (" + cubefiles[i] + ")" << std::endl;
			assert(0);
		}

		glTexImage2D(targetparam, 0, format, surface->w, surface->h, 0, format, GL_UNSIGNED_BYTE, surface->pixels );

		SDL_FreeSurface(surface);
	}

	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	CheckForOpenGLErrors("Cubemap creation", error);

	return true;
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
	file.read(&data[0], length);

	// load dds
	const char * texdata(0);
	unsigned long texlen(0);
	unsigned format(0);
	unsigned levels(0);
	if (!ReadDDS(
		(void*)&data[0], length,
		(const void*&)texdata, texlen,
		format, width, height, levels))
	{
		return false;
	}

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

	// load texture
	target = GL_TEXTURE_2D;

	assert(!texid);
	glGenTextures(1, &texid);
	CheckForOpenGLErrors("Texture ID generation", error);

	glBindTexture(GL_TEXTURE_2D, texid);

	SetSampler(info, levels > 1);

	const char * idata = texdata;
	unsigned blocklen = 16 * texlen / (width * height);
	unsigned ilen = texlen;
	unsigned iw = width;
	unsigned ih = height;
	for (unsigned i = 0; i < levels; ++i)
	{
		if (format == GL_BGR || format == GL_BGRA)
		{
			// fixme: support compression here?
			ilen = iw * ih * blocklen / 16;
			glTexImage2D(GL_TEXTURE_2D, i, iformat, iw, ih, 0, format, GL_UNSIGNED_BYTE, idata);
		}
		else
		{
			ilen = std::max(1u, iw / 4) * std::max(1u, ih / 4) * blocklen;
			glCompressedTexImage2D(GL_TEXTURE_2D, i, iformat, iw, ih, 0, ilen, idata);
		}
		CheckForOpenGLErrors("Texture creation", error);

		idata += ilen;
		iw = std::max(1u, iw / 2);
		ih = std::max(1u, ih / 2);
	}

	// force mipmaps for GL3
	if (levels == 1 && GLC_ARB_framebuffer_object)
		glGenerateMipmap(GL_TEXTURE_2D);

	return true;
}
