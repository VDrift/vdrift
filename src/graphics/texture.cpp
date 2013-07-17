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
#include "glutil.h"
#include "glew.h"
#include "dds.h"

#ifdef __APPLE__
#include <SDL_image/SDL_image.h>
#include <SDL_gfx/SDL_rotozoom.h>
#else
#include <SDL/SDL_image.h>
#include <SDL/SDL_rotozoom.h>
#endif

#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include <cassert>

static bool IsPowerOfTwo(int x)
{
	return ((x != 0) && !(x & (x - 1)));
}

static float Scale(TEXTUREINFO::Size size, float width, float height)
{
	float maxsize, minscale;
	if (size == TEXTUREINFO::SMALL)
	{
		maxsize = 128;
		minscale = 0.25;
	}
	else if (size == TEXTUREINFO::MEDIUM)
	{
		maxsize = 256;
		minscale = 0.5;
	}
	else
	{
		return 1.0;
	}

	float scalew = (width > maxsize) ? maxsize / width : 1.0;
	float scaleh = (height > maxsize) ? maxsize / height : 1.0;
	float scale = (scalew < scaleh) ? scalew : scaleh;
	if (scale < minscale) scale = minscale;

	return scale;
}

static void GetTextureFormat(
	const SDL_Surface * surface,
	const TEXTUREINFO & info,
	int & internalformat,
	int & format,
	bool & alpha)
{
	bool compress = info.compress && (surface->w > 512 || surface->h > 512);
	bool srgb = info.srgb;

	internalformat = compress ? (srgb ? GL_COMPRESSED_SRGB : GL_COMPRESSED_RGB) : (srgb ? GL_SRGB8 : GL_RGB);
	switch (surface->format->BytesPerPixel)
	{
		case 1:
			internalformat = compress ? GL_COMPRESSED_LUMINANCE : GL_LUMINANCE;
			format = GL_LUMINANCE;
			alpha = false;
			break;
		case 2:
			internalformat = compress ? GL_COMPRESSED_LUMINANCE_ALPHA : GL_LUMINANCE_ALPHA;
			format = GL_LUMINANCE_ALPHA;
			alpha = true;
			break;
		case 3:
			internalformat = compress ? (srgb ? GL_COMPRESSED_SRGB : GL_COMPRESSED_RGB) : (srgb ? GL_SRGB8 : GL_RGB);
#ifdef __APPLE__
			format = GL_BGR;
#else
			format = GL_RGB;
#endif
			alpha = false;
			break;
		case 4:
			internalformat = compress ? (srgb ? GL_COMPRESSED_SRGB_ALPHA : GL_COMPRESSED_RGBA) : (srgb ? GL_SRGB8_ALPHA8 : GL_RGBA);
#ifdef __APPLE__
			format = GL_BGRA;
#else
			format = GL_RGBA;
#endif
			alpha = true;
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

static void GenerateMipmap(GLenum target)
{
	if (glGenerateMipmap)
	{
		glGenerateMipmap(target);
		// mesa tampering with attribute arrays, reset explicitly
		glDisableVertexAttribArray(0);
		glDisableVertexAttribArray(1);
	}
}

static void SetSampler(const TEXTUREINFO & info, bool hasmiplevels = false)
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
		// automatic mipmap generation (deprecated in GL3)
		if (!hasmiplevels && info.mipmap && !glGenerateMipmap)
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

static void GenTexture(
	const SDL_Surface * surface,
	const TEXTUREINFO & info,
	unsigned & id,
	bool & alpha,
	std::ostream & error)
{
	// detect channels
	int internalformat, format;
	GetTextureFormat(surface, info, internalformat, format, alpha);

	// gen texture
	glGenTextures(1, &id);
	GLUTIL::CheckForOpenGLErrors("Texture ID generation", error);

	// init texture
	glBindTexture(GL_TEXTURE_2D, id);

	SetSampler(info);

	glTexImage2D(GL_TEXTURE_2D, 0, internalformat, surface->w, surface->h, 0, format, GL_UNSIGNED_BYTE, surface->pixels);
	GLUTIL::CheckForOpenGLErrors("Texture creation", error);

	// If we support generatemipmap, go ahead and do it regardless of the info.mipmap setting.
	// In the GL3 renderer the sampler decides whether or not to do mip filtering, so we conservatively make mipmaps available for all textures.
	GenerateMipmap(GL_TEXTURE_2D);
}

TEXTURE::TEXTURE():
	m_id(0),
	m_w(0),
	m_h(0),
	m_scale(1.0),
	m_alpha(false),
	m_cube(false)
{
	// ctor
}

TEXTURE::~TEXTURE()
{
	Unload();
}

void TEXTURE::Activate() const
{
	assert(m_id);
	if (m_cube)
	{
		glEnable(GL_TEXTURE_CUBE_MAP);
		glBindTexture(GL_TEXTURE_CUBE_MAP, m_id);
	}
	else
	{
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, m_id);
	}
}

void TEXTURE::Deactivate() const
{
	if (m_cube)
	{
		glDisable(GL_TEXTURE_CUBE_MAP);
		glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
	}
	else
	{
		glDisable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, 0);
	}
}

