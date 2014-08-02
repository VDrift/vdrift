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
#include <SDL2/SDL.h>
#include <sstream>
#include <cassert>
#include <cstdlib>
#include <cstring>

Window::Window() :
	window(NULL),
	glcontext(NULL),
	fsaa(1),
	w(0),
	h(0),
	initialized(false)
{
	// Constructor.
}

Window::~Window()
{
	if (glcontext)
		SDL_GL_DeleteContext(glcontext);

	if (window)
		SDL_DestroyWindow(window);

	if (initialized)
		SDL_Quit();
}

void Window::Init(
	const std::string & caption,
	int resx,
	int resy,
	int color_bpp,
	int depth_bpp,
	int antialiasing,
	bool fullscreen,
	bool vsync,
	std::ostream & info_output,
	std::ostream & error_output)
{
	Uint32 sdl_flags = SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_JOYSTICK | SDL_INIT_HAPTIC;
	if (SDL_Init(sdl_flags) < 0)
	{
		error_output << "SDL initialization failed: " << SDL_GetError() << std::endl;
		assert(0);
	}

	ChangeDisplay(
		resx, resy, color_bpp, depth_bpp,
		antialiasing, fullscreen, vsync,
		info_output, error_output);

	SDL_SetWindowTitle(window, caption.c_str());

	// Initialize GLEW
	glewExperimental = GL_TRUE;
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

	// Clear GL_INVALID_ENUM caused by glew in core profile by calling glGetString(GL_EXTENSIONS)
	glGetError();

	LogOpenGLInfo(info_output);
}

void Window::SwapBuffers()
{
	SDL_GL_SwapWindow(window);
}

void Window::ShowMouseCursor(bool value)
{
	if (value)
	{
		SDL_ShowCursor(SDL_ENABLE);
		SDL_SetWindowGrab(window, SDL_FALSE);
	}
	else
	{
		SDL_ShowCursor(SDL_DISABLE);
		SDL_SetWindowGrab(window, SDL_TRUE);
	}
}

void Window::Screenshot(const std::string & filename)
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

int Window::GetW() const
{
	return w;
}

int Window::GetH() const
{
	return h;
}

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

bool Window::ResizeWindow(int width, int height)
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

void Window::ChangeDisplay(
	int width,
	int height,
	int color_bpp,
	int depth_bpp,
	int antialiasing,
	bool fullscreen,
	bool vsync,
	std::ostream & info_output,
	std::ostream & error_output)
{
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, depth_bpp);

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

	SDL_DisplayMode desktop_mode;
	int display = GetVideoDisplay();

	SDL_GetDesktopDisplayMode(display, &desktop_mode);

	if (width == 0)
		width = desktop_mode.w;

	if (height == 0)
		height = desktop_mode.h;

	if (color_bpp == 0)
		color_bpp = SDL_BITSPERPIXEL(desktop_mode.format);

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

	int vsync_set = SDL_GL_SetSwapInterval(vsync ? 1 : 0);
	if (vsync_set != -1)
	{
		if (vsync)
			info_output << "Enabling vertical synchronization." << std::endl;
		else
			info_output << "Disabling vertical synchronization." << std::endl;
	}
	else
	{
		info_output << "Setting vertical synchronization not supported." << std::endl;
	}

	w = width;
	h = height;
}

void Window::LogOpenGLInfo(std::ostream & info_output)
{
	std::ostringstream cardinfo;
	cardinfo << "Video card information:" << std::endl;
	cardinfo << "GL Vendor: " << glGetString(GL_VENDOR) << std::endl;
	cardinfo << "GL Renderer: " << glGetString(GL_RENDERER) << std::endl;
	cardinfo << "GL Version: " << glGetString(GL_VERSION) << std::endl;

	GLint texUnits(0), texSize(0);
	glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &texUnits);
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &texSize);
	cardinfo << "Texture units: " << texUnits << std::endl;
	cardinfo << "Maximum texture size: " << texSize;

	info_output << cardinfo.str() << std::endl;
}
