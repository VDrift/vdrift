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

#ifndef _SIM_VEHICLEINFO_H
#define _SIM_VEHICLEINFO_H

#include "fracturebody.h"
#include "aerodevice.h"
#include "differential.h"
#include "antirollbar.h"
#include "wheel.h"
#include "transmission.h"
#include "clutch.h"
#include "engine.h"

namespace sim
{

struct VehicleInfo
{
	// has to contain at least wheels + body, first n bodies are the wheels
	// motion states are for: vehicle body + n wheels + m children bodies
	// minimum number of wheels is 2
	FractureBodyInfo body;
	btAlignedObjectArray<MotionState*> motionstate;
	btAlignedObjectArray<AeroDeviceInfo> aerodevice;
	btAlignedObjectArray<DifferentialInfo> differential;
	btAlignedObjectArray<AntiRollBar> antiroll;
	btAlignedObjectArray<WheelInfo> wheel;
	TransmissionInfo transmission;
	ClutchInfo clutch;
	EngineInfo engine;

	// driveline link targets are n wheels + m differentials
	// 0 <= shaft id < wheel.size() + differential.size()
	// the link graph has to be without cycles (tree)
	// differential link count is equal to differential count
	btAlignedObjectArray<int> differential_link_a;
	btAlignedObjectArray<int> differential_link_b;
	int transmission_link;

	VehicleInfo() : body(motionstate) {}
};

}

#endif // _SIM_VEHICLEINFO_H