bool TEXTURE::Load(const std::string & path, const TEXTUREINFO & info, std::ostream & error)
{
	if (m_id)
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
		return true;

	m_cube = info.cube;
	if (m_cube)
	{
		return LoadCube(path, info, error);
	}

	SDL_Surface * orig_surface = 0;
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
		orig_surface = SDL_CreateRGBSurfaceFrom(
			info.data, info.width, info.height,
			info.bytespp * 8, info.width * info.bytespp,
			rmask, gmask, bmask, amask);
	}
	if (!orig_surface)
	{
		orig_surface = IMG_Load(path.c_str());
		if (!orig_surface)
		{
			error << "Error loading texture file: " << path << std::endl;
			error << IMG_GetError() << std::endl;
			return false;
		}
	}

	SDL_Surface * surface = orig_surface;
	if (surface)
	{
		m_scale = Scale(info.maxsize, orig_surface->w, orig_surface->h);
		float scalew = m_scale;
		float scaleh = m_scale;

		// scale to power of two if necessary
		bool norescale = (IsPowerOfTwo(orig_surface->w) && IsPowerOfTwo(orig_surface->h)) ||
					(info.npot && (GLEW_VERSION_2_0 || GLEW_ARB_texture_non_power_of_two));

		if (!norescale)
		{
			int maxsize = 2048;
			int new_w = orig_surface->w;
			int new_h = orig_surface->h;

			if (!IsPowerOfTwo(orig_surface->w))
			{
				for (new_w = 1; new_w <= maxsize && new_w <= orig_surface->w * m_scale; new_w = new_w * 2);
			}

			if (!IsPowerOfTwo(orig_surface->h))
			{
				 for (new_h = 1; new_h <= maxsize && new_h <= orig_surface->h * m_scale; new_h = new_h * 2);
			}

			scalew = ((float)new_w + 0.5) / orig_surface->w;
			scaleh = ((float)new_h + 0.5) / orig_surface->h;
		}

		// scale texture down if necessary
		if (scalew < 1.0 || scaleh < 1.0)
		{
			surface = zoomSurface(orig_surface, scalew, scaleh, SMOOTHING_ON);
		}

		// store dimensions
		m_w = surface->w;
		m_h = surface->h;

		GenTexture(surface, info, m_id, m_alpha, error);
	}

	// free the texture surface separately if it's a scaled copy of the original
	if (surface && surface != orig_surface )
	{
		SDL_FreeSurface(surface);
	}

	// free the original surface if it's not a custom surface (used for the track map)
	if (!info.data && orig_surface)
	{
		SDL_FreeSurface(orig_surface);
	}

	return true;
}

void TEXTURE::Unload()
{
	if (m_id)
		glDeleteTextures(1, &m_id);
	m_id = 0;
}

