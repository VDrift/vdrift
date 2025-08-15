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
#include "graphics/glcore.h"
#include <SDL3/SDL.h>
#include <sstream>
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <set>

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
		SDL_GL_DestroyContext(glcontext);

	if (window)
		SDL_DestroyWindow(window);

	if (initialized)
		SDL_Quit();
}

void Window::Init(
	const std::string & caption,
	int resx,
	int resy,
	int depth_bpp,
	int antialiasing,
	bool fullscreen,
	bool vsync,
	std::ostream & info_output,
	std::ostream & error_output)
{
	Uint32 sdl_flags = SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_JOYSTICK | SDL_INIT_HAPTIC;
	if (SDL_Init(sdl_flags) == false)
	{
		error_output << "SDL initialization failed: " << SDL_GetError() << std::endl;
		assert(0);
	}

	bool gl3_core = true;
	ChangeDisplay(
		resx, resy, depth_bpp, antialiasing,
		fullscreen, vsync, gl3_core,
		info_output, error_output);

	// try again in case gl3 core failed
	if (glcontext == NULL)
	{
		gl3_core = false;
		ChangeDisplay(
			resx, resy, depth_bpp, antialiasing,
			fullscreen, vsync, gl3_core,
			info_output, error_output);
		// if we fail again we are screwed
		assert(glcontext);
	}

	SDL_SetWindowTitle(window, caption.c_str());

	if (glcLoadFunctions() == GLC_LOAD_FAILED)
	{
		error_output << "OpenGL initialization failed." << std::endl;
		initialized = false;
	}
	else
	{
		initialized = true;
	}

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
		SDL_ShowCursor();
		SDL_SetWindowMouseGrab(window, false);
	}
	else
	{
		SDL_HideCursor();
		SDL_SetWindowMouseGrab(window, true);
	}
}

void Window::Screenshot(const std::string & filename)
{
	unsigned char *pixels = (unsigned char *) malloc(3 * w * h);
	assert(pixels);
	glReadPixels(0, 0, w, h, GL_RGB, GL_UNSIGNED_BYTE, pixels);

#if SDL_BYTEORDER == SDL_LIL_ENDIAN
	SDL_PixelFormat format = SDL_PIXELFORMAT_RGB24;
#else
	SDL_PixelFormat format = SDL_PIXELFORMAT_BGR24;
#endif
	SDL_Surface * surf = SDL_CreateSurfaceFrom(w, h, format, pixels, 3 * w);
	if (surf == NULL)
		return;

	SDL_FlipSurface(surf, SDL_FLIP_VERTICAL);
	SDL_SaveBMP(surf, filename.c_str());
	SDL_DestroySurface(surf);
}

bool Window::Initialized() const
{
	return initialized;
}

int Window::GetW() const
{
	return w;
}

int Window::GetH() const
{
	return h;
}

void Window::GetSupportedResolutions(std::vector<std::pair<int, int>> & res, std::ostream & error_output)
{
	// get supported display modes
	SDL_DisplayID display = SDL_GetPrimaryDisplay();
	int num_modes = 0;
	SDL_DisplayMode **modes = SDL_GetFullscreenDisplayModes(display, &num_modes);
	if (modes == NULL) {
		error_output << SDL_GetError() << std::endl;
		return;
	}

	// get unique resolutions
	std::set<std::pair<int, int>> unique_res;
	for (int i = 0; i < num_modes; ++i)
	{
		SDL_DisplayMode *mode = modes[i];
		unique_res.insert(std::make_pair(mode->w, mode->h));
	}

	res.clear();
	res.reserve(unique_res.size());
	for (const auto & r : unique_res)
	{
		res.push_back(r);
	}

	SDL_free(modes);
	return;
}

bool Window::ResizeWindow(int width, int height)
{
	// We can't resize something we don't have.
	if (!window || !glcontext)
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
	int depth_bpp,
	int antialiasing,
	bool fullscreen,
	bool vsync,
	bool gl3_core,
	std::ostream & info_output,
	std::ostream & error_output)
{
	SDL_DisplayID display = SDL_GetPrimaryDisplay();
	const SDL_DisplayMode * mode = SDL_GetDesktopDisplayMode(display);
	if (mode == NULL)
	{
		error_output << SDL_GetError() << std::endl;
		return;
	}

	if (width == 0)
		width = mode->w;

	if (height == 0)
		height = mode->h;

	// Try to resize the existing window and surface
	if (!fullscreen && ResizeWindow(width, height))
		return;

	if (glcontext)
	{
		SDL_GL_DestroyContext(glcontext);
		glcontext = NULL;
	}

	if (window)
	{
		SDL_DestroyWindow(window);
	}

	if (gl3_core)
	{
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
		info_output << "Request OpenGL 3.3 Core Profile context." << std::endl;
	}
	else
	{
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, 0);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 0);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
		info_output << "Fall back to default OpenGL context." << std::endl;
	}
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, depth_bpp);
	if (antialiasing > 1)
	{
		fsaa = antialiasing;
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, fsaa);
		info_output << "Enabling antialiasing: " << fsaa << "X" << std::endl;
	}
	else
	{
		fsaa = 1;
		info_output << "Disabling antialiasing" << std::endl;
	}

	// Create a new window
	const char * title = "VDRIFT";
	Uint32 window_flags = SDL_WINDOW_OPENGL;
	if (fullscreen)
		window_flags |= SDL_WINDOW_FULLSCREEN;

	window = SDL_CreateWindow(title, width, height, window_flags);
	if (!window)
	{
		error_output << SDL_GetError() << std::endl;
		assert(0);
		return;
	}

	glcontext = SDL_GL_CreateContext(window);
	if (!glcontext)
	{
		error_output << SDL_GetError() << std::endl;
		SDL_ClearError();
		return;
	}

	if (SDL_GL_MakeCurrent(window, glcontext) == false)
	{
		error_output << SDL_GetError() << std::endl;
		assert(0);
		return;
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
