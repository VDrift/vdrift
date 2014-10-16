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

#ifndef _WINDOW_H
#define _WINDOW_H

#include <iosfwd>
#include <string>

struct SDL_Window;

class Window
{
public:
	Window();

	~Window();

	void Init(
		const std::string & caption,
		int resx,
		int resy,
		int depth_bpp,
		int antialiasing,
		bool fullscreen,
		bool vsync,
		std::ostream & info_output,
		std::ostream & error_output);

	void SwapBuffers();

	/// Note that when the mouse cursor is hidden, it is also grabbed (confined to the application window)
	void ShowMouseCursor(bool value);

	void Screenshot(const std::string & filename);

	bool Initialized() const;

	int GetW() const;

	int GetH() const;

private:
	SDL_Window * window;
	void * glcontext;
	int fsaa;
	int w, h;
	bool initialized;

	bool ResizeWindow(int width, int height);

	void ChangeDisplay(
		int width,
		int height,
		int depth_bpp,
		int antialiasing,
		bool fullscreen,
		bool vsync,
		bool gl3_core,
		std::ostream & info_output,
		std::ostream & error_output);

	void LogOpenGLInfo(std::ostream & info_output);
};

#endif

