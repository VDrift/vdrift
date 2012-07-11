#include "soundfilter.h"

#include <cassert>

SOUNDFILTER::SOUNDFILTER() :
	order(0)
{
	ClearState();
}

void SOUNDFILTER::ClearState()
{
	for (int c = 0; c < 2; ++c)
	{
		for (int i = 0; i < MAX_FILTER_ORDER; ++i)
		{
			statex[c][i] = 0.0;
			statey[c][i] = 0.0;
		}
	}
}

void SOUNDFILTER::SetFilter(const int neworder, const float * xcoeff, const float * ycoeff)
{
	assert(!(neworder > MAX_FILTER_ORDER || neworder < 0));

	order = neworder;
	for (int i = 0; i < neworder+1; i++)
	{
		xc[i] = xcoeff[i];
		yc[i] = ycoeff[i];
	}
}

void SOUNDFILTER::Filter(int * chan1, int * chan2, const int len)
{
	for (int i = 0; i < len; i++)
	{
		// store old state
		for (int s = 1; s <= order; s++)
		{
			statex[0][s] = statex[0][s-1];
			statex[1][s] = statex[1][s-1];

			statey[0][s] = statey[0][s-1];
			statey[1][s] = statey[1][s-1];
		}

		// set the sample state for now to the current input
		statex[0][0] = chan1[i];
		statex[1][0] = chan2[i];

		switch (order)
		{
			case 1:
				chan1[i] = (int) (statex[0][0]*xc[0]+statex[0][1]*xc[1]+statey[0][1]*yc[1]);
				chan2[i] = (int) (statex[1][0]*xc[0]+statex[1][1]*xc[1]+statey[1][1]*yc[1]);
				break;
			case 0:
				chan1[i] = (int) (statex[0][0]*xc[0]);
				chan2[i] = (int) (statex[1][0]*xc[0]);
				break;
			default:
				break;
		}

		// store the state of the output
		statey[0][0] = chan1[i];
		statey[1][0] = chan2[i];
	}
}

void SOUNDFILTER::SetFilterOrder1(float xc0, float xc1, float yc1)
{
	order = 1;
	xc[0] = xc0;
	xc[1] = xc1;
	yc[1] = yc1;
}

void SOUNDFILTER::SetFilterOrder0(float xc0)
{
	order = 0;
	xc[0] = xc0;
}
