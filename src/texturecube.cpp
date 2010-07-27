#include "texturecube.h"
#include "opengl_utility.h"

#ifdef __APPLE__
#include <SDL_image/SDL_image.h>
#else
#include <SDL/SDL_image.h>
#endif

#include <cassert>

TextureCube::TextureCube() :
	loaded(false),
	w(0),
	h(0)
{
	// ctor
}

TextureCube::~TextureCube()
{
	if (loaded)
	{
		glDeleteTextures(1, &tex_id);
	}
	loaded = false;
}

bool TextureCube::Load(const TextureCubeInfo & info, std::ostream & error)
{
	if (loaded)
	{
		error << "Tried to double load texture " << info.name << std::endl;
		return false;
	}

	if (info.name.empty())
	{
		error << "Tried to load a texture with an empty name" << std::endl;
		return false;
	}

	if (info.verticalcross)
	{
		return LoadVerticalCross(info, error);
	}

	std::string cubefiles[6];
	cubefiles[0] = info.name + "-xp.png";
	cubefiles[1] = info.name + "-xn.png";
	cubefiles[2] = info.name + "-yn.png";
	cubefiles[3] = info.name + "-yp.png";
	cubefiles[4] = info.name + "-zn.png";
	cubefiles[5] = info.name + "-zp.png";

	GLuint new_handle = 0;
	glGenTextures(1, &new_handle);
	OPENGL_UTILITY::CheckForOpenGLErrors("Cubemap texture ID generation", error);
	tex_id = new_handle;

	glBindTexture(GL_TEXTURE_CUBE_MAP, new_handle);

	for (unsigned int i = 0; i < 6; i++)
	{
		SDL_Surface * texture_surface = NULL;

		if ((texture_surface = IMG_Load(cubefiles[i].c_str())))
		{
			//store dimensions
			if (i != 0 && (w != (unsigned int) texture_surface->w || h != (unsigned int) texture_surface->h))
			{
				error << "Cube map sides aren't equal sizes: " << info.name << std::endl;
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
					error << "Texture has unknown format: " + info.name + " (" + cubefiles[i] + ")" << std::endl;
					return false;
					break;
			}

			if (format != GL_RGB)
			{
				error << "Cube map texture format isn't GL_RGB (this causes problems for some reason): " + info.name + " (" + cubefiles[i] + ")" << std::endl;
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
				error << "Iterated too far: " + info.name + " (" + cubefiles[i] + ")" << std::endl;
				assert(0);
			}

			glTexImage2D( targetparam, 0, format,texture_surface->w, texture_surface->h, 0, format, GL_UNSIGNED_BYTE, texture_surface->pixels );
		}
		else
		{
			error << "Error loading texture file: " + info.name + " (" + cubefiles[i] + ")" << std::endl;
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

	loaded = true;
	glDisable(GL_TEXTURE_CUBE_MAP);

	OPENGL_UTILITY::CheckForOpenGLErrors("Cubemap creation", error);

	return true;
}

bool TextureCube::LoadVerticalCross(const TextureCubeInfo & info, std::ostream & error)
{
	std::string cubefile = info.name;

	GLuint new_handle = 0;
	glGenTextures(1, &new_handle);
	OPENGL_UTILITY::CheckForOpenGLErrors("Cubemap ID generation", error);
	tex_id = new_handle;

	glBindTexture(GL_TEXTURE_CUBE_MAP, new_handle);

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
					error << "Texture has unknown format: " + info.name << std::endl;
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
				error << "Texture has unknown format: " + info.name << std::endl;
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
		error << "Error loading texture file: " + info.name << std::endl;
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

		glGenerateMipmapEXT(GL_TEXTURE_CUBE_MAP);
	}

	loaded = true;
	glDisable(GL_TEXTURE_CUBE_MAP);

	OPENGL_UTILITY::CheckForOpenGLErrors("Cubemap creation", error);

	return true;
}

void TextureCube::Activate() const
{
	assert(loaded);
	glBindTexture(GL_TEXTURE_CUBE_MAP, tex_id);
}

void TextureCube::Deactivate() const
{
	glDisable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, 0);
}
