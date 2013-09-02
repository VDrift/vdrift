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

#include "window.h"
#include "glew.h"
#include <SDL/SDL.h>
#include <sstream>
#include <cassert>
#include <cstdlib>
#include <cstring>

WINDOW_SDL::WINDOW_SDL() :
	w(0), h(0),
	initialized(false),
	fsaa(1),
	surface(NULL),
	window(NULL),
	glcontext(NULL)
{
	// Constructor.
}

WINDOW_SDL::~WINDOW_SDL()
{
#if SDL_VERSION_ATLEAST(2,0,0)
	if (glcontext)
		SDL_GL_DeleteContext(glcontext);

	if (window)
		SDL_DestroyWindow(window);
#endif
	if (initialized)
		SDL_Quit();
}

void WINDOW_SDL::Init(
	const std::string & windowcaption,
	unsigned int resx, unsigned int resy,
	unsigned int bpp, unsigned int depthbpp,
	bool fullscreen, unsigned int antialiasing,
	std::ostream & info_output,
	std::ostream & error_output)
{
	Uint32 sdl_flags = SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_JOYSTICK;
#if SDL_VERSION_ATLEAST(2,0,0)
	sdl_flags |= SDL_INIT_HAPTIC;
#endif
	if (SDL_Init(sdl_flags) < 0)
	{
		error_output << "SDL initialization failed: " << SDL_GetError() << std::endl;
		assert(0);
	}

	ChangeDisplay(resx, resy, bpp, depthbpp, fullscreen, antialiasing, info_output, error_output);

#if SDL_VERSION_ATLEAST(2,0,0)
	SDL_SetWindowTitle(window, windowcaption.c_str());
#else
	SDL_WM_SetCaption(windowcaption.c_str(), NULL);
#endif

	// initialize GLEW
	GLenum glew_err = glewInit();
	if (glew_err != GLEW_OK)
	{
		error_output << "GLEW failed to initialize: " << glewGetErrorString(glew_err) << std::endl;
		assert(glew_err == GLEW_OK);
		initialized = false;
	}
	else
	{
		info_output << "Using GLEW " << glewGetString(GLEW_VERSION) << std::endl;
		initialized = true;
	}

	LogOpenGLInfo(info_output);
}

void WINDOW_SDL::SwapBuffers()
{
#if SDL_VERSION_ATLEAST(2,0,0)
	SDL_GL_SwapWindow(window);
#else
	SDL_GL_SwapBuffers();
#endif
}

void WINDOW_SDL::ShowMouseCursor(bool value)
{
	if (value)
	{
		SDL_ShowCursor(SDL_ENABLE);
#if SDL_VERSION_ATLEAST(2,0,0)
		SDL_SetWindowGrab(window, SDL_FALSE);
#else
		SDL_WM_GrabInput(SDL_GRAB_OFF);
#endif
	}
	else
	{
		SDL_ShowCursor(SDL_DISABLE);
#if SDL_VERSION_ATLEAST(2,0,0)
		SDL_SetWindowGrab(window, SDL_TRUE);
#else
		SDL_WM_GrabInput(SDL_GRAB_ON);
#endif
	}
}

void WINDOW_SDL::Screenshot(const std::string & filename)
{
	SDL_Surface * temp = SDL_CreateRGBSurface(SDL_SWSURFACE, w, h, 24,
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
								0x000000FF, 0x0000FF00, 0x00FF0000, 0
#else
								0x00FF0000, 0x0000FF00, 0x000000FF, 0
#endif
								);
	assert(temp);

	unsigned char *pixels = (unsigned char *) malloc(3 * w * h);
	assert(pixels);

	glReadPixels(0, 0, w, h, GL_RGB, GL_UNSIGNED_BYTE, pixels);

	for (int i = 0; i < h; i++)
		memcpy(((char *) temp->pixels) + temp->pitch * i, pixels + 3 * w * (h - i - 1), w * 3);
	free(pixels);

	SDL_SaveBMP(temp, filename.c_str());
	SDL_FreeSurface(temp);
}

int WINDOW_SDL::GetW() const
{
	return w;
}

int WINDOW_SDL::GetH() const
{
	return h;
}

float WINDOW_SDL::GetWHRatio() const
{
	return (float)w / (float)h;
}

