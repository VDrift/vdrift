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

#ifndef _RENDERDIMENSIONS
#define _RENDERDIMENSIONS

#include <iosfwd>

class RenderDimensions
{
public:
	// Constructors.
	RenderDimensions();
	RenderDimensions(float w, float h);
	RenderDimensions(float w, float h, bool mult);

	/// Given the viewport size defined by w and h, put the calculated dimensions into outw and outh.
	/// Returns true if the calculated width and height have changed since the last call.
	bool get(unsigned int w, unsigned int h, unsigned int & outw, unsigned int & outh);

private:
	unsigned int width, height;
	float origw, origh;
	bool origMultiples;

	friend std::ostream & operator<<(std::ostream & out, const RenderDimensions & dim);
};

#endif
