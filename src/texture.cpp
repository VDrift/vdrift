#include "texture.h"

#include <string>
using std::string;

#include <iostream>
using std::cerr;
using std::endl;

#include <cassert>

#ifdef __APPLE__
#include <SDL_image/SDL_image.h>
#include <SDL_gfx/SDL_rotozoom.h>
#else
#include <SDL/SDL_image.h>
#include <SDL/SDL_rotozoom.h>
#endif

#include "opengl_utility.h"

bool TEXTURE_GL::LoadCubeVerticalCross(std::ostream & error_output)
{
	string cubefile = texture_info.GetName();

	GLuint new_handle = 0;
	glGenTextures(1, &new_handle);
	OPENGL_UTILITY::CheckForOpenGLErrors("Cubemap ID generation", error_output);
	tex_id = new_handle;

	glBindTexture(GL_TEXTURE_CUBE_MAP_ARB, new_handle);

	SDL_Surface * texture_surface = NULL;

	if ((texture_surface = IMG_Load(cubefile.c_str())))
	{
		for (int i = 0; i < 6; i++)
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
					error_output << "Texture has unknown format: " + texture_info.GetName() << endl;
					return false;
					break;
			}

			if (format != GL_RGB)
			{
				//throw EXCEPTION(__FILE__, __LINE__, "Cube map texture format isn't GL_RGB (this causes problems for some reason): " + texture_info.GetName() + " (" + cubefile + ")");
				//game.WriteDebuggingData("Warning:  Cube map texture format isn't GL_RGB (this causes problems for some reason): " + texture_info.GetName() + " (" + cubefile + ")");
			}

			int offsetx = 0;
			int offsety = 0;

			GLenum targetparam;
			if (i == 0)
			{
				targetparam = GL_TEXTURE_CUBE_MAP_NEGATIVE_X_ARB;
				offsetx = 0;
				offsety = h;
			}
			else if (i == 1)
			{
				targetparam = GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB;
				offsetx = w*2;
				offsety = h;
			}
			else if (i == 2)
			{
				targetparam = GL_TEXTURE_CUBE_MAP_NEGATIVE_Y_ARB;
				offsetx = w;
				offsety = h*2;
			}
			else if (i == 3)
			{
				targetparam = GL_TEXTURE_CUBE_MAP_POSITIVE_Y_ARB;
				offsetx = w;
				offsety = 0;
			}
			else if (i == 4)
			{
				targetparam = GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_ARB;
				offsetx = w;
				offsety = h*3;
			}
			else if (i == 5)
			{
				targetparam = GL_TEXTURE_CUBE_MAP_POSITIVE_Z_ARB;
				offsetx = w;
				offsety = h;
			}
			else
			{
				error_output << "Texture has unknown format: " + texture_info.GetName() << endl;
				return false;
			}

			unsigned char cubeface[w*h*texture_surface->format->BytesPerPixel];

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
		}
	}
	else
	{
		error_output << "Error loading texture file: " + texture_info.GetName() << endl;
	}

	if (texture_surface)
	{
		// Free up any memory we may have used
		SDL_FreeSurface( texture_surface );
		texture_surface = NULL;
	}

	//glTexParameteri (GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_WRAP_S, GL_REPEAT);
	//glTexParameteri (GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP_ARB,GL_TEXTURE_MAG_FILTER,GL_LINEAR);

	glTexParameteri(GL_TEXTURE_CUBE_MAP_ARB,GL_TEXTURE_MIN_FILTER,GL_LINEAR);

	loaded = true;
	glDisable(GL_TEXTURE_CUBE_MAP_ARB);

	OPENGL_UTILITY::CheckForOpenGLErrors("Cubemap creation", error_output);

	return true;
}