#if SDL_VERSION_ATLEAST(2,0,0)
static int GetVideoDisplay()
{
	const char *variable = SDL_getenv("SDL_VIDEO_FULLSCREEN_DISPLAY");
	if (!variable)
		variable = SDL_getenv("SDL_VIDEO_FULLSCREEN_HEAD");

	if (variable)
		return SDL_atoi(variable);
	else
		return 0;
}

bool WINDOW_SDL::ResizeWindow(int width, int height)
{
	// We can't resize something we don't have.
	if (!window)
		return false;

	// Resize window
	SDL_GetWindowSize(window, &w, &h);
	if (w != width || h != height)
		SDL_SetWindowSize(window, width, height);

	w = width;
	h = height;
	return true;
}
#endif

void WINDOW_SDL::ChangeDisplay(
	int width, int height,
	int bpp, int dbpp,
	bool fullscreen,
	unsigned int antialiasing,
	std::ostream & info_output,
	std::ostream & error_output)
{

	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, dbpp);

	fsaa = 1;
	if (antialiasing > 1)
	{
		fsaa = antialiasing;
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, fsaa);
		info_output << "Enabling antialiasing: " << fsaa << "X" << std::endl;
	}
	else
	{
		info_output << "Disabling antialiasing" << std::endl;
	}

#if SDL_VERSION_ATLEAST(2,0,0)
	SDL_DisplayMode desktop_mode;
	int display = GetVideoDisplay();

	SDL_GetDesktopDisplayMode(display, &desktop_mode);

	if (width == 0)
		width = desktop_mode.w;

	if (height == 0)
		height = desktop_mode.h;

	if (bpp == 0)
		bpp = SDL_BITSPERPIXEL(desktop_mode.format);

	// Try to resize the existing window and surface
	if (!fullscreen && ResizeWindow(width, height))
		return;

	if (glcontext)
	{
		SDL_GL_DeleteContext(glcontext);
		glcontext = NULL;
	}

	if (window)
	{
		SDL_DestroyWindow(window);
	}

	// Create a new window
	Uint32 window_flags = SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL;
	if (fullscreen)
		window_flags |= SDL_WINDOW_FULLSCREEN;

	window = SDL_CreateWindow(NULL,
		SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
		width, height, window_flags);
	if (!window)
	{
		assert(0);
	}

	glcontext = SDL_GL_CreateContext(window);
	if (!glcontext)
	{
		assert(0);
	}
	if (SDL_GL_MakeCurrent(window, glcontext) < 0)
	{
		assert(0);
	}

#else
	const SDL_VideoInfo *videoInfo = SDL_GetVideoInfo();
	if (!videoInfo)
	{
		error_output << "SDL video query failed: " << SDL_GetError() << std::endl;
		assert (0);
	}

	int videoFlags = SDL_OPENGL | SDL_GL_DOUBLEBUFFER | SDL_HWPALETTE;
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
	{
		info_output << "Display change was successful: " << width << "x" << height << "x" << bpp << " " << dbpp << "z fullscreen=" << fullscreen << std::endl;
	}
#endif

	w = width;
	h = height;
}

void WINDOW_SDL::LogOpenGLInfo(std::ostream & info_output)
{
	std::stringstream cardinfo;
	cardinfo << "Video card information:" << std::endl;
	cardinfo << "GL Vendor: " << glGetString(GL_VENDOR) << std::endl;
	cardinfo << "GL Renderer: " << glGetString(GL_RENDERER) << std::endl;
	cardinfo << "GL Version: " << glGetString(GL_VERSION) << std::endl;

	GLint texUnits(0), texUnitsFull(0), texSize(0), maxFloats(0);
	glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &texUnits);
	glGetIntegerv(GL_MAX_TEXTURE_UNITS, &texUnitsFull);
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &texSize);
	glGetIntegerv(GL_MAX_VARYING_FLOATS_ARB, &maxFloats);
	cardinfo << "Texture units: " << texUnitsFull << " full, " << texUnits << " partial" << std::endl;
	cardinfo << "Maximum texture size: " << texSize << std::endl;
	cardinfo << "Maximum varying floats: " << maxFloats;

	info_output << cardinfo.str() << std::endl;
}
