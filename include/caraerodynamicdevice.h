#ifndef _CARAERO_H
#define _CARAERO_H

#include "LinearMath/btVector3.h"
#include "joeserialize.h"
#include "macros.h"
#include <iostream>

class CARAERO
{
	friend class joeserialize::Serializer;
	private:
		//constants (not actually declared as const because they can be changed after object creation)
		btScalar air_density; ///the current air density in kg/m^3
		btScalar drag_frontal_area; ///the projected frontal area in m^2, used for drag calculations
		btScalar drag_coefficient; ///the drag coefficient, a unitless measure of aerodynamic drag
		btScalar lift_surface_area; ///the wing surface area in m^2
		btScalar lift_coefficient; ///a unitless lift coefficient
		btScalar lift_efficiency; ///the efficiency of the wing, a unitless value from 0.0 to 1.0
		btVector3 position; ///the position that the drag and lift forces are applied on the body
		
		//for info only
		mutable btVector3 lift_vector;
		mutable btVector3 drag_vector;
		
	public:
		//default constructor makes an aerodynamically transparent device (i.e. no drag or lift)
		CARAERO() :
			air_density(1.2),
			drag_frontal_area(0),
			drag_coefficient(0),
			lift_surface_area(0),
			lift_coefficient(0),
			lift_efficiency(0)
		{
			// ctor
		}

		void DebugPrint(std::ostream & out) const
		{
			out << "---Aerodynamic Device---" << "\n";
			out << "Drag: " << drag_vector[0] << ", " << drag_vector[1] << ", " << drag_vector[2] << "\n";
			out << "Lift: " << lift_vector[0] << ", " << lift_vector[1] << ", " << lift_vector[2] << "\n";
		}
		
		void Set(
			const btVector3 & newpos,
			btScalar new_drag_frontal_area,
			btScalar new_drag_coefficient,
			btScalar new_lift_surface_area,
			btScalar new_lift_coefficient,
			btScalar new_lift_efficiency)
		{
			position = newpos;
			drag_frontal_area = new_drag_frontal_area;
			drag_coefficient = new_drag_coefficient;
			lift_surface_area = new_lift_surface_area;
 			lift_coefficient = new_lift_coefficient;
			lift_efficiency = new_lift_efficiency;
		}

		const btVector3 & GetPosition() const
		{
			return position;
		}
	
		btVector3 GetForce(const btVector3 & bodyspace_wind_vector) const
		{
			//calculate drag force
			drag_vector = bodyspace_wind_vector * bodyspace_wind_vector.length() * 
				0.5 * air_density * drag_coefficient * drag_frontal_area;
			
			//calculate lift force and associated drag
			btScalar wind_speed = -bodyspace_wind_vector[0]; //positive wind speed when the wind is heading at us
			if (wind_speed < 0) wind_speed = -wind_speed * 0.2; //assume the surface doesn't generate much lift when in reverse
			const btScalar k = 0.5 * air_density * wind_speed * wind_speed;
			const btScalar lift = k * lift_coefficient * lift_surface_area;
			const btScalar drag = -lift_coefficient * lift * (1.0 -  lift_efficiency);
			lift_vector = btVector3 (drag, 0, lift);
			
			btVector3 force = drag_vector + lift_vector;
			
			return force;
		}
		
		btScalar GetAerodynamicDownforceCoefficient() const
		{
			return 0.5 * air_density * lift_coefficient * lift_surface_area;
		}
		
		btScalar GetAeordynamicDragCoefficient() const
		{
			return 0.5 * air_density * (drag_coefficient * drag_frontal_area + lift_coefficient * lift_coefficient * lift_surface_area * (1.0-lift_efficiency));
		}
		
		bool Serialize(joeserialize::Serializer & s)
		{
			return true;
		}

		btVector3 GetLiftVector() const
		{
			return lift_vector;
		}

		btVector3 GetDragVector() const
		{
			return drag_vector;
		}
};

#endif