bool TEXTURE_GL::LoadCube(std::ostream & error_output)
{
	if (texture_info.GetVerticalCross())
	{
		return LoadCubeVerticalCross(error_output);
	}

	string cubefiles[6];

	cubefiles[0] = texture_info.GetName()+"-xp.png";
	cubefiles[1] = texture_info.GetName()+"-xn.png";
	cubefiles[2] = texture_info.GetName()+"-yn.png";
	cubefiles[3] = texture_info.GetName()+"-yp.png";
	cubefiles[4] = texture_info.GetName()+"-zn.png";
	cubefiles[5] = texture_info.GetName()+"-zp.png";

	GLuint new_handle = 0;
	glGenTextures(1, &new_handle);
	OPENGL_UTILITY::CheckForOpenGLErrors("Cubemap texture ID generation", error_output);
	tex_id = new_handle;

	glBindTexture(GL_TEXTURE_CUBE_MAP_ARB, new_handle);

	for (unsigned int i = 0; i < 6; i++)
	{
		//bool error = false;

		SDL_Surface * texture_surface = NULL;

		if ((texture_surface = IMG_Load(cubefiles[i].c_str())))
		{
			//store dimensions
			if (i != 0 && (w != (unsigned int) texture_surface->w || h != (unsigned int) texture_surface->h))
			{
				error_output << "Cube map sides aren't equal sizes" << endl;
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
					error_output << "Texture has unknown format: " + texture_info.GetName() + " (" + cubefiles[i] + ")" << endl;
					return false;
					break;
			}

			if (format != GL_RGB)
			{
				error_output << "Cube map texture format isn't GL_RGB (this causes problems for some reason): " + texture_info.GetName() + " (" + cubefiles[i] + ")" << endl;
				return false;
			}

			// Create MipMapped Texture

			GLenum targetparam;
			if (i == 0)
				targetparam = GL_TEXTURE_CUBE_MAP_NEGATIVE_X_ARB;
			else if (i == 1)
				targetparam = GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB;
			else if (i == 2)
				targetparam = GL_TEXTURE_CUBE_MAP_NEGATIVE_Y_ARB;
			else if (i == 3)
				targetparam = GL_TEXTURE_CUBE_MAP_POSITIVE_Y_ARB;
			else if (i == 4)
				targetparam = GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_ARB;
			else if (i == 5)
				targetparam = GL_TEXTURE_CUBE_MAP_POSITIVE_Z_ARB;
			else
			{
				error_output << "Iterated too far: " + texture_info.GetName() + " (" + cubefiles[i] + ")" << endl;
				assert(0);
			}

			glTexImage2D( targetparam, 0, format,texture_surface->w, texture_surface->h, 0, format, GL_UNSIGNED_BYTE, texture_surface->pixels );
		}
		else
		{
			error_output << "Error loading texture file: " + texture_info.GetName() + " (" + cubefiles[i] + ")" << endl;
			return false;
		}

		if (texture_surface)
		{
			// Free up any memory we may have used
			SDL_FreeSurface( texture_surface );
			texture_surface = NULL;
		}
	}

	glTexParameteri(GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP_ARB,GL_TEXTURE_MAG_FILTER,GL_LINEAR);

	glTexParameteri(GL_TEXTURE_CUBE_MAP_ARB,GL_TEXTURE_MIN_FILTER,GL_LINEAR);

	loaded = true;
	glDisable(GL_TEXTURE_CUBE_MAP_ARB);

	OPENGL_UTILITY::CheckForOpenGLErrors("Cubemap creation", error_output);

	return true;
}

