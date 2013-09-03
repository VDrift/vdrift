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

#ifndef _SUSPENSIONBUMPDETECTION_H
#define _SUSPENSIONBUMPDETECTION_H

class CAR;

class SuspensionBumpDetection
{
public:
	SuspensionBumpDetection();

	void Update(float vel, float displacementpercent, float dt);

	bool JustDisplaced() const
	{
		return (state == DISPLACED && laststate != DISPLACED);
	}

	bool JustSettled() const
	{
		return (state == SETTLED && laststate != SETTLED);
	}

	float GetTotalBumpSize() const
	{
		return dpend - dpstart;
	}

private:
	friend class CAR;
	enum
	{
		DISPLACING,
		DISPLACED,
		SETTLING,
		SETTLED
	} state, laststate;

	const float displacetime; ///< how long the suspension has to be displacing a high velocity, uninterrupted
	const float displacevelocitythreshold; ///< the threshold for high velocity
	const float settletime; ///< how long the suspension has to be settled, uninterrupted
	const float settlevelocitythreshold;

	float displacetimer;
	float settletimer;
	float dpstart, dpend;
};
#endif // _SUSPENSIONBUMPDETECTION_H
