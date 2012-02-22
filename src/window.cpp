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

#ifdef __APPLE__
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif
#include <SDL/SDL.h>
#include <cassert>
#include "window.h"

WINDOW_SDL::WINDOW_SDL() : surface(NULL), initialized(false), fsaa(1)
{
    // Constructor.
}

WINDOW_SDL::~WINDOW_SDL()
{
	if (initialized)
		SDL_Quit();
}

void WINDOW_SDL::Init(const std::string & windowcaption, unsigned int resx, unsigned int resy, unsigned int bpp, unsigned int depthbpp, bool fullscreen, unsigned int antialiasing, bool enableGL3, std::ostream & info_output, std::ostream & error_output)
{
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_JOYSTICK) < 0)
	{
        // Die...
		error_output << "SDL initialization failed: " << SDL_GetError() << std::endl;
		assert(0);
	}

	ChangeDisplay(resx, resy, bpp, depthbpp, fullscreen, antialiasing, enableGL3, info_output, error_output);

	SDL_WM_SetCaption(windowcaption.c_str(), NULL);

	initialized = true;
}

void WINDOW_SDL::SwapBuffers()
{
	SDL_GL_SwapBuffers();
}

void WINDOW_SDL::Screenshot(std::string filename)
{
	SDL_Surface *screen;
	SDL_Surface *temp = NULL;
	unsigned char *pixels;
	int i;

	screen = surface;

	if (!(screen->flags & SDL_OPENGL))
	{
		SDL_SaveBMP(temp, filename.c_str());
		return;
	}

	temp = SDL_CreateRGBSurface(SDL_SWSURFACE, screen->w, screen->h, 24,
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
                                0x000000FF, 0x0000FF00, 0x00FF0000, 0
#else
                                0x00FF0000, 0x0000FF00, 0x000000FF, 0
#endif
                                );

	assert(temp);

	pixels = (unsigned char *) malloc(3 * screen->w * screen->h);
	assert(pixels);

	glReadPixels(0, 0, screen->w, screen->h, GL_RGB, GL_UNSIGNED_BYTE, pixels);

	for (i=0; i<screen->h; i++)
		memcpy(((char *) temp->pixels) + temp->pitch * i, pixels + 3*screen->w * (screen->h-i-1), screen->w*3);
	free(pixels);

	SDL_SaveBMP(temp, filename.c_str());
	SDL_FreeSurface(temp);
}

unsigned int WINDOW_SDL::GetW() const
{
    return w;
}

unsigned int WINDOW_SDL::GetH() const
{
    return h;
}

float WINDOW_SDL::GetWHRatio() const
{
    return (float)w/(float)h;
}

void WINDOW_SDL::ChangeDisplay(const int width, const int height, const int bpp, const int dbpp, const bool fullscreen, unsigned int antialiasing, bool enableGL3, std::ostream & info_output, std::ostream & error_output)
{
	const SDL_VideoInfo *videoInfo = SDL_GetVideoInfo();

	if (!videoInfo)
	{
		error_output << "SDL video query failed: " << SDL_GetError() << std::endl;
		assert (0);
	}

	int videoFlags = SDL_OPENGL | SDL_GL_DOUBLEBUFFER | SDL_HWPALETTE;

	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, dbpp);

	fsaa = 1;
	if (antialiasing > 1)
	{
		fsaa = antialiasing;
		info_output << "Enabling antialiasing: " << fsaa << "X" << std::endl;
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, fsaa);
	}
	else
		info_output << "Disabling antialiasing" << std::endl;

	if (fullscreen)
		videoFlags |= (SDL_HWSURFACE | SDL_ANYFORMAT | SDL_FULLSCREEN);
	else
		videoFlags |= (SDL_SWSURFACE | SDL_ANYFORMAT);

	if (surface != NULL)
	{
		SDL_FreeSurface(surface);
		surface = NULL;
	}

	surface = SDL_SetVideoMode(width, height, bpp, videoFlags);

	if (!surface)
	{
		error_output << "Display change failed: " << width << "x" << height << "x" << bpp << " " << dbpp << "z fullscreen=" << fullscreen << std::endl << "Error: " << SDL_GetError() << std::endl;
		assert (0);
	}
	else
		info_output << "Display change was successful: " << width << "x" << height << "x" << bpp << " " << dbpp << "z fullscreen=" << fullscreen << std::endl;

	w = width;
	h = height;
}