bool TEXTURE_GL::Load(std::ostream & error_output, const std::string & texsize)
{
	if (texture_info.GetName().empty() && !texture_info.GetSurface())
	{
		 error_output << "Tried to load a texture with an empty name" << endl;
		 return false;
	}

	if (loaded)
	{
		error_output << "Tried to double load texture " << texture_info.GetName() << endl;
		return false;
	}

	if (texture_info.GetCube())
	{
		return LoadCube(error_output);
	}

	std::string filepath = texture_info.GetName();

	GLuint new_handle = 0;

	SDL_Surface * orig_surface(NULL);
	if (texture_info.GetSurface())
	{
		orig_surface = texture_info.GetSurface();
	}
	else
	{
		orig_surface = IMG_Load(filepath.c_str());
	}

	string texture_size (texsize);

	SDL_Surface * texture_surface(orig_surface);

	if (orig_surface)
	{
	    origw = texture_surface->w;
        origh = texture_surface->h;

	    //scale to power of two if necessary
	    if (!texture_info.GetAllowNonPowerOfTwo() && (!IsPowerOfTwo(orig_surface->w) || !IsPowerOfTwo(orig_surface->h)))
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

	        //std::cout << newy << std::endl;

	        //std::cout << "Old: " << orig_surface->w << ", " << orig_surface->h << ", New: " << newx << ", " << newy << std::endl;

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
		if (texture_size == "medium" || texture_size == "small")
		{
			int maxsize = 256;
			float minscale = 0.5;
			if (texture_size == "small")
			{
				maxsize = 128;
				minscale = 0.25;
			}
			float scalew = 1.0;
			float scaleh = 1.0;
			scale = 1.0; //member variable

			if (orig_surface->w > maxsize)
				scalew = (float)maxsize / orig_surface->w;
			if (orig_surface->h > maxsize)
				scaleh = (float)maxsize / orig_surface->h;

			if (scalew < scaleh)
				scale = scalew;
			else
				scale = scaleh;

			if (scale < minscale) scale = minscale;

			if (scale < 1.0)
			{
				//cerr << "endian bug!?: " + filename << endl;
				texture_surface = zoomSurface (orig_surface, scale, scale, SMOOTHING_ON);
			}
		}

		//store dimensions
		w = texture_surface->w;
		h = texture_surface->h;

		//detect channels
		int format = GL_RGB;
		switch (texture_surface->format->BytesPerPixel)
		{
			case 1:
				format = GL_LUMINANCE;
				alphachannel = false;
				break;
			case 2:
				format = GL_LUMINANCE_ALPHA;
				alphachannel = true;
				break;
			case 3:
				format = GL_RGB;
				alphachannel = false;
				break;
			case 4:
				format = GL_RGBA;
				alphachannel = true;
				break;
			default:
				break;
		}

		glGenTextures(1, &new_handle);
		OPENGL_UTILITY::CheckForOpenGLErrors("Texture ID generation", error_output);

		// Create MipMapped Texture
		glBindTexture(GL_TEXTURE_2D, new_handle);
		if (texture_info.GetRepeatU())
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		else
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		if (texture_info.GetRepeatV())
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		else
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		if (texture_info.GetMipMap())
		{
			if (texture_info.GetNearest())
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
			gluBuild2DMipmaps( GL_TEXTURE_2D, format, texture_surface->w, texture_surface->h, format, GL_UNSIGNED_BYTE, texture_surface->pixels ); //causes a crash on some png files, dunno why

		}
		else
		{
			if (texture_info.GetNearest())
			{
				glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
				glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
			}
			else
			{
				glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
			}
			glTexImage2D( GL_TEXTURE_2D, 0, format,texture_surface->w, texture_surface->h, 0, format, GL_UNSIGNED_BYTE, texture_surface->pixels );
		}

		OPENGL_UTILITY::CheckForOpenGLErrors("Texture creation", error_output);

		//check for anisotropy
		if (texture_info.GetAnisotropy() > 1)
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, (float)texture_info.GetAnisotropy());
	}
	else
	{
		error_output << "Error loading texture file: " + texture_info.GetName() << endl;
		return false;
	}

	//free the texture surface separately if it's a scaled copy of the original
	if (texture_surface != orig_surface && texture_surface)
	{
		SDL_FreeSurface( texture_surface );
	}

	//free the original surface if it's not a custom surface (used for the track map)
	if (!texture_info.GetSurface() && orig_surface)
	{
		SDL_FreeSurface( orig_surface );
	}

	loaded = true;
	tex_id = new_handle;

	return true;
}

void TEXTURE_GL::Activate() const
{
	assert(loaded);

	if (texture_info.GetCube())
	{
		glBindTexture(GL_TEXTURE_CUBE_MAP_ARB, tex_id);
	}
	else
	{
		glBindTexture(GL_TEXTURE_2D, tex_id);
	}

	OPENGL_UTILITY::CheckForOpenGLErrors("Texture binding ("+texture_info.GetName()+")", std::cerr);
}

void TEXTURE_GL::Unload()
{
	if (loaded)
	{
		glDeleteTextures(1,&tex_id);
		OPENGL_UTILITY::CheckForOpenGLErrors("Texture ID deletion", std::cerr);
	}
	loaded = false;
}

bool TEXTURE_GL::IsEqualTo(const TEXTUREINFO & texinfo) const
{
	return (texinfo.GetName() == texture_info.GetName() && texinfo.GetMipMap() == texture_info.GetMipMap());
}
