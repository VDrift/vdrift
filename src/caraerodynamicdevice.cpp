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

#include "caraerodynamicdevice.h"

CARAERO::CARAERO() : air_density(1.2), drag_frontal_area(0), drag_coefficient(0), lift_surface_area(0), lift_coefficient(0), lift_efficiency(0)
{
	// Constructor.
}

void CARAERO::DebugPrint(std::ostream & out) const
{
	out << "---Aerodynamic Device---" << "\n";
	out << "Drag: " << drag_vector[0] << ", " << drag_vector[1] << ", " << drag_vector[2] << "\n";
	out << "Lift: " << lift_vector[0] << ", " << lift_vector[1] << ", " << lift_vector[2] << "\n";
}

void CARAERO::Set(const btVector3 & newpos, btScalar new_drag_frontal_area, btScalar new_drag_coefficient, btScalar new_lift_surface_area, btScalar new_lift_coefficient, btScalar new_lift_efficiency)
{
	position = newpos;
	drag_frontal_area = new_drag_frontal_area;
	drag_coefficient = new_drag_coefficient;
	lift_surface_area = new_lift_surface_area;
	lift_coefficient = new_lift_coefficient;
	lift_efficiency = new_lift_efficiency;
}

const btVector3 & CARAERO::GetPosition() const
{
	return position;
}

btVector3 CARAERO::GetForce(const btVector3 & bodyspace_wind_vector) const
{
	// Calculate drag force.
	drag_vector = bodyspace_wind_vector * bodyspace_wind_vector.length() * 0.5 * air_density * drag_coefficient * drag_frontal_area;

	// Positive wind speed when the wind is heading at us.
	btScalar wind_speed = -direction::forward.dot(bodyspace_wind_vector);

	// Assume the surface doesn't generate much lift when in reverse.
	if(wind_speed < 0)
		wind_speed = -wind_speed * 0.2;

	// Calculate lift force and associated drag.
	const btScalar k = 0.5 * air_density * wind_speed * wind_speed;
	const btScalar lift = k * lift_coefficient * lift_surface_area;
	const btScalar drag = -lift_coefficient * lift * (1.0 -  lift_efficiency);
	lift_vector = direction::forward * drag + direction::up * lift;

	btVector3 force = drag_vector + lift_vector;

	return force;
}

btScalar CARAERO::GetAerodynamicDownforceCoefficient() const
{
	return 0.5 * air_density * lift_coefficient * lift_surface_area;
}

btScalar CARAERO::GetAeordynamicDragCoefficient() const
{
	return 0.5 * air_density * (drag_coefficient * drag_frontal_area + lift_coefficient * lift_coefficient * lift_surface_area * (1.0-lift_efficiency));
}

btVector3 CARAERO::GetLiftVector() const
{
	return lift_vector;
}

btVector3 CARAERO::GetDragVector() const
{
	return drag_vector;
}
