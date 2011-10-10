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

#ifndef _CARAERO_H
#define _CARAERO_H

#include "LinearMath/btVector3.h"
#include "coordinatesystem.h"
#include "joeserialize.h"
#include "macros.h"

#include <ostream>

class CARAERO
{
	friend class joeserialize::Serializer;
public:
	// Default constructor makes an aerodynamically transparent device (i.e. no drag or lift).
	CARAERO();

	void DebugPrint(std::ostream & out) const;

	void Set(const btVector3 & newpos, btScalar new_drag_frontal_area, btScalar new_drag_coefficient, btScalar new_lift_surface_area, btScalar new_lift_coefficient, btScalar new_lift_efficiency);

	const btVector3 & GetPosition() const;

	btVector3 GetForce(const btVector3 & bodyspace_wind_vector) const;

	btScalar GetAerodynamicDownforceCoefficient() const;

	btScalar GetAeordynamicDragCoefficient() const;

	btVector3 GetLiftVector() const;

	btVector3 GetDragVector() const;

private:
	// Constants (not actually declared as const because they can be changed after object creation).
	/// The current air density in kg/m^3.
	btScalar air_density;

	/// The projected frontal area in m^2, used for drag calculations.
	btScalar drag_frontal_area;

	/// The drag coefficient, a unitless measure of aerodynamic drag.
	btScalar drag_coefficient;

	/// The wing surface area in m^2.
	btScalar lift_surface_area;

	/// A unitless lift coefficient.
	btScalar lift_coefficient;

	/// The efficiency of the wing, a unitless value from 0.0 to 1.0.
	btScalar lift_efficiency;

	/// The position that the drag and lift forces are applied on the body.
	btVector3 position;

	// For info only.
	mutable btVector3 lift_vector;
	mutable btVector3 drag_vector;
};

#endif
