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

#include <string>
#include <ostream>

struct SDL_Surface;

class WINDOW_SDL
{
public:
	WINDOW_SDL();
	~WINDOW_SDL();

	void Init(const std::string & windowcaption, unsigned int resx, unsigned int resy, unsigned int bpp, unsigned int depthbpp, bool fullscreen, unsigned int antialiasing, std::ostream & info_output, std::ostream & error_output);
	void SwapBuffers();
	void Screenshot(std::string filename);
	unsigned int GetW() const;
	unsigned int GetH() const;
	float GetWHRatio() const;

private:
	// Configuration variables, internal data.
	unsigned int w, h;
	SDL_Surface * surface;
	bool initialized;
	unsigned int fsaa;

	void ChangeDisplay(const int width, const int height, const int bpp, const int dbpp, const bool fullscreen, unsigned int antialiasing, std::ostream & info_output, std::ostream & error_output);
};

#endif

