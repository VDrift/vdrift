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

#ifndef _CARSOUND_H
#define _CARSOUND_H

#include "physics/carwheelposition.h"
#include "crashdetection.h"
#include "enginesoundinfo.h"

#include <iosfwd>
#include <string>
#include <vector>

class Sound;
class CarDynamics;
class ContentManager;

class CarSound
{
public:
	CarSound();

	CarSound(const CarSound & other);

	CarSound & operator= (const CarSound & other);

	~CarSound();

	bool Load(
		const std::string & carpath,
		const std::string & carname,
		Sound & sound,
		ContentManager & content,
		std::ostream & error_output);

	void Update(const CarDynamics & dynamics, float dt);

	void EnableInteriorSound(bool value);

private:
	CrashDetection crashdetection;
	std::vector<EngineSoundInfo> enginesounds;
	unsigned tiresqueal[WHEEL_POSITION_SIZE];
	unsigned tirebump[WHEEL_POSITION_SIZE];
	unsigned grasssound[WHEEL_POSITION_SIZE];
	unsigned gravelsound[WHEEL_POSITION_SIZE];
	unsigned crashsound;
	unsigned gearsound;
	unsigned brakesound;
	unsigned handbrakesound;
	unsigned roadnoise;
	Sound * psound;

	int gearsound_check;
	bool brakesound_check;
	bool handbrakesound_check;
	bool interior;

	void Clear();
};

#endif // _CARSOUND_H

