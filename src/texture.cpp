#include "texture.h"
#include "opengl_utility.h"

#include <string>
#include <iostream>
#include <vector>
#include <cassert>

#ifdef __APPLE__
#include <SDL_image/SDL_image.h>
#include <SDL_gfx/SDL_rotozoom.h>
#else
#include <SDL/SDL_image.h>
#include <SDL/SDL_rotozoom.h>
#endif

Uint8 ExtractComponent(Uint32 value, Uint32 mask, Uint32 shift, Uint32 loss)
{
	Uint32 temp = value & mask;
	temp = temp >> shift;
	temp = temp << loss;
	return (Uint8) temp;
}

//save the component to the value using the given mask, shift, loss, and return the resulting value
Uint32 InsertComponent(Uint32 value, Uint8 component, Uint32 mask, Uint32 shift, Uint32 loss)
{
	Uint32 temp = component;
	temp = temp >> loss;
	temp = temp << shift;
	return (value & ~mask) | temp;
}

struct COMPONENTINFO
{
	Uint32 mask;
	Uint32 shift;
	Uint32 loss;
};

void PreMultiplyAlpha(SDL_Surface * surface)
{
	if (surface->format->BytesPerPixel == 1)
	{
		//std::cout << "Not true color: " << surface->format->BytesPerPixel << std::endl;
		return; //not a true-color image
	}
	if (surface->format->Amask == 0)
	{
		//std::cout << "No alpha channel: " << surface->format->Amask << std::endl;
		return; //no alpha channel
	}

	int error = SDL_LockSurface(surface);
	assert(!error);

	std::vector <struct COMPONENTINFO> channelinfo(4);
	std::vector <Uint8> channels(4);

	for (int y = 0; y < surface->h; ++y)
	{
		for (int x = 0; x < surface->w; ++x)
		{
			/*char* pixeladdy = & ( ((char*)surface->pixels)[surface->format->BytesPerPixel*x+y*surface->pitch] );

			Uint32 pixel = 0;
			for (int i = 0; i < surface->format->BytesPerPixel; ++i)
				pixel = pixel | ((Uint32)(*(pixeladdy+i)) << 8*i); //little endian
				//pixel = (pixel << 8*i) | (Uint32)(*(pixeladdy+i)); //big endian*/

			Uint32* pixeladdy = & ( ((Uint32*)surface->pixels)[x+y*surface->w] );
			Uint32 pixel = *((Uint32 *)pixeladdy);

			//fill our channel info structs
			channelinfo[0].mask = surface->format->Rmask;
			channelinfo[0].shift = surface->format->Rshift;
			channelinfo[0].loss = surface->format->Rloss;

			channelinfo[1].mask = surface->format->Gmask;
			channelinfo[1].shift = surface->format->Gshift;
			channelinfo[1].loss = surface->format->Gloss;

			channelinfo[2].mask = surface->format->Bmask;
			channelinfo[2].shift = surface->format->Bshift;
			channelinfo[2].loss = surface->format->Bloss;

			channelinfo[3].mask = surface->format->Amask;
			channelinfo[3].shift = surface->format->Ashift;
			channelinfo[3].loss = surface->format->Aloss;

			//extract channels
			for (unsigned int i = 0; i < channelinfo.size(); ++i)
			{
				channels[i] = ExtractComponent(pixel, channelinfo[i].mask, channelinfo[i].shift, channelinfo[i].loss);
			}

			//pre-multiply alpha channel to color channels
			float r = (float)channels[0]/255.0;
			float g = (float)channels[1]/255.0;
			float b = (float)channels[2]/255.0;
			float a = (float)channels[3]/255.0;
			channels[0] = a*r*255.0;
			channels[1] = a*g*255.0;
			channels[2] = a*b*255.0;

			/*channels[0] *= channels[3];
			channels[1] *= channels[3];
			channels[2] *= channels[3];*/

			//save back to the pixel
			for (unsigned int i = 0; i < channelinfo.size(); ++i)
			{
				pixel = InsertComponent(pixel, channels[i], channelinfo[i].mask, channelinfo[i].shift, channelinfo[i].loss);
			}

			*((Uint32 *)pixeladdy) = pixel;
		}
	}

	SDL_UnlockSurface(surface);
}

void GenTexture(const SDL_Surface * surface, const TextureInfo & info, GLuint & tex_id, bool & alphachannel, std::ostream & error)
{
	//detect channels
	bool compression = (surface->w > 512 || surface->h > 512);
	int format = GL_RGB;
	int internalformat = compression ? GL_COMPRESSED_RGB : GL_RGB;
	switch (surface->format->BytesPerPixel)
	{
		case 1:
			format = GL_LUMINANCE;
			internalformat = compression ? GL_COMPRESSED_LUMINANCE : GL_LUMINANCE;
			alphachannel = false;
			break;
		case 2:
			format = GL_LUMINANCE_ALPHA;
			internalformat = compression ? GL_COMPRESSED_LUMINANCE_ALPHA : GL_LUMINANCE_ALPHA;
			alphachannel = true;
			break;
		case 3:
			format = GL_RGB;
			internalformat = compression ? GL_COMPRESSED_RGB : GL_RGB;
			alphachannel = false;
			break;
		case 4:
			format = GL_RGBA;
			internalformat = compression ? GL_COMPRESSED_RGBA : GL_RGBA;
			alphachannel = true;
			break;
		default:
			break;
	}

	glGenTextures(1, &tex_id);
	OPENGL_UTILITY::CheckForOpenGLErrors("Texture ID generation", error);

	// Create MipMapped Texture
	glBindTexture(GL_TEXTURE_2D, tex_id);
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
		//std:: cout << (int) texture_surface->format->BytesPerPixel << "," << format << "," << texture_surface->w << "," << texture_surface->h << "," << texture_surface->pixels << endl;
		gluBuild2DMipmaps( GL_TEXTURE_2D, internalformat, surface->w, surface->h, format, GL_UNSIGNED_BYTE, surface->pixels ); //causes a crash on some png files, dunno why

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
		glTexImage2D( GL_TEXTURE_2D, 0, internalformat, surface->w, surface->h, 0, format, GL_UNSIGNED_BYTE, surface->pixels );
	}
	OPENGL_UTILITY::CheckForOpenGLErrors("Texture creation", error);

	//check for anisotropy
	if (info.anisotropy > 1)
	{
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, (float)info.anisotropy);
	}
}

