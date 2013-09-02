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

#ifndef _SIGNALPROCESSING_H
#define _SIGNALPROCESSING_H

#include <vector>
#include <cassert>

namespace signalprocessing
{

class DELAY
{
	private:
		std::vector <float> oldstate;
		int index;

	public:
		DELAY(int newdelayticks)
		{
			oldstate.resize(newdelayticks+1);
			Clear(0.0);
		}

		void Clear(float clearvalue)
		{
			for (int i = 0; i < (int)oldstate.size(); i++)
				oldstate[i] = clearvalue;

			index = 0;
		}

		float Process(float input)
		{
			assert (index < (int)oldstate.size());

			oldstate[index] = input;
			index++;
			if (index >= (int)oldstate.size())
				index = 0;
			return oldstate[index];
		}
};

class LOWPASS
{
	private:
		float lastout;
		float coeff;

	public:
		LOWPASS(float newcoeff) : coeff(newcoeff) {}

		float Process(float input)
		{
			lastout = input*coeff + lastout*(1.0f-coeff);
			return lastout;
		}
};

class PID
{
	private:
		float dState; // Last position input
		float iState; // Integrator state
		float iMax, iMin; // Maximum and minimum allowable integrator state
		float	pGain, // integral gain
			iGain, // proportional gain
			dGain; // derivative gain
		bool limiting;

	public:
		PID(float p, float i, float d, bool newlimiting) : dState(0), iState(0), iMax(1), iMin(-1),
			pGain(p),iGain(i),dGain(d),limiting(newlimiting) {}

		float Process(float error, float position)
		{
			float pTerm, dTerm, iTerm;
			pTerm = pGain * error;
			// calculate the proportional term
			// calculate the integral state with appropriate limiting
			iState += error;
			if (limiting)
			{
				if (iState > iMax) iState = iMax;
				else if (iState < iMin) iState = iMin;
			}
			iTerm = iGain * iState;  // calculate the integral term
			dTerm = dGain * (position - dState);
			dState = position;
			return pTerm + iTerm - dTerm;
		}

		void SetState(float init)
		{
			iState = init;
		}
};

}

#endif
