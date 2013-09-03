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

#include <cassert>
#include "crashdetection.h"

CrashDetection::CrashDetection() : lastvel(0), curmaxdecel(0), maxdecel(0), deceltrigger(200)
{
    // Constructor.
}

void CrashDetection::Update(float vel, float dt)
{
	maxdecel = 0;

	float decel = (lastvel - vel)/dt;

	if (decel > deceltrigger && curmaxdecel == 0)
		// Idle, start capturing decel.
		curmaxdecel = decel;
	else if (curmaxdecel > 0)
	{
		// Currently capturing, check for max.
		if (decel > curmaxdecel)
			curmaxdecel = decel;
		else
		{
			maxdecel = curmaxdecel;
			assert(maxdecel > deceltrigger);
			curmaxdecel = 0;
		}
	}

	lastvel = vel;
}

float CrashDetection::GetMaxDecel() const
{
    return maxdecel;
}
