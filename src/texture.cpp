#include "texture.h"
#include "opengl_utility.h"

#ifdef __APPLE__
#include <SDL_image/SDL_image.h>
#include <SDL_gfx/SDL_rotozoom.h>
#else
#include <SDL/SDL_image.h>
#include <SDL/SDL_rotozoom.h>
#endif

#include <string>
#include <iostream>
#include <vector>
#include <cassert>

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

static float Scale(const std::string & size, float width, float height)
{
	float maxsize, minscale;

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

	float scalew = (width > maxsize) ? maxsize / width : 1.0;
	float scaleh = (height > maxsize) ? maxsize / height : 1.0;
	float scale = (scalew < scaleh) ? scalew : scaleh;
	if (scale < minscale) scale = minscale;

	return scale;
}

static bool IsPowerOfTwo(int x)
{
	return ((x != 0) && !(x & (x - 1)));
}

bool TEXTURE::LoadCubeVerticalCross(const std::string & path, const TEXTUREINFO & info, std::ostream & error)
{
	std::string cubefile = path;

	GLuint new_handle = 0;
	glGenTextures(1, &new_handle);
	OPENGL_UTILITY::CheckForOpenGLErrors("Cubemap ID generation", error);
	id = new_handle;

	glBindTexture(GL_TEXTURE_CUBE_MAP, new_handle);

	SDL_Surface * texture_surface = IMG_Load(cubefile.c_str());
	if (texture_surface)
	{
		for (int i = 0; i < 6; ++i)
		{
			w = texture_surface->w/3;
			h = texture_surface->h/4;

			//detect channels
			int format = GL_RGB;
			switch (texture_surface->format->BytesPerPixel)
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

			if (format != GL_RGB)
			{
				//throw EXCEPTION(__FILE__, __LINE__, "Cube map texture format isn't GL_RGB (this causes problems for some reason): " + texture_path + " (" + cubefile + ")");
				//game.WriteDebuggingData("Warning:  Cube map texture format isn't GL_RGB (this causes problems for some reason): " + texture_path + " (" + cubefile + ")");
			}

			int offsetx = 0;
			int offsety = 0;

			GLenum targetparam;
			if (i == 0)
			{
				targetparam = GL_TEXTURE_CUBE_MAP_NEGATIVE_X;
				offsetx = 0;
				offsety = h;
			}
			else if (i == 1)
			{
				targetparam = GL_TEXTURE_CUBE_MAP_POSITIVE_X;
				offsetx = w*2;
				offsety = h;
			}
			else if (i == 2)
			{
				targetparam = GL_TEXTURE_CUBE_MAP_NEGATIVE_Y;
				offsetx = w;
				offsety = h*2;
			}
			else if (i == 3)
			{
				targetparam = GL_TEXTURE_CUBE_MAP_POSITIVE_Y;
				offsetx = w;
				offsety = 0;
			}
			else if (i == 4)
			{
				targetparam = GL_TEXTURE_CUBE_MAP_NEGATIVE_Z;
				offsetx = w;
				offsety = h*3;
			}
			else if (i == 5)
			{
				targetparam = GL_TEXTURE_CUBE_MAP_POSITIVE_Z;
				offsetx = w;
				offsety = h;
			}
			else
			{
				error << "Texture has unknown format: " + path << std::endl;
				return false;
			}

			unsigned char * cubeface = new unsigned char[w*h*texture_surface->format->BytesPerPixel];

			if (i == 4) //special case for negative z
			{
				for (unsigned int yi = 0; yi < h; yi++)
				{
					for (unsigned int xi = 0; xi < w; xi++)
					{
						for (unsigned int ci = 0; ci < texture_surface->format->BytesPerPixel; ci++)
						{
							int idx1 = ((h-yi-1)+offsety)*texture_surface->w*texture_surface->format->BytesPerPixel + (w-xi-1+offsetx)*texture_surface->format->BytesPerPixel + ci;
							int idx2 = yi*w*texture_surface->format->BytesPerPixel+xi*texture_surface->format->BytesPerPixel+ci;
							cubeface[idx2] = ((unsigned char *)(texture_surface->pixels))[idx1];
							//cout << idx1 << "," << idx2 << endl;
						}
					}
				}
			}
			else
			{
				for (unsigned int yi = 0; yi < h; yi++)
				{
					for (unsigned int xi = 0; xi < w; xi++)
					{
						for (unsigned int ci = 0; ci < texture_surface->format->BytesPerPixel; ci++)
						{
							int idx1 = (yi+offsety)*texture_surface->w*texture_surface->format->BytesPerPixel+(xi+offsetx)*texture_surface->format->BytesPerPixel+ci;
							int idx2 = yi*w*texture_surface->format->BytesPerPixel+xi*texture_surface->format->BytesPerPixel+ci;
							cubeface[idx2] = ((unsigned char *)(texture_surface->pixels))[idx1];
							//cout << idx1 << "," << idx2 << endl;
						}
					}
				}
			}
			glTexImage2D( targetparam, 0, format, w, h, 0, format, GL_UNSIGNED_BYTE, cubeface );
			delete [] cubeface;
		}
	}
	else
	{
		error << "Error loading texture file: " + path << std::endl;
		return false;
	}

	if (texture_surface)
	{
		// Free up any memory we may have used
		SDL_FreeSurface( texture_surface );
		texture_surface = NULL;
	}

	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	if (info.mipmap)
	{
		glTexParameteri(GL_TEXTURE_CUBE_MAP,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP,GL_TEXTURE_MAG_FILTER,GL_LINEAR);

		if (GLEW_ARB_framebuffer_object)
		{
			glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
		}
	}

	glDisable(GL_TEXTURE_CUBE_MAP);

	OPENGL_UTILITY::CheckForOpenGLErrors("Cubemap creation", error);

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

	GLuint new_handle = 0;
	glGenTextures(1, &new_handle);
	OPENGL_UTILITY::CheckForOpenGLErrors("Cubemap texture ID generation", error);
	id = new_handle;

	glBindTexture(GL_TEXTURE_CUBE_MAP, new_handle);

	for (unsigned int i = 0; i < 6; ++i)
	{
		SDL_Surface * texture_surface = IMG_Load(cubefiles[i].c_str());
		if (texture_surface)
		{
			//store dimensions
			if (i != 0 && (w != (unsigned int) texture_surface->w || h != (unsigned int) texture_surface->h))
			{
				error << "Cube map sides aren't equal sizes" << std::endl;
				return false;
			}
			w = texture_surface->w;
			h = texture_surface->h;

			//detect channels
			int format = GL_RGB;
			switch (texture_surface->format->BytesPerPixel)
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

			if (format != GL_RGB)
			{
				error << "Cube map texture format isn't GL_RGB (this causes problems for some reason): " + path + " (" + cubefiles[i] + ")" << std::endl;
				return false;
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

			glTexImage2D( targetparam, 0, format,texture_surface->w, texture_surface->h, 0, format, GL_UNSIGNED_BYTE, texture_surface->pixels );
		}
		else
		{
			error << "Error loading texture file: " + path + " (" + cubefiles[i] + ")" << std::endl;
			return false;
		}

		if (texture_surface)
		{
			// Free up any memory we may have used
			SDL_FreeSurface( texture_surface );
			texture_surface = NULL;
		}
	}

	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	glDisable(GL_TEXTURE_CUBE_MAP);

	OPENGL_UTILITY::CheckForOpenGLErrors("Cubemap creation", error);

	return true;
}

void GenTexture(const SDL_Surface * surface, const TEXTUREINFO & info, GLuint & id, bool & alphachannel, std::ostream & error)
{
	//detect channels
	bool compression = (surface->w > 512 || surface->h > 512) && !info.normalmap;
	bool srgb = info.srgb;
	int format;
	int internalformat = compression ? (srgb ? GL_COMPRESSED_SRGB : GL_COMPRESSED_RGB) : (srgb ? GL_SRGB8 : GL_RGB);
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
#ifdef __APPLE__
            format = GL_BGR;
#else
			format = GL_RGB;
#endif
            internalformat = compression ? (srgb ? GL_COMPRESSED_SRGB : GL_COMPRESSED_RGB) : (srgb ? GL_SRGB8 : GL_RGB);
			alphachannel = false;
			break;
		case 4:
#ifdef __APPLE__
            format = GL_BGRA;
#else
			format = GL_RGBA;
#endif
			internalformat = compression ? (srgb ? GL_COMPRESSED_SRGB_ALPHA : GL_COMPRESSED_RGBA) : (srgb ? GL_SRGB8_ALPHA8 : GL_RGBA);
			alphachannel = true;
			break;
		default:
#ifdef __APPLE__
            format = GL_BGR;
#else
            format = GL_RGB;
#endif
			break;
	}

	glGenTextures(1, &id);
	OPENGL_UTILITY::CheckForOpenGLErrors("Texture ID generation", error);

	// Create MipMapped Texture
	glBindTexture(GL_TEXTURE_2D, id);
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
		if (!glGenerateMipmap) // this kind of automatic mipmap generation is deprecated in GL3, so don't use it
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
	glTexImage2D( GL_TEXTURE_2D, 0, internalformat, surface->w, surface->h, 0, format, GL_UNSIGNED_BYTE, surface->pixels );
	OPENGL_UTILITY::CheckForOpenGLErrors("Texture creation", error);

	// If we support generatemipmap, go ahead and do it regardless of the info.mipmap setting.
	// In the GL3 renderer the sampler decides whether or not to do mip filtering, so we conservatively make mipmaps available for all textures.
	if (glGenerateMipmap)
		glGenerateMipmap(GL_TEXTURE_2D);

	//check for anisotropy
	if (info.anisotropy > 1)
	{
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, (float)info.anisotropy);
	}
}

