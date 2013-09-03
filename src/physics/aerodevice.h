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

#ifndef _AERODEVICE_H
#define _AERODEVICE_H

#include "LinearMath/btVector3.h"
#include "coordinatesystem.h"

struct AeroDeviceInfo
{
	btScalar air_density; 		/// air density in kg/m^3
	btScalar drag_frontal_area; /// the projected frontal area in m^2, used for drag calculations
	btScalar drag_coefficient; 	/// the drag coefficient, a unitless measure of aerodynamic drag
	btScalar lift_surface_area; /// the wing surface area in m^2
	btScalar lift_coefficient; 	/// a unitless lift coefficient
	btScalar lift_efficiency; 	/// the efficiency of the wing, a unitless value from 0.0 to 1.0
	btVector3 position; 		/// the position that the drag and lift forces are applied on the body
	void* user_ptr;				/// user supplied pointer

	AeroDeviceInfo() :
		air_density(1.2),
		drag_frontal_area(0),
		drag_coefficient(0),
		lift_surface_area(0),
		lift_coefficient(0),
		lift_efficiency(0),
		position(0, 0, 0),
		user_ptr(0)
	{
		// ctor
	}
};

class AeroDevice
{
public:
	const btVector3 & getPosition() const
	{
		return position;
	}

	const btVector3 & getLift() const
	{
		return lift_vector;
	}

	const btVector3 & getDrag() const
	{
		return drag_vector;
	}

	btScalar getLiftCoefficient() const
	{
		return lift_coefficient;
	}

	btScalar getDragCoefficient() const
	{
		return drag_coefficient + induced_drag_coefficient;
	}

	btVector3 getForce(const btVector3 & bodyspace_wind_vector)
	{
		//calculate drag force
		drag_vector = drag_coefficient * bodyspace_wind_vector.length() * bodyspace_wind_vector;

		//positive wind speed when the wind is heading at us
		btScalar wind_speed = -Direction::forward.dot(bodyspace_wind_vector);

		//assume the surface doesn't generate much lift when in reverse
		if (wind_speed < 0) wind_speed = -wind_speed * 0.2;

		//calculate lift force and associated drag
		lift_vector = (Direction::up * lift_coefficient -
						Direction::forward * induced_drag_coefficient) * wind_speed * wind_speed;

		btVector3 force = drag_vector + lift_vector;

		return force;
	}

	void* GetUserPointer() const
	{
		return user_ptr;
	}


	AeroDevice() :
		lift_coefficient(0),
		drag_coefficient(0),
		induced_drag_coefficient(0),
		position(0, 0, 0),
		user_ptr(0)
	{
		// ctor
	}

	AeroDevice(const AeroDeviceInfo & info)
	{
		// elliptic load wing: 1 / (pi * AR * e) = 1 - lift_efficiency
		lift_coefficient = 0.5 * info.air_density * info.lift_coefficient * info.lift_surface_area;
		induced_drag_coefficient = lift_coefficient * info.lift_coefficient * (1.0 - info.lift_efficiency);
		drag_coefficient = 0.5 * info.air_density * info.drag_coefficient * info.drag_frontal_area;
		position = info.position;
		user_ptr = info.user_ptr;
	}

private:
	btScalar lift_coefficient;
	btScalar drag_coefficient;
	btScalar induced_drag_coefficient;
	btVector3 position;
	btVector3 lift_vector;
	btVector3 drag_vector;
	void* user_ptr;
};

#endif