bool TEXTURE::LoadCubeVerticalCross(const std::string & path, const TEXTUREINFO & info, std::ostream & error)
{
	SDL_Surface * surface = IMG_Load(path.c_str());
	if (!surface)
	{
		error << "Error loading texture file: " + path << std::endl;
		error << IMG_GetError() << std::endl;
		return false;
	}

	unsigned id = 0;
	glGenTextures(1, &id);
	GLUTIL::CheckForOpenGLErrors("Cubemap ID generation", error);
	glBindTexture(GL_TEXTURE_CUBE_MAP, id);
	glEnable(GL_TEXTURE_CUBE_MAP);

	// set sampler
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	if (info.mipmap)
	{
		glTexParameteri(GL_TEXTURE_CUBE_MAP,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_LINEAR);
		if (!glGenerateMipmap)
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_GENERATE_MIPMAP, GL_TRUE);
	}

	m_id = id;
	m_w = surface->w / 3;
	m_h = surface->h / 4;

	// upload texture
	unsigned bytespp = surface->format->BytesPerPixel;
	std::vector<unsigned char> cubeface(m_w * m_h * bytespp);
	for (int i = 0; i < 6; ++i)
	{
		// detect channels
		int format = GL_RGB;
		switch (bytespp)
		{
			case 1:
				format = GL_LUMINANCE;
				break;
			case 2:
				format = GL_LUMINANCE_ALPHA;
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
			offsety = m_h;
		}
		else if (i == 1)
		{
			targetparam = GL_TEXTURE_CUBE_MAP_POSITIVE_X;
			offsetx = m_w*2;
			offsety = m_h;
		}
		else if (i == 2)
		{
			targetparam = GL_TEXTURE_CUBE_MAP_NEGATIVE_Y;
			offsetx = m_w;
			offsety = m_h*2;
		}
		else if (i == 3)
		{
			targetparam = GL_TEXTURE_CUBE_MAP_POSITIVE_Y;
			offsetx = m_w;
			offsety = 0;
		}
		else if (i == 4)
		{
			targetparam = GL_TEXTURE_CUBE_MAP_NEGATIVE_Z;
			offsetx = m_w;
			offsety = m_h*3;
		}
		else if (i == 5)
		{
			targetparam = GL_TEXTURE_CUBE_MAP_POSITIVE_Z;
			offsetx = m_w;
			offsety = m_h;
		}
		else
		{
			error << "Texture has unknown format: " + path << std::endl;
			return false;
		}

		if (i == 4) //special case for negative z
		{
			for (unsigned yi = 0; yi < m_h; yi++)
			{
				for (unsigned xi = 0; xi < m_w; xi++)
				{
					for (unsigned ci = 0; ci < bytespp; ci++)
					{
						int idx1 = ((m_h - yi - 1) + offsety) * surface->w * bytespp + (m_w - xi - 1 + offsetx) * bytespp + ci;
						int idx2 = yi * m_w * bytespp + xi * bytespp + ci;
						cubeface[idx2] = ((unsigned char *)(surface->pixels))[idx1];
					}
				}
			}
		}
		else
		{
			for (unsigned yi = 0; yi < m_h; yi++)
			{
				for (unsigned xi = 0; xi < m_w; xi++)
				{
					for (unsigned ci = 0; ci < bytespp; ci++)
					{
						int idx1 = (yi + offsety) * surface->w * bytespp + (xi + offsetx) * bytespp + ci;
						int idx2 = yi * m_w * bytespp + xi * bytespp + ci;
						cubeface[idx2] = ((unsigned char *)(surface->pixels))[idx1];
					}
				}
			}
		}
		glTexImage2D(targetparam, 0, format, m_w, m_h, 0, format, GL_UNSIGNED_BYTE, &cubeface[0]);
	}

	if (info.mipmap)
		GenerateMipmap(GL_TEXTURE_CUBE_MAP);

	glDisable(GL_TEXTURE_CUBE_MAP);

	GLUTIL::CheckForOpenGLErrors("Cubemap creation", error);

	SDL_FreeSurface(surface);

	return true;
}

bool TEXTURE::LoadCube(const std::string & path, const TEXTUREINFO & info, std::ostream & error)
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

	unsigned id = 0;
	glGenTextures(1, &id);
	GLUTIL::CheckForOpenGLErrors("Cubemap texture ID generation", error);
	glBindTexture(GL_TEXTURE_CUBE_MAP, id);
	m_id = id;

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
		if (i != 0 && ((m_w != (unsigned)surface->w) || (m_h != (unsigned)surface->h)))
		{
			error << "Cube map sides aren't equal sizes" << std::endl;
			return false;
		}
		m_w = surface->w;
		m_h = surface->h;

		// detect channels
		int format = GL_RGB;
		switch (surface->format->BytesPerPixel)
		{
			case 1:
				format = GL_LUMINANCE;
				break;
			case 2:
				format = GL_LUMINANCE_ALPHA;
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

	glDisable(GL_TEXTURE_CUBE_MAP);

	GLUTIL::CheckForOpenGLErrors("Cubemap creation", error);

	return true;
}

bool TEXTURE::LoadDDS(const std::string & path, const TEXTUREINFO & info, std::ostream & error)
{
	std::ifstream file(path.c_str(), std::ifstream::in | std::ifstream::binary);
	if (!file)
		return false;

	// test for dds magic value
	char magic[4];
	file.read(magic, 4);
	if (!isDDS(magic, 4))
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
	unsigned format(0), width(0), height(0), levels(0);
	if (!readDDS(
		(void*)&data[0], length,
		(const void*&)texdata, texlen,
		format, width, height, levels))
	{
		return false;
	}

	// set properties
	m_w = width;
	m_h = height;
	m_scale = 1.0f;
	m_alpha = (format != GL_BGR);
	m_cube = false;

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
	assert(!m_id);
	glGenTextures(1, &m_id);
	GLUTIL::CheckForOpenGLErrors("Texture ID generation", error);

	glBindTexture(GL_TEXTURE_2D, m_id);

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
		GLUTIL::CheckForOpenGLErrors("Texture creation", error);

		idata += ilen;
		iw = std::max(1u, iw / 2);
		ih = std::max(1u, ih / 2);
	}

	// force mipmaps for GL3
	if (levels == 1)
		GenerateMipmap(GL_TEXTURE_2D);

	return true;
}
