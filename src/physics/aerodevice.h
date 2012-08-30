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

#ifndef _SIM_AERODEVICE_H
#define _SIM_AERODEVICE_H

#include "LinearMath/btVector3.h"
#include "coordinatesystem.h"

namespace sim
{

struct AeroDeviceInfo
{
	btScalar air_density; 		///< air density in kg/m^3
	btScalar drag_frontal_area; ///< the projected frontal area in m^2, used for drag calculations
	btScalar drag_coefficient; 	///< the drag coefficient, a unitless measure of aerodynamic drag
	btScalar lift_surface_area; ///< the wing surface area in m^2
	btScalar lift_coefficient; 	///< a unitless lift coefficient
	btScalar lift_efficiency; 	///< the efficiency of the wing, a unitless value from 0.0 to 1.0
	btVector3 position; 		///< the position that the drag and lift forces are applied on the body
	AeroDeviceInfo();			///< default constructor (no lift/drag)
};

class AeroDevice
{
public:
	/// relative position
	const btVector3 & getPosition() const;

	/// lift vector in local space
	const btVector3 & getLift() const;

	/// drag vector in local space
	const btVector3 & getDrag() const;

	/// lift coefficient contant
	btScalar getLiftCoefficient() const;

	/// drag coeficient constant
	btScalar getDragCoefficient() const;

	/// get lift and drag sum in local space
	btVector3 getForce(const btVector3 & bodyspace_wind_vector);

	AeroDevice();

	AeroDevice(const AeroDeviceInfo & info);

private:
	btScalar lift_coefficient;
	btScalar drag_coefficient;
	btScalar induced_drag_coefficient;
	btVector3 position;
	btVector3 lift_vector;
	btVector3 drag_vector;
};

// implementation

inline AeroDeviceInfo::AeroDeviceInfo() :
	air_density(1.2),
	drag_frontal_area(0),
	drag_coefficient(0),
	lift_surface_area(0),
	lift_coefficient(0),
	lift_efficiency(0),
	position(0, 0, 0)
{
	// ctor
}

inline const btVector3 & AeroDevice::getPosition() const
{
	return position;
}

inline const btVector3 & AeroDevice::getLift() const
{
	return lift_vector;
}

inline const btVector3 & AeroDevice::getDrag() const
{
	return drag_vector;
}

inline btScalar AeroDevice::getLiftCoefficient() const
{
	return lift_coefficient;
}

inline btScalar AeroDevice::getDragCoefficient() const
{
	return drag_coefficient + induced_drag_coefficient;
}

inline btVector3 AeroDevice::getForce(const btVector3 & bodyspace_wind_vector)
{
	//calculate drag force
	drag_vector = drag_coefficient * bodyspace_wind_vector.length() * bodyspace_wind_vector;

	//positive wind speed when the wind is heading at us
	btScalar wind_speed = -direction::forward.dot(bodyspace_wind_vector);

	//assume the surface doesn't generate much lift when in reverse
	if (wind_speed < 0) wind_speed = -wind_speed * 0.2;

	//calculate lift force and associated drag
	lift_vector = (direction::up * lift_coefficient -
					direction::forward * induced_drag_coefficient) * wind_speed * wind_speed;

	btVector3 force = drag_vector + lift_vector;

	return force;
}

inline AeroDevice::AeroDevice() :
	lift_coefficient(0),
	drag_coefficient(0),
	induced_drag_coefficient(0),
	position(0, 0, 0)
{
	// ctor
}

inline AeroDevice::AeroDevice(const AeroDeviceInfo & info)
{
	// elliptic load wing: 1 / (pi * AR * e) = 1 - lift_efficiency
	lift_coefficient = 0.5 * info.air_density * info.lift_coefficient * info.lift_surface_area;
	induced_drag_coefficient = lift_coefficient * info.lift_coefficient * (1.0 - info.lift_efficiency);
	drag_coefficient = 0.5 * info.air_density * info.drag_coefficient * info.drag_frontal_area;
	position = info.position;
}

}

#endif
