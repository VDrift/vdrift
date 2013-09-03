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

#ifndef SOUNDFILTER_H
#define SOUNDFILTER_H

#define MAX_FILTER_ORDER 5

class SoundFilter
{
public:
	SoundFilter();

	// reset filter
	void ClearState();

	// the coefficients are arrays like xcoeff[neworder+1], note ycoeff[0] is ignored
	void SetFilter(const int neworder, const float * xcoeff, const float * ycoeff);

	// apply filter
	void Filter(int * chan1, int * chan2, const int len);

	// yc0 is ignored
	void SetFilterOrder1(float xc0, float xc1, float yc1);

	// yc0 is ignored
	void SetFilterOrder0(float xc0);

private:
	int order;
	float xc[MAX_FILTER_ORDER+1];
	float yc[MAX_FILTER_ORDER+1];
	float statex[2][MAX_FILTER_ORDER+1];
	float statey[2][MAX_FILTER_ORDER+1];
};

#endif // SOUNDFILTER_H