bool IsPowerOfTwo(int x)
{
	return ((x != 0) && !(x & (x - 1)));
}

float getScale(const std::string & size, float width, float height)
{
	float maxsize, minsize;
	float maxscale, minscale;
	if (size == "medium")
	{
		maxsize = 256;
		minscale = 0.5;
	}
	else if (size == "small")
	{
		maxsize = 128;
		minscale = 0.25;
	}
	else
	{
		return 1.0;
	}

	float scale = 1.0;
	float scalew = 1.0;
	float scaleh = 1.0;

	if (width > maxsize)
		scalew = maxsize / width;
	if (height > maxsize)
		scaleh = maxsize / height;

	if (scalew < scaleh)
		scale = scalew;
	else
		scale = scaleh;

	if (scale < minscale)
		scale = minscale;

	return scale;
}

TEXTURE::TEXTURE() :
	loaded(false),
	w(0),
	h(0),
	origw(0),
	origh(0),
	scale(1.0),
	alphachannel(false)
{
	// ctor
}

TEXTURE::~TEXTURE()
{
	if (loaded)
	{
		glDeleteTextures(1, &tex_id);
	}
	loaded = false;
}

bool TEXTURE::Load(const TextureInfo & info, std::ostream & error)
{
	if (loaded)
	{
		error << "Tried to double load texture " << info.name << std::endl;
		return false;
	}

	if (info.name.empty() && !info.surface)
	{
		error << "Tried to load a texture with an empty name" << std::endl;
		return false;
	}

	tex_id = 0;
	SDL_Surface * orig_surface = info.surface;
	if (!orig_surface)
	{
		orig_surface = IMG_Load(info.name.c_str());
		if (!orig_surface)
		{
			error << "Error loading texture file: " << info.name << std::endl;
			return false;
		}
	}

	SDL_Surface * texture_surface(orig_surface);
	if (orig_surface)
	{
		origw = texture_surface->w;
		origh = texture_surface->h;

		//scale to power of two if necessary
		if (!info.npot && (!IsPowerOfTwo(orig_surface->w) || !IsPowerOfTwo(orig_surface->h)))
		{
			int newx = orig_surface->w;
			int maxsize = 2048;
			if (!IsPowerOfTwo(orig_surface->w))
			{
				for (newx = 1; newx <= maxsize && newx <= orig_surface->w; newx = newx * 2)
				{
				}
			}

			int newy = orig_surface->h;
			if (!IsPowerOfTwo(orig_surface->h))
			{
				for (newy = 1; newy <= maxsize && newy <= orig_surface->h; newy = newy * 2)
				{
				}
			}

			float scalew = ((float)newx+0.5) / orig_surface->w;
			float scaleh = ((float)newy+0.5) / orig_surface->h;

			SDL_Surface * pot_surface = zoomSurface (orig_surface, scalew, scaleh, SMOOTHING_ON);

			assert(IsPowerOfTwo(pot_surface->w));
			assert(IsPowerOfTwo(pot_surface->h));

			SDL_FreeSurface(orig_surface);
			orig_surface = pot_surface;
			texture_surface = orig_surface;
		}

		//std::cout << "NPOT: " << texture_info.GetAllowNonPowerOfTwo() << ": " << !IsPowerOfTwo(orig_surface->w) << ", " << !IsPowerOfTwo(orig_surface->h) << std::endl;

		//scale texture down if necessary
		scale = getScale(info.size, orig_surface->w, orig_surface->h);
		if (scale < 1.0)
		{
			texture_surface = zoomSurface (orig_surface, scale, scale, SMOOTHING_ON);
		}

		//PreMultiplyAlpha(texture_surface);

		//store dimensions
		w = texture_surface->w;
		h = texture_surface->h;

		GenTexture(texture_surface, info, tex_id, alphachannel, error);
	}

	//free the texture surface separately if it's a scaled copy of the original
	if (texture_surface != orig_surface && texture_surface)
	{
		SDL_FreeSurface(texture_surface);
	}

	//free the original surface if it's not a custom surface (used for the track map)
	if (!info.surface && orig_surface)
	{
		SDL_FreeSurface(orig_surface);
	}

	loaded = true;
	return true;
}

void TEXTURE::Activate() const
{
	assert(loaded);
	glBindTexture(GL_TEXTURE_2D, tex_id);
}

void TEXTURE::Deactivate() const
{
	glDisable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, 0);
}