bool TEXTURE::Load(const std::string & path, const TEXTUREINFO & info, std::ostream & error)
{
	if (id)
	{
		error << "Tried to double load texture " << path << std::endl;
		return false;
	}

	if (path.empty() && !info.surface)
	{
		error << "Tried to load a texture with an empty name" << std::endl;
		return false;
	}

	id = 0;
	if (info.cube)
	{
		cube = true;
		return LoadCube(path, info, error);
	}

	SDL_Surface * orig_surface = info.surface;
	if (!orig_surface)
	{
		orig_surface = IMG_Load(path.c_str());
		if (!orig_surface)
		{
			error << "Error loading texture file: " << path << std::endl;
			return false;
		}
	}

	SDL_Surface * texture_surface(orig_surface);
	if (orig_surface)
	{
	    origw = texture_surface->w;
        origh = texture_surface->h;

		//scale to power of two if necessary
		bool norescale = (IsPowerOfTwo(orig_surface->w) && IsPowerOfTwo(orig_surface->h)) ||
					(info.npot && (GLEW_VERSION_2_0 || GLEW_ARB_texture_non_power_of_two));
		if (!norescale)
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

		//scale texture down if necessary
		scale = Scale(info.size, orig_surface->w, orig_surface->h);
		if (scale < 1.0)
		{
			texture_surface = zoomSurface (orig_surface, scale, scale, SMOOTHING_ON);
		}

		//store dimensions
		w = texture_surface->w;
		h = texture_surface->h;

		GenTexture(texture_surface, info, id, alpha, error);
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

	return true;
}

void TEXTURE::Activate() const
{
	assert(id);
	if (cube)
	{
		glBindTexture(GL_TEXTURE_CUBE_MAP, id);
	}
	else
	{
		glBindTexture(GL_TEXTURE_2D, id);
	}
}

void TEXTURE::Deactivate() const
{
	glDisable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, 0);
}

void TEXTURE::Unload()
{
	if (id) glDeleteTextures(1, &id);
	id = 0;
}
