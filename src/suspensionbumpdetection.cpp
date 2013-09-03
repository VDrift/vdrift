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

#include "suspensionbumpdetection.h"

SuspensionBumpDetection::SuspensionBumpDetection() :
	state(SETTLED),
	laststate(SETTLED),
	displacetime(0.01),
	displacevelocitythreshold(0.5),
	settletime(0.01),
	settlevelocitythreshold(0.0),
	displacetimer(0),
	settletimer(0),
	dpstart(0),
	dpend(0)
{

}

void SuspensionBumpDetection::Update(float vel, float displacementpercent, float dt)
{
	laststate = state;

	//switch states based on velocity
	if (state == SETTLED)
	{
		if (vel >= displacevelocitythreshold)
		{
			state = DISPLACING;
			displacetimer = displacetime;
			dpstart = displacementpercent;
		}
	}
	else if (state == DISPLACING)
	{
		if (vel < displacevelocitythreshold)
		{
			state = SETTLED;
		}
	}
	else if (state == DISPLACED)
	{
		if (vel <= settlevelocitythreshold)
		{
			state = SETTLING;
		}
	}
	else if (state == SETTLING)
	{
		//if (std::abs(vel) > settlevelocitythreshold)
		if (vel > settlevelocitythreshold)
		{
			state = DISPLACED;
		}
	}

	//switch states based on time
	if (state == DISPLACING)
	{
		displacetimer -= dt;
		if (displacetimer <= 0)
		{
			state = DISPLACED;
			settletimer = settletime;
		}
	}
	else if (state == SETTLING)
	{
		settletimer -= dt;
		if (settletimer <= 0)
		{
			state = SETTLED;
			dpend = displacementpercent;
		}
	}
}
