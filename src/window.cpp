#include "window.h"
#include "matrix4.h"
#include "mathvector.h"
#include "model.h"
#include "texture.h"
#include "vertexarray.h"
#include "reseatable_reference.h"
#include "definitions.h"
#include "containeralgorithm.h"
#include "graphics_config.h"

#include <SDL/SDL.h>

#include <cassert>
#include <sstream>
#include <string>
#include <iostream>
#include <map>
#include <vector>
#include <algorithm>

using std::stringstream;
using std::string;
using std::pair;
using std::endl;
using std::map;
using std::vector;

void WINDOW_SDL::Init(const std::string & windowcaption,
				unsigned int resx, unsigned int resy, unsigned int bpp,
				unsigned int depthbpp, bool fullscreen,
				unsigned int antialiasing,
				bool enableGL3,
				std::ostream & info_output, std::ostream & error_output)
{
	if ( SDL_Init( SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_JOYSTICK
/*#ifdef ENABLE_FORCE_FEEDBACK
			| SDL_INIT_HAPTIC
#endif*/
				 ) < 0 )
	{
		string err = SDL_GetError();
		error_output << "SDL initialization failed: " << err << endl;
		// Die...
		assert(0);
	}
	else
		info_output << "SDL initialization successful" << endl;

	ChangeDisplay(resx, resy, bpp, depthbpp, fullscreen, antialiasing, enableGL3, info_output, error_output);

	SDL_WM_SetCaption(windowcaption.c_str(), NULL);

	initialized = true;
}

void WINDOW_SDL::ChangeDisplay(const int width, const int height, const int bpp, const int dbpp,
				   const bool fullscreen, unsigned int antialiasing, bool enableGL3,
       				   std::ostream & info_output, std::ostream & error_output)
{
	const SDL_VideoInfo *videoInfo = SDL_GetVideoInfo();

	if ( !videoInfo )
	{
		string err = SDL_GetError();
		error_output << "SDL video query failed: " << err << endl;
		assert (0);
	}
	else
		info_output << "SDL video query was successful" << endl;

	int videoFlags;
	videoFlags  = SDL_OPENGL;
	videoFlags |= SDL_GL_DOUBLEBUFFER;
	videoFlags |= SDL_HWPALETTE;
	//videoFlags |= SDL_RESIZABLE;

	/*if (enableGL3)
	{
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
	}*/

	SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );
	SDL_GL_SetAttribute( SDL_GL_DEPTH_SIZE, dbpp );

	fsaa = 1;
	//if (antialiasing > 1 && GLEW_multisample) //can't check this because OpenGL and GLEW aren't initialized
	if (antialiasing > 1)
	{
		fsaa = antialiasing;
		info_output << "Enabling antialiasing: " << fsaa << "X" << endl;
		SDL_GL_SetAttribute( SDL_GL_MULTISAMPLESAMPLES, fsaa );
	}
	else
		info_output << "Disabling antialiasing" << endl;

	if (fullscreen)
	{
		videoFlags |= SDL_HWSURFACE|SDL_ANYFORMAT|SDL_FULLSCREEN;
	}
	else
	{
		videoFlags |= SDL_SWSURFACE|SDL_ANYFORMAT;
	}

	//if (SDL_VideoModeOK(game.config.res_x.data, game.config.res_y.data, game.config.bpp.data, videoFlags) != 0)

	if (surface != NULL)
	{
		SDL_FreeSurface(surface);
		surface = NULL;
	}

	surface = SDL_SetVideoMode(width, height, bpp, videoFlags);

	if (!surface)
	{
		string err = SDL_GetError();
		error_output << "Display change failed: " << width << "x" << height << "x" << bpp << " " << dbpp << "z fullscreen=" << fullscreen << endl;
		error_output << "Error: " << err << endl;
		assert (0);
	}
	else
		info_output << "Display change was successful: " << width << "x" << height << "x" << bpp << " " << dbpp << "z fullscreen=" << fullscreen << endl;

	w = width;
	h = height;
}

void WINDOW_SDL::Deinit()
{
	SDL_Quit();
}

void WINDOW_SDL::SwapBuffers(std::ostream & error_output)
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
