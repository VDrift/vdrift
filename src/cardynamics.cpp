#include "cardynamics.h"

//#define SUSPENSION_FORCE_DIRECTION

typedef CARDYNAMICS::T T;

MATHVECTOR <T, 3> CARDYNAMICS::GetGravityForce()
{
	MATHVECTOR <T, 3> g;
	g.Set ( 0,0,-9.81 );
	g = g * body.GetMass();
	return g;
}

///use the wheel speed to calculate the driveshaft speed
T CARDYNAMICS::CalculateDriveshaftSpeed()
{
	T driveshaft_speed = 0.0;
	T left_front_wheel_speed = wheel[FRONT_LEFT].GetAngularVelocity();
	T right_front_wheel_speed = wheel[FRONT_RIGHT].GetAngularVelocity();
	T left_rear_wheel_speed = wheel[REAR_LEFT].GetAngularVelocity();
	T right_rear_wheel_speed = wheel[REAR_RIGHT].GetAngularVelocity();
	for ( int i = 0; i < 4; i++ ) assert ( !isnan ( wheel[WHEEL_POSITION ( i ) ].GetAngularVelocity() ) );
	if ( drive == RWD )
	{
		driveshaft_speed = rear_differential.CalculateDriveshaftSpeed ( left_rear_wheel_speed, right_rear_wheel_speed );
	}
	else if ( drive == FWD )
	{
		driveshaft_speed = front_differential.CalculateDriveshaftSpeed ( left_front_wheel_speed, right_front_wheel_speed );
	}
	else if ( drive == AWD )
	{
		driveshaft_speed = center_differential.CalculateDriveshaftSpeed (
		                       front_differential.CalculateDriveshaftSpeed ( left_front_wheel_speed, right_front_wheel_speed ),
		                       rear_differential.CalculateDriveshaftSpeed ( left_rear_wheel_speed, right_rear_wheel_speed ) );
	}

	return driveshaft_speed;
}

T CARDYNAMICS::GetDriveshaftSpeed() const
{
	T driveshaft_speed = 0.0;
	T left_front_wheel_speed = wheel[FRONT_LEFT].GetAngularVelocity();
	T right_front_wheel_speed = wheel[FRONT_RIGHT].GetAngularVelocity();
	T left_rear_wheel_speed = wheel[REAR_LEFT].GetAngularVelocity();
	T right_rear_wheel_speed = wheel[REAR_RIGHT].GetAngularVelocity();
	for ( int i = 0; i < 4; i++ ) assert ( !isnan ( wheel[WHEEL_POSITION ( i ) ].GetAngularVelocity() ) );
	if ( drive == RWD )
	{
		driveshaft_speed = rear_differential.GetDriveshaftSpeed ( left_rear_wheel_speed, right_rear_wheel_speed );
	}
	else if ( drive == FWD )
	{
		driveshaft_speed = front_differential.GetDriveshaftSpeed ( left_front_wheel_speed, right_front_wheel_speed );
	}
	else if ( drive == AWD )
	{
		driveshaft_speed = center_differential.GetDriveshaftSpeed (
				front_differential.GetDriveshaftSpeed ( left_front_wheel_speed, right_front_wheel_speed ),
				rear_differential.GetDriveshaftSpeed ( left_rear_wheel_speed, right_rear_wheel_speed ) );
	}

	return driveshaft_speed;
}

///apply forces on the engine due to drag from the clutch
void CARDYNAMICS::ApplyClutchTorque ( T engine_drag, T clutch_speed )
{
	if ( transmission.GetGear() == 0 )
	{
		engine.SetClutchTorque ( 0.0 );
	}
	else
	{
		engine.SetClutchTorque ( engine_drag );
	}
}

///calculate the drive torque that the engine applies to each wheel, and put the output into the supplied 4-element array
void CARDYNAMICS::CalculateDriveTorque ( T * wheel_drive_torque, T clutch_torque )
{
	T driveshaft_torque = transmission.GetTorque ( clutch_torque );
	assert ( !isnan ( driveshaft_torque ) );

	for ( int i = 0; i < WHEEL_POSITION_SIZE; i++ )
		wheel_drive_torque[i] = 0;

	if ( drive == RWD )
	{
		rear_differential.ComputeWheelTorques ( driveshaft_torque );
		wheel_drive_torque[REAR_LEFT] = rear_differential.GetSide1Torque();
		wheel_drive_torque[REAR_RIGHT] = rear_differential.GetSide2Torque();
	}
	else if ( drive == FWD )
	{
		front_differential.ComputeWheelTorques ( driveshaft_torque );
		wheel_drive_torque[FRONT_LEFT] = front_differential.GetSide1Torque();
		wheel_drive_torque[FRONT_RIGHT] = front_differential.GetSide2Torque();
	}
	else if ( drive == AWD )
	{
		center_differential.ComputeWheelTorques ( driveshaft_torque );
		front_differential.ComputeWheelTorques ( center_differential.GetSide1Torque() );
		rear_differential.ComputeWheelTorques ( center_differential.GetSide2Torque() );
		wheel_drive_torque[FRONT_LEFT] = front_differential.GetSide1Torque();
		wheel_drive_torque[FRONT_RIGHT] = front_differential.GetSide2Torque();
		wheel_drive_torque[REAR_LEFT] = rear_differential.GetSide1Torque();
		wheel_drive_torque[REAR_RIGHT] = rear_differential.GetSide2Torque();
	}

	for ( int i = 0; i < WHEEL_POSITION_SIZE; i++ ) assert ( !isnan ( wheel_drive_torque[WHEEL_POSITION ( i ) ] ) );
}

void CARDYNAMICS::ApplyEngineTorqueToBody ( MATHVECTOR <T, 3> & total_force, MATHVECTOR <T, 3> & total_torque )
{
	MATHVECTOR <T, 3> engine_torque;
	engine_torque.Set ( -engine.GetTorque(), 0, 0 );
	assert ( !isnan ( engine_torque[0] ) );
	body.GetOrientation().RotateVector ( engine_torque );
	MATHVECTOR <T, 3> zero ( 0 );
	MATHVECTOR <T, 3> engine_pos = CarLocalToWorld ( engine.GetPosition() ) - GetCenterOfMassPosition();
	//MATHVECTOR <T, 3> engine_pos = CarLocalToRigidBodyLocal(engine.GetPosition());
	body.GetForceAndTorqueAtOffset ( zero, engine_torque, engine_pos, total_force, total_torque );
	for ( int i = 0; i < 3; i++ ) assert ( !isnan ( total_force[i] ) );
	for ( int i = 0; i < 3; i++ ) assert ( !isnan ( total_torque[i] ) );
}

void CARDYNAMICS::ApplyAerodynamicsToBody ( MATHVECTOR <T, 3> & total_force, MATHVECTOR <T, 3> & total_torque )
{
	int count = 0;
	for ( std::vector <CARAERO <T> >::iterator i = aerodynamics.begin(); i != aerodynamics.end(); ++i )
	{
		MATHVECTOR <T, 3> wind_velocity = -body.GetVelocity();
		( -body.GetOrientation() ).RotateVector ( wind_velocity );
		MATHVECTOR <T, 3> wind_force = i->GetForce ( wind_velocity );
		MATHVECTOR <T, 3> aero_point = CarLocalToWorld ( i->GetPosition() ) - GetCenterOfMassPosition();
		body.GetOrientation().RotateVector ( wind_force );
		body.GetForceAtOffset ( wind_force, aero_point, total_force, total_torque );
		//std::cout << count << ". " << -body.GetVelocity() << "..." << wind_velocity << "..." << wind_force << std::endl;
		for ( int i = 0; i < 3; i++ ) assert ( !isnan ( total_force[i] ) );
		for ( int i = 0; i < 3; i++ ) assert ( !isnan ( total_torque[i] ) );
		count++;
	}

	//apply rotational damping/drag
	MATHVECTOR <T, 3> rotational_aero_drag = - body.GetAngularVelocity() * 1000.0f;
	total_torque = total_torque + rotational_aero_drag;
	//std::cout << rotational_aero_drag << " yields " << total_torque << std::endl;
}

void CARDYNAMICS::ComputeSuspensionDisplacement ( int i, T dt )
{
	//compute bump effect
	T posx = wheel_contacts[i].GetPosition() [0];
	T posz = wheel_contacts[i].GetPosition() [2];
	T phase = 2*3.141593* ( posx+posz ) / wheel_contacts[i].GetBumpWavelength();
	T shift = 2.0*sin ( phase*1.414214 );
	T amplitude = 0.25*wheel_contacts[i].GetBumpAmplitude();
	T bumpoffset = amplitude * ( sin ( phase + shift ) + sin ( 1.414214*phase ) - 2.0 );

	T wheelheight_above_ground = wheel_contacts[i].GetWheelheight() - tire[i].GetRadius() - bumpoffset;
	assert ( !isnan ( wheelheight_above_ground ) );
	assert ( !isnan ( suspension[WHEEL_POSITION ( i ) ].GetDisplacement() ) );
	suspension[WHEEL_POSITION ( i ) ].SetDisplacement ( suspension[WHEEL_POSITION ( i ) ].GetDisplacement()-wheelheight_above_ground, dt );
	assert ( !isnan ( suspension[WHEEL_POSITION ( i ) ].GetDisplacement() ) );
}

///returns the suspension force (so it can be applied to the tires)
MATHVECTOR <T, 3> CARDYNAMICS::ApplySuspensionForceToBody ( int i, T dt, MATHVECTOR <T, 3> & total_force, MATHVECTOR <T, 3> & total_torque )
{
	//compute suspension force
	T springdampforce = suspension[WHEEL_POSITION ( i ) ].GetForce ( dt );
	assert ( !isnan ( springdampforce ) );

	//correct for a bottomed suspension by moving the car body up.  this is sort of a kludge.
	T overtravel = suspension[WHEEL_POSITION ( i ) ].GetOvertravel();
	if ( overtravel > 0 )
	{
		MATHVECTOR <T, 3> worldup ( 0,0,1 );
		body.GetOrientation().RotateVector ( worldup );
		worldup[0] = 0;
		worldup[1] = 0;
		if ( worldup[2] < 0 )
			worldup[2] = 0;
		body.SetPosition ( body.GetPosition() + worldup*overtravel*0.25 );
	}

	//do anti-roll
	int otheri = i;
	if ( i == 0 || i == 2 )
		otheri++;
	else
		otheri--;
	T antirollforce = suspension[WHEEL_POSITION ( i ) ].GetAntiRollK() *
	                  ( suspension[WHEEL_POSITION ( i ) ].GetDisplacement()-
	                    suspension[WHEEL_POSITION ( otheri ) ].GetDisplacement() );
	suspension[WHEEL_POSITION(i)].SetAntiRollInfo(antirollforce);
	assert ( !isnan ( antirollforce ) );

	//find the vector direction to apply the suspension force
#ifdef SUSPENSION_FORCE_DIRECTION
	const MATHVECTOR <T, 3> & wheelext = wheel[i].GetExtendedPosition();
	const MATHVECTOR <T, 3> & hinge = suspension[i].GetHinge();
	MATHVECTOR <T, 3> relwheelext = wheelext - hinge;
	MATHVECTOR <T, 3> up ( 0,0,1 );
	MATHVECTOR <T, 3> rotaxis = up.cross ( relwheelext.Normalize() );
	MATHVECTOR <T, 3> forcedirection = relwheelext.Normalize().cross ( rotaxis );
	//std::cout << i << ". " << forcedirection << std::endl;
	MATHVECTOR <T, 3> suspension_force = forcedirection* ( antirollforce+springdampforce );
#else
	MATHVECTOR <T, 3> suspension_force = MATHVECTOR <T, 3> ( 0,0,1 ) * ( antirollforce+springdampforce );
#endif

	body.GetOrientation().RotateVector ( suspension_force ); //transform to world space
	//MATHVECTOR <T, 3> suspension_force_application_point = CarLocalToRigidBodyWorld(suspension[WHEEL_POSITION(i)].GetPosition());
#ifdef SUSPENSION_FORCE_DIRECTION
	MATHVECTOR <T, 3> suspension_force_application_point = GetWheelPosition ( WHEEL_POSITION ( i ) ) - GetCenterOfMassPosition();
#else
	//MATHVECTOR <T, 3> suspension_force_application_point = CarLocalToWorld(suspension[WHEEL_POSITION(i)].GetPosition()) - GetCenterOfMassPosition();
	MATHVECTOR <T, 3> suspension_force_application_point = GetWheelPosition ( WHEEL_POSITION ( i ) ) - GetCenterOfMassPosition();
#endif
	
	body.GetForceAtOffset ( suspension_force, suspension_force_application_point, total_force, total_torque );
	
	for ( int n = 0; n < 3; n++ ) assert ( !isnan ( total_force[n] ) );
	for ( int n = 0; n < 3; n++ ) assert ( !isnan ( total_torque[n] ) );

	return suspension_force;
}

MATHVECTOR <T, 3> CARDYNAMICS::ComputeTireFrictionForce ( int i, T dt, const MATHVECTOR <T, 3> & suspension_force,
        bool frictionlimiting, T wheelspeed, MATHVECTOR <T, 3> & groundvel,
        const QUATERNION <T> & wheel_orientation )
{
	MATHVECTOR <T, 3> wheel_normal;
	wheel_normal.Set ( 0,0,1 );
	wheel_orientation.RotateVector ( wheel_normal );
	for ( int n = 0; n < 3; n++ ) assert ( !isnan ( wheel_normal[n] ) );
#ifdef SUSPENSION_FORCE_DIRECTION
	//T normal_force = suspension_force.dot(wheel_normal);
	T normal_force = suspension_force.Magnitude();
#else
	T normal_force = suspension_force.Magnitude();
#endif
	assert ( !isnan ( normal_force ) );

	//determine camber relative to the road
	//the component of vector A projected onto plane B = A || B = B × (A×B / |B|) / |B|
	//plane B is the plane defined by using the tire's forward-facing vector as the plane's normal, in wheelspace
	//vector A is the normal of the driving surface, in wheelspace
	MATHVECTOR <T, 3> B ( 1,0,0 ); //forward facing normal vector
	MATHVECTOR <T, 3> A = wheel_contacts[WHEEL_POSITION ( i ) ].GetNormal(); //driving surface normal
	( - ( body.GetOrientation() * wheel_orientation ) ).RotateVector ( A ); //rotate to wheelspace
	MATHVECTOR <T, 3> Aproj = B.cross ( A.cross ( B ) /B.Magnitude() ); //project the ground normal onto our forward facing plane
	assert ( Aproj.Magnitude() > 0.001 ); //ensure the wheel isn't in an odd orientation
	Aproj = Aproj.Normalize();
	MATHVECTOR <T, 3> up ( 0,0,1 ); //upward facing normal vector
	T camber_rads = acos ( Aproj.dot ( up ) ); //find the angular difference in the camber axis between up and the projected ground normal
	assert ( !isnan ( camber_rads ) );
	//MATHVECTOR <T, 3> crosscheck = Aproj.cross(up); //find axis of rotation between Aproj and up
	//camber_rads = (crosscheck[0] < 0) ? -camber_rads : camber_rads; //correct sign of angular distance
	camber_rads = -camber_rads;
	//std::cout << i << ". " << Aproj << " | " << camber_rads*180/3.141593 << std::endl;
	wheel[WHEEL_POSITION(i)].SetCamberDeg(camber_rads*180.0/3.141593);

	T friction_coeff = tire[WHEEL_POSITION ( i ) ].GetTread() *wheel_contacts[i].GetFrictionTread() + ( 1.0-tire[WHEEL_POSITION ( i ) ].GetTread() ) *wheel_contacts[i].GetFrictionNoTread();

	MATHVECTOR <T, 3> friction_force = tire[WHEEL_POSITION ( i ) ].GetForce ( normal_force, friction_coeff, groundvel, wheelspeed, camber_rads );

	//cap longitudinal force to prevent limit cycling.  we should never need to apply so much tire friction that we move the body faster than the wheel is rotating
	if ( frictionlimiting )
	{
		T limit = ( ( wheelspeed-groundvel[0] ) *dt*body.GetMass() *0.25 ) / ( dt*dt );
		//std::cout << "friction_force0=" << friction_force[0] << ", limit=" << limit << std::endl;
		if ( ( friction_force[0] < 0 && limit > 0 ) || ( friction_force[0] > 0 && limit < 0 ) )
			limit = 0;
		if ( friction_force[0] > 0 && friction_force[0] > limit )
			friction_force[0] = limit;
		else if ( friction_force[0] < 0 && friction_force[0] < limit )
			friction_force[0] = limit;
	}

	for ( int n = 0; n < 3; n++ ) assert ( !isnan ( friction_force[n] ) );

	return friction_force;
}

///do traction control system (wheelspin prevention) calculations and modify the throttle position if necessary
void CARDYNAMICS::DoTCS ( int i, T suspension_force )
{
	T sp ( 0 ), ah ( 0 );
	tire[WHEEL_POSITION ( i ) ].LookupSigmaHatAlphaHat ( suspension_force, sp, ah );
	//sp is the ideal slip ratio given tire loading

	T gasthresh = 0.1;

	T sense = 1.0;
	if ( transmission.GetGear() < 0 )
		sense = -1.0;

	T gas = engine.GetThrottle();

	//only active if throttle commanded past threshold
	if ( gas > gasthresh )
	{
		//see if we're spinning faster than the rest of the wheels
		T maxspindiff = 0;
		T myrotationalspeed = wheel[WHEEL_POSITION ( i ) ].GetAngularVelocity();
		for ( int i2 = 0; i2 < WHEEL_POSITION_SIZE; i2++ )
		{
			T spindiff = myrotationalspeed - wheel[WHEEL_POSITION ( i2 ) ].GetAngularVelocity();
			if ( spindiff < 0 )
				spindiff = -spindiff;
			if ( spindiff > maxspindiff )
				maxspindiff = spindiff;
		}

		//don't engage if all wheels are moving at the same rate
		if ( maxspindiff > 1.0 )
		{
			T error = tire[WHEEL_POSITION ( i ) ].GetSlide() *sense - sp;
			T thresholdeng = 0.0;
			//T thresholdeng = -sp/2.0;
			T thresholddis = -sp/2.0;
			//T thresholddis = -sp;

			if ( error > thresholdeng && ! tcs_active[i] )
				tcs_active[i] = true;

			if ( error < thresholddis && tcs_active[i] )
				tcs_active[i] = false;

			if ( tcs_active[i] )
			{
				T curclutch = clutch.GetClutch();
				if ( curclutch > 1 ) curclutch = 1;
				if ( curclutch < 0 ) curclutch = 0;

				gas = gas - error*10.0*curclutch;
				if ( gas < 0 ) gas = 0;
				if ( gas > 1 ) gas = 1;
				engine.SetThrottle ( gas );
			}
		}
		else
			tcs_active[i] = false;
	}
	else
		tcs_active[i] = false;
}

///do anti-lock brake system calculations and modify the brake force if necessary
void CARDYNAMICS::DoABS ( int i, T suspension_force )
{
	//an ideal ABS algorithm
	T sp ( 0 ), ah ( 0 );
	tire[WHEEL_POSITION ( i ) ].LookupSigmaHatAlphaHat ( suspension_force, sp, ah );
	//sp is the ideal slip ratio given tire loading

	//only active if brakes commanded past threshold
	T brakesetting = brake[WHEEL_POSITION ( i ) ].GetBrakeFactor();
	if ( brakesetting > 0.1 )
	{
		T maxspeed = 0;
		for ( int i2 = 0; i2 < WHEEL_POSITION_SIZE; i2++ )
		{
			if ( wheel[WHEEL_POSITION ( i2 ) ].GetAngularVelocity() > maxspeed )
				maxspeed = wheel[WHEEL_POSITION ( i2 ) ].GetAngularVelocity();
		}

		//don't engage ABS if all wheels are moving slowly
		if ( maxspeed > 6.0 )
		{
			T error = - tire[WHEEL_POSITION ( i ) ].GetSlide() - sp;
			
			T thresholdeng = 0.0;
			T thresholddis = -sp/2.0;

			if ( error > thresholdeng && ! abs_active[i] )
				abs_active[i] = true;

			if ( error < thresholddis && abs_active[i] )
				abs_active[i] = false;
		}
		else
			abs_active[i] = false;
	}
	else
		abs_active[i] = false;

	if ( abs_active[i] )
		brake[WHEEL_POSITION ( i ) ].SetBrakeFactor ( 0.0 );
}

void CARDYNAMICS::ApplyWheelForces ( T dt, T wheel_drive_torque, int i, const MATHVECTOR <T, 3> & suspension_force, MATHVECTOR <T, 3> & total_force, MATHVECTOR <T, 3> & total_torque )
{
	//compute tire friction force
	bool frictionlimiting = false;
	MATHVECTOR <T, 3> groundvel = GetWheelVelocity ( WHEEL_POSITION ( i ) );
	for ( int n = 0; n < 3; n++ ) assert ( !isnan ( groundvel[n] ) );
	QUATERNION <T> wheel_orientation = GetWheelSteeringAndSuspensionOrientation ( WHEEL_POSITION ( i ) );
	QUATERNION <T> wheelspace = body.GetOrientation() * wheel_orientation;
	( -wheelspace ).RotateVector ( groundvel );
	for ( int n = 0; n < 3; n++ ) assert ( !isnan ( groundvel[n] ) );

	MATHVECTOR <T, 3> tire_force;
	MATHVECTOR <T, 3> tire_torque;

	const int extra_iterations = 1;

	const T newdt = dt/extra_iterations;
	for ( int n = 0; n < extra_iterations; n++ )
	{
		wheel[WHEEL_POSITION ( i ) ].Integrate1 ( newdt );

		T wheelspeed = wheel[WHEEL_POSITION ( i ) ].GetAngularVelocity() *tire[WHEEL_POSITION ( i ) ].GetRadius();
		assert ( !isnan ( wheelspeed ) );
		MATHVECTOR <T, 3> friction_force = ComputeTireFrictionForce ( i, newdt, suspension_force, frictionlimiting, wheelspeed, groundvel, wheel_orientation );

		//calculate reaction torque
		tire_force.Set ( friction_force[0], friction_force[1], 0 );
		T reaction_torque = tire_force [0] * tire[WHEEL_POSITION ( i ) ].GetRadius();
		assert ( !isnan ( reaction_torque ) );

		//set wheel drive and brake torques
		wheel[WHEEL_POSITION ( i ) ].SetDriveTorque ( wheel_drive_torque );
		T wheel_brake_torque = -brake[WHEEL_POSITION ( i ) ].GetTorque ( wheel[WHEEL_POSITION ( i ) ].GetAngularVelocity() );
		//std::cout << i << ". " << wheel_brake_torque << std::endl;
		assert ( !isnan ( wheel_brake_torque ) );
		wheel[WHEEL_POSITION ( i ) ].SetBrakingTorque ( wheel_brake_torque );

		//limit the reaction torque to the applied drive and braking torque
		T applied_torque = wheel_drive_torque + wheel_brake_torque;
		if ( ( applied_torque > 0 && reaction_torque > applied_torque ) ||
		        ( applied_torque < 0 && reaction_torque < applied_torque ) )
			reaction_torque = applied_torque;

		//calculate force feedback
		tire_torque.Set ( 0, -reaction_torque, -friction_force[2] );
		//tire_torque.Set(0, reaction_torque, friction_force[2]);
		tire[WHEEL_POSITION ( i ) ].SetFeedback ( friction_force[2] );

		//set wheel torque due to tire rolling resistance
		T tire_rolling_resistance_torque ( 0 );
		if ( !brake[WHEEL_POSITION ( i ) ].GetLocked() )
		{
			T tire_friction_torque = tire_force [0] * tire[WHEEL_POSITION ( i ) ].GetRadius();

			//cap longitudinal forces to prevent the tire from limit cycling.  we should never need to apply so much tire friction that we make the wheel rotate faster than the ground *taking into account braking and drive torque*
			if ( frictionlimiting )
			{
				T limit = -applied_torque + ( ( wheelspeed-groundvel[0] ) *newdt*wheel[WHEEL_POSITION ( i ) ].GetInertia() ) / ( newdt*newdt*tire[WHEEL_POSITION ( i ) ].GetRadius() );
				if ( ( tire_friction_torque < 0 && limit > 0 ) || ( tire_friction_torque > 0 && limit < 0 ) )
					limit = 0;
				if ( tire_friction_torque > 0 && tire_friction_torque > limit )
					tire_friction_torque = limit;
				else if ( tire_friction_torque < 0 && tire_friction_torque < limit )
					tire_friction_torque = limit;
			}

			tire_rolling_resistance_torque = -tire[WHEEL_POSITION ( i ) ].GetRollingResistance ( wheel[WHEEL_POSITION ( i ) ].GetAngularVelocity(), wheel_contacts[i].GetRollingResistanceCoefficient() )
			                                 * tire[WHEEL_POSITION ( i ) ].GetRadius() - tire_friction_torque;
			assert ( !isnan ( tire_rolling_resistance_torque ) );
		}
		wheel[WHEEL_POSITION ( i ) ].SetRollingResistanceTorque ( tire_rolling_resistance_torque );

		for ( int axis = 0; axis < 2; axis++ )
			tire_force[axis] -= groundvel[axis]*wheel_contacts[i].GetRollingDrag();

		//have the wheels internally apply forces, or just forcibly set the wheel speed if the brakes are locked
		if ( brake[WHEEL_POSITION ( i ) ].GetLocked() )
		{
			wheel[WHEEL_POSITION ( i ) ].SetAngularVelocity ( 0 );
			wheel[WHEEL_POSITION ( i ) ].ZeroForces();
		}
		else
			wheel[WHEEL_POSITION ( i ) ].ApplyForces ( newdt );

		wheel[WHEEL_POSITION ( i ) ].Integrate2 ( newdt );
	}

	//apply forces to body
	MATHVECTOR <T, 3> world_tire_force = tire_force;
	wheel_orientation.RotateVector ( world_tire_force );
	body.GetOrientation().RotateVector ( world_tire_force );
	MATHVECTOR <T, 3> world_tire_torque = tire_torque;
	wheel_orientation.RotateVector ( world_tire_torque );
	body.GetOrientation().RotateVector ( world_tire_torque );
	//MATHVECTOR <T, 3> tirepos = WorldToRigidBodyWorld(GetWheelPositionAtDisplacement(WHEEL_POSITION(i), suspension[i].GetDisplacementPercent()));
	MATHVECTOR <T, 3> tirepos = GetWheelPosition ( WHEEL_POSITION ( i ) ) - GetCenterOfMassPosition();
	MATHVECTOR <T, 3> cm_force;
	MATHVECTOR <T, 3> cm_torque;
	body.GetForceAndTorqueAtOffset ( world_tire_force, world_tire_torque, tirepos, cm_force, cm_torque );

	//std::cout << i << ", " << tirepos << std::endl;

	//std::cout << i << ". force = " << tire_force << ", torque = " << tire_torque << std::endl;
	//std::cout << i << ". groundspeed = " << groundvel << ", wheelspeed = " << wheelspeed << ", friction = " << friction_force << std::endl;
	//std::cout << i << ". groundvel = " << groundvel << ", wheelspeed = " << wheelspeed << ", torque = " << tire_torque << std::endl;
	//std::cout << i << ". bodyvel = " << body.GetVelocity() << std::endl;
	//std::cout << i << ". worldvel = " << GetWheelVelocity(WHEEL_POSITION(i),dt) << ", cm_force = " << cm_force << ", cm_torque = " << cm_torque << std::endl;
	//std::cout << i << ". world force = " << world_tire_force << ", body torque = " << body_tire_torque << std::endl;

	total_force = total_force + cm_force;
	total_torque = total_torque + cm_torque;
}

///the core function of the car dynamics simulation:  find and apply all forces on the car and components.
void CARDYNAMICS::ApplyForces ( T dt )
{
	assert ( dt > 0 );

	//do TCS first thing
	if ( tcs )
	{
		for ( int i = 0; i < WHEEL_POSITION_SIZE; i++ )
		{
			DoTCS ( i, last_suspension_force[i].Magnitude() );
		}
	}

	//calculate driveshaft speed
	T driveshaft_speed = CalculateDriveshaftSpeed();
	assert ( !isnan ( driveshaft_speed ) );

	//calculate clutch speed
	T clutch_speed = transmission.CalculateClutchSpeed ( driveshaft_speed );
	assert ( !isnan ( clutch_speed ) );

	//get crankshaft speed
	T crankshaft_speed = engine.GetAngularVelocity();
	assert ( !isnan ( crankshaft_speed ) );

	//calculate engine drag torque due to friction from the clutch
	//last_crankshaft_speed = (crankshaft_speed + last_crankshaft_speed) * 0.5;
	//T speed_diff = clutch_speed - crankshaft_speed;
	T engine_drag = clutch.GetTorque ( crankshaft_speed, clutch_speed );
	assert ( !isnan ( engine_drag ) );
	
	//compute engine combustion torque and friction torque
	engine.ComputeForces();
	
	//telemetry.AddRecord("clutch torque", engine_drag);
	
	//reduce engine_drag if required; the most force the clutch should be able to exert is only enough to get the crankshaft_speed and clutch_speed to match
	/*assert(dt != 0);
	//T max_engine_drag = -(speed_diff*engine.GetInertia()/dt - engine.GetTorque());
	T max_engine_drag = -((last_crankshaft_speed-crankshaft_speed)*engine.GetInertia()/dt - engine.GetTorque());
	//T max_engine_drag = -(((last_crankshaft_speed+speed_diff)*0.5)*engine.GetInertia()/dt - engine.GetTorque());
	if ((max_engine_drag > 0 && engine_drag > max_engine_drag) || (max_engine_drag < 0 && engine_drag < max_engine_drag))
	{
		//std::cout << speed_diff << ", " << engine.GetTorque() << ", " << engine_drag << ", " << max_engine_drag << std::endl;
		engine_drag = max_engine_drag;
		assert ( !isnan ( engine_drag ) );
	}
	telemetry.AddRecord("max engine drag", max_engine_drag);
	telemetry.AddRecord("speed diff*100", speed_diff*100);
	telemetry.AddRecord("engine torque*100", engine.GetTorque()*100);
	telemetry.AddRecord("crankshaft_speed*10", crankshaft_speed*10);
	telemetry.AddRecord("last_crankshaft_speed*100", last_crankshaft_speed*100);
	std::cout << speed_diff << ", " << last_crankshaft_speed << ", " << (last_crankshaft_speed+speed_diff)*0.5 << std::endl;
	last_crankshaft_speed = speed_diff;*/

	//apply the clutch drag torque to the engine, then have the engine apply its internal forces
	ApplyClutchTorque ( engine_drag, clutch_speed );
	engine.ApplyForces ( dt );

	//get the drive torque for each wheel
	T wheel_drive_torque[4];
	CalculateDriveTorque ( wheel_drive_torque, engine_drag );

	//start accumulating forces and torques on the car body
	MATHVECTOR <T, 3> total_force ( 0 );
	MATHVECTOR <T, 3> total_torque ( 0 );

	//apply equal and opposite engine torque to the chassis
	ApplyEngineTorqueToBody ( total_force, total_torque );

	//apply gravity
	total_force = total_force + GetGravityForce();
	for ( int i = 0; i < 3; i++ ) assert ( !isnan ( total_force[i] ) );

	//apply aerodynamics
	ApplyAerodynamicsToBody ( total_force, total_torque );

	//compute suspension displacements
	for ( int i = 0; i < WHEEL_POSITION_SIZE; i++ )
	{
		ComputeSuspensionDisplacement ( i, dt );
	}

	//compute suspension forces
	for ( int i = 0; i < WHEEL_POSITION_SIZE; i++ )
	{
		last_suspension_force[i] = ApplySuspensionForceToBody ( i, dt, total_force, total_torque );
	}

	//do abs
	if ( abs )
	{
		for ( int i = 0; i < WHEEL_POSITION_SIZE; i++ )
		{
			DoABS ( i, last_suspension_force[i].Magnitude() );
		}
	}

	//compute wheel forces
	for ( int i = 0; i < WHEEL_POSITION_SIZE; i++ )
	{
		ApplyWheelForces ( dt, wheel_drive_torque[i], i, last_suspension_force[i], total_force, total_torque );
	}

	total_force = total_force + contact_force/dt;
	total_torque = total_torque + contact_torque/dt;
	contact_force.Set ( 0.0 );
	contact_torque.Set ( 0.0 );

	for ( int n = 0; n < 3; n++ ) assert ( !isnan ( total_force[n] ) );
	for ( int n = 0; n < 3; n++ ) assert ( !isnan ( total_torque[n] ) );
	body.SetForce ( total_force );
	lastbodyforce = total_force;
	body.SetTorque ( total_torque );

	//update fuel tank
	fuel_tank.Consume ( engine.FuelRate() *dt );
	engine.SetOutOfGas ( fuel_tank.Empty() );
}

///set up our data structures for storing previous wheel positions
void CARDYNAMICS::InitializeWheelVelocity()
{
	/*int frames = 4; //number of previous frames to keep
	for (int i = 0; i < frames; i++)
	{
	previous_wheel_positions.push_back(std::vector <MATHVECTOR <T, 3> >());
	previous_wheel_positions.back().resize(WHEEL_POSITION_SIZE);
	for (unsigned int w = 0; w < previous_wheel_positions.back().size(); w++)
	previous_wheel_positions.back()[w] = GetWheelPosition(WHEEL_POSITION(w));
	}*/
	//previous_wheel_position.resize(WHEEL_POSITION_SIZE);
	wheel_velocity.resize ( WHEEL_POSITION_SIZE );
	//wheel_position_valid.resize(WHEEL_POSITION_SIZE, false);
}

CARDYNAMICS::CARDYNAMICS() : drive ( RWD ), maxangle ( 45.0 ), abs(false), tacho_rpm(0), telemetry ( "telemetry" )
{
	suspension.resize ( WHEEL_POSITION_SIZE );wheel.resize ( WHEEL_POSITION_SIZE );
	tire.resize ( WHEEL_POSITION_SIZE );wheel_contacts.resize ( WHEEL_POSITION_SIZE );
	brake.resize ( WHEEL_POSITION_SIZE );abs_active.resize ( WHEEL_POSITION_SIZE, false );
	tcs_active.resize ( WHEEL_POSITION_SIZE, false );last_suspension_force.resize ( WHEEL_POSITION_SIZE );
	//wheel_position.resize(WHEEL_POSITION_SIZE);
}

void CARDYNAMICS::SetABS ( const bool newabs ) {abs = newabs;}
bool CARDYNAMICS::GetABSEnabled() const {return abs;}
bool CARDYNAMICS::GetABSActive() const
{
	return abs && ( abs_active[0]||abs_active[1]||abs_active[2]||abs_active[3] );
}

void CARDYNAMICS::SetTCS ( const bool newtcs ) {tcs = newtcs;}
bool CARDYNAMICS::GetTCSEnabled() const {return tcs;}
bool CARDYNAMICS::GetTCSActive() const
{
	return tcs && ( tcs_active[0]||tcs_active[1]||tcs_active[2]||tcs_active[3] );
}

void CARDYNAMICS::UpdateTelemetry ( float dt )
{
	//telemetry.AddRecord("RPM", GetDriveshaftRPM());
	//telemetry.AddRecord("rear left wheel speed", wheel[REAR_LEFT].GetAngularVelocity());
	
	/*for (int i = 2; i < WHEEL_POSITION_SIZE; i++) //front wheels
	{
		std::stringstream str;
		str << i;
		telemetry.AddRecord(str.str()+"-displacement", suspension[WHEEL_POSITION(i)].GetDisplacement());
		//telemetry.back().AddVariable(&(suspension[WHEEL_POSITION(i)].GetLastDisplacement()), str.str()+"-last-displacement");
		//telemetry.back().AddVariable(&(suspension[WHEEL_POSITION(i)].GetVelocity()), str.str()+"-velocity");
		//telemetry.AddRecord(str.str()+"-spring", suspension[WHEEL_POSITION(i)].GetSpringForce());
		//telemetry.AddRecord(str.str()+"-damp", suspension[WHEEL_POSITION(i)].GetDampForce());
	}*/

	/*telemetry.AddRecord ( "velocity x", body.GetVelocity() [0] );
	telemetry.AddRecord ( "velocity y", body.GetVelocity() [1] );
	telemetry.AddRecord ( "velocity z", body.GetVelocity() [2] );
	telemetry.AddRecord ( "speed", body.GetVelocity().Magnitude() );*/

	/*telemetry.push_back(CARTELEMETRY <T> ("wheel"));
	for (int i = 0; i < WHEEL_POSITION_SIZE; i++)
	{
	std::stringstream str;
	str << i;
	telemetry.back().AddVariable(&(wheel[WHEEL_POSITION(i)].GetDriveTorque()), str.str()+"-torque-drive");
	telemetry.back().AddVariable(&(wheel[WHEEL_POSITION(i)].GetBrakingTorque()), str.str()+"-torque-braking");
	telemetry.back().AddVariable(&(wheel[WHEEL_POSITION(i)].GetRollingResistanceTorque()), str.str()+"-torque-rollingresistance");
	telemetry.back().AddVariable(&(wheel[WHEEL_POSITION(i)].GetAngVelInfo()), str.str()+"-speed");
		
	}

	telemetry.push_back(CARTELEMETRY <T> ("front_differential"));
	telemetry.back().AddVariable(&(front_differential.GetSide1Speed()), "front_diff_speed_side1");
	telemetry.back().AddVariable(&(front_differential.GetSide2Speed()), "front_diff_speed_side2");
	telemetry.back().AddVariable(&(front_differential.GetSide1Torque()), "front_diff_torque_side1");
	telemetry.back().AddVariable(&(front_differential.GetSide2Torque()), "front_diff_torque_side2");

	telemetry.push_back(CARTELEMETRY <T> ("rear_differential"));
	telemetry.back().AddVariable(&(rear_differential.GetSide1Speed()), "rear_diff_speed_side1");
	telemetry.back().AddVariable(&(rear_differential.GetSide2Speed()), "rear_diff_speed_side2");
	telemetry.back().AddVariable(&(rear_differential.GetSide1Torque()), "rear_diff_torque_side1");
	telemetry.back().AddVariable(&(rear_differential.GetSide2Torque()), "rear_diff_torque_side2");

	telemetry.push_back(CARTELEMETRY <T> ("center_differential"));
	telemetry.back().AddVariable(&(center_differential.GetSide1Speed()), "center_diff_speed_side1");
	telemetry.back().AddVariable(&(center_differential.GetSide2Speed()), "center_diff_speed_side2");
	telemetry.back().AddVariable(&(center_differential.GetSide1Torque()), "center_diff_torque_side1");
	telemetry.back().AddVariable(&(center_differential.GetSide2Torque()), "center_diff_torque_side2");*/

	telemetry.Update ( dt );
}

void CARDYNAMICS::Tick ( T dt )
{
	std::vector <MATHVECTOR <T, 3> > previous_wheel_position ( 4 );
	for ( int n = 0; n < 4; n++ )
	{
		previous_wheel_position[n] = GetWheelPosition ( WHEEL_POSITION ( n ) );
	}

	body.Integrate1 ( dt );
	engine.Integrate1 ( dt );

	//for (int i = 0; i < WHEEL_POSITION_SIZE; i++) wheel[WHEEL_POSITION(i)].Integrate1(dt); //moved to ApplyWheelForces

	ApplyForces ( dt );

	//for (int i = 0; i < WHEEL_POSITION_SIZE; i++) wheel[WHEEL_POSITION(i)].Integrate2(dt); //moved to ApplyWheelForces

	engine.Integrate2 ( dt );
	body.Integrate2 ( dt );

	//calculate the wheel velocity
	assert ( wheel_velocity.size() == 4 );
	for ( int n = 0; n < 4; n++ )
	{
		wheel_velocity[n] = ( GetWheelPosition ( WHEEL_POSITION ( n ) ) - previous_wheel_position[n] ) /dt;
	}

	//the order of functions below is tailored to modified verlet integration, but it still doesn't quite work right
	/*std::vector <MATHVECTOR <T, 3> > previous_wheel_position(4);
	for (int n = 0; n < 4; n++)
	{
	previous_wheel_position[n] = GetWheelPosition(WHEEL_POSITION(n));
	}

	body.Integrate1(dt);
	engine.Integrate1(dt);
	for (int i = 0; i < WHEEL_POSITION_SIZE; i++)
	wheel[WHEEL_POSITION(i)].Integrate1(dt);

	//calculate the wheel velocity
	assert(wheel_velocity.size() == 4);
	for (int n = 0; n < 4; n++)
	{
	wheel_velocity[n] = (GetWheelPosition(WHEEL_POSITION(n)) - previous_wheel_position[n])/dt;
	}

	ApplyForces(dt);

	for (int i = 0; i < WHEEL_POSITION_SIZE; i++)
	wheel[WHEEL_POSITION(i)].Integrate2(dt);
	engine.Integrate2(dt);
	body.Integrate2(dt);*/
	
	//calculate tacho
	const float tacho_factor = 0.1;
	tacho_rpm = engine.GetRPM()*tacho_factor+tacho_rpm*(1.0-tacho_factor);

	UpdateTelemetry(dt);
}

///set the height between the center of the wheel and the ground
void CARDYNAMICS::SetWheelContactProperties ( WHEEL_POSITION wheel_index, T wheelheight, const MATHVECTOR <T, 3> & position, const MATHVECTOR <T, 3> & normal, T bumpwave, T bumpamp, T frict_no, T frict_tread, T roll_res, T roll_drag, SURFACE::CARSURFACETYPE surface )
{
	wheel_contacts[wheel_index] = CARCONTACTPROPERTIES ( wheelheight, position, normal,
	                              bumpwave, bumpamp, frict_no,
	                              frict_tread, roll_res, roll_drag, surface );
}

const CARCONTACTPROPERTIES & CARDYNAMICS::GetWheelContactProperties ( WHEEL_POSITION wheel_index ) const
{
	return wheel_contacts[wheel_index];
}

///returns the worldspace position of the center of the wheel
MATHVECTOR< T, 3 > CARDYNAMICS::GetWheelPosition ( WHEEL_POSITION wp ) const
{
	/*if (!wheel_position_valid[wp])
	{
	wheel_position[wp] = GetWheelPositionAtDisplacement(wp, suspension[wp].GetDisplacementPercent());
	wheel_position_valid[wp] = true;
	}

	return wheel_position[wp];*/

	return GetWheelPositionAtDisplacement ( wp, suspension[wp].GetDisplacementPercent() );
}

///returns the worldspace linear velocity of the center of the wheel relative to the ground
MATHVECTOR <T, 3> CARDYNAMICS::GetWheelVelocity ( WHEEL_POSITION wp ) const
{
	/*MATHVECTOR <T, 3> wheelpos = GetWheelPosition(wp);
	MATHVECTOR <T, 3> angvel = body.GetAngularVelocity();
	MATHVECTOR <T, 3> linearvel = body.GetVelocity();
	body.GetOrientation().RotateVector(linearvel);
	MATHVECTOR <T, 3> linearangvel = angvel.cross(wheelpos);
	body.GetOrientation().RotateVector(linearangvel);

	//std::cout << linearvel << " --- " << linearangvel << std::endl;

	return linearvel + linearangvel;*/

	//return body.GetVelocity();

	//assert(previous_wheel_position.size() == 4);

	//simple two-point approximation
	//return (GetWheelPosition(wp) - previous_wheel_position[wp])/dt;

	assert ( wheel_velocity.size() == 4 );
	return wheel_velocity[wp];

	//two point approximation
	//return (GetWheelPosition(wp, suspension[wp].GetDisplacement()) - previous_wheel_positions[1][wp])/(2.0*dt);

	//two point average
	//MATHVECTOR <T, 3> vel = (GetWheelPosition(wp, suspension[wp].GetDisplacement()) - previous_wheel_positions[0][wp])/(dt);
	//MATHVECTOR <T, 3> lastvel = (previous_wheel_positions[0][wp] - previous_wheel_positions[1][wp])/(dt);
	//return (vel+lastvel)*0.5;

	//five point approximation
	//return (-GetWheelPosition(wp, suspension[wp].GetDisplacement()) + previous_wheel_positions[0][wp]*8.0
	//- previous_wheel_positions[2][wp]*8.0+ previous_wheel_positions[3][wp])/(12.0*dt);
}

///returns the orientation of the wheel
QUATERNION <T> CARDYNAMICS::GetWheelOrientation ( WHEEL_POSITION wp )
{
	QUATERNION <T> ident;
	return GetWheelOrientation ( wp, ident );
}

///returns the orientation of the wheel, accounting for a supplied orientation adjustment for the model
QUATERNION <T> CARDYNAMICS::GetWheelOrientation ( WHEEL_POSITION wp, QUATERNION <T> modelorient )
{
	QUATERNION <T> wheelrot = -wheel[wp].GetOrientation();
	QUATERNION <T> siderot;
	if ( wp == FRONT_RIGHT || wp == REAR_RIGHT )
	{
		siderot.Rotate ( 3.141593, 0,0,1 );
	}

	return body.GetOrientation() * GetWheelSteeringAndSuspensionOrientation ( wp ) * modelorient * wheelrot * siderot;
}

///returns the orientation of the wheel but ignores wheel rotation, accounting for a supplied orientation adjustment for the model
QUATERNION <T> CARDYNAMICS::GetWheelOrientationFloating ( WHEEL_POSITION wp, QUATERNION <T> modelorient )
{
	QUATERNION <T> wheelrot;
	QUATERNION <T> siderot;
	if ( wp == FRONT_RIGHT || wp == REAR_RIGHT )
	{
		siderot.Rotate ( 3.141593, 0,0,1 );
	}

	return body.GetOrientation() * GetWheelSteeringAndSuspensionOrientation ( wp ) * modelorient * wheelrot * siderot;
}

///returns the orientation of the wheel due only to steering and suspension
QUATERNION <T> CARDYNAMICS::GetWheelSteeringAndSuspensionOrientation ( WHEEL_POSITION wp ) const
{
	QUATERNION <T> steer;
	steer.Rotate ( -wheel[wp].GetSteerAngle() *3.141593/180.0, 0, 0, 1 );

	QUATERNION <T> camber;
	T camber_rotation = -suspension[wp].GetCamber() *3.141593/180.0;
	if ( wp == 1 || wp == 3 )
		camber_rotation = -camber_rotation;
	camber.Rotate ( camber_rotation, 1, 0, 0 );

	QUATERNION <T> toe;
	T toe_rotation = suspension[wp].GetToe() *3.141593/180.0;
	if ( wp == 0 || wp == 2 )
		toe_rotation = -toe_rotation;
	toe.Rotate ( toe_rotation, 0, 0, 1 );

	return camber*toe*steer;
}

///returns the worldspace position of the center of the wheel when the suspension is compressed by the displacement_percent where 1.0 is fully compressed
MATHVECTOR< T, 3 > CARDYNAMICS::GetWheelPositionAtDisplacement ( WHEEL_POSITION wp, T displacement_percent ) const
{
	//very simple linear suspension displacement
	/*MATHVECTOR <T, 3> suspdisp;
	suspdisp.Set(0,0,suspension[wp].GetTravel()*displacement_percent);
	return CarLocalToWorld(wheel[wp].GetExtendedPosition()+suspdisp);*/

	//simple hinge (arc) suspension displacement
	const MATHVECTOR <T, 3> & wheelext = wheel[wp].GetExtendedPosition();
	const MATHVECTOR <T, 3> & hinge = suspension[wp].GetHinge();
	//MATHVECTOR <T, 3> hinge = wheelext-MATHVECTOR<T,3>(100,0,0);
	MATHVECTOR <T, 3> relwheelext = wheelext - hinge;
	T travel = suspension[wp].GetTravel();
	T displacement = displacement_percent * travel;
	MATHVECTOR <T, 3> up ( 0,0,1 );
	body.GetOrientation().RotateVector ( up );
	//std::cout << wheelext << " --- " << hinge << std::endl;
	MATHVECTOR <T, 3> rotaxis = up.cross ( relwheelext.Normalize() );
	T hingeradius = relwheelext.Magnitude();
	T displacementradians = displacement / hingeradius;
	QUATERNION <T> hingerotate;
	hingerotate.Rotate ( -displacementradians, rotaxis[0], rotaxis[1], rotaxis[2] );
	//std::cout << "Disp: " << displacement << ", rad: " << displacementradians << std::endl;
	MATHVECTOR <T, 3> localwheelpos = relwheelext;
	hingerotate.RotateVector ( localwheelpos );
	return CarLocalToWorld ( localwheelpos+hinge );
}

void CARDYNAMICS::SetDrive ( const std::string & newdrive )
{
	if ( newdrive == "RWD" )
		drive = RWD;
	else if ( newdrive == "FWD" )
		drive = FWD;
	else if ( newdrive == "AWD" )
		drive = AWD;
	else
		assert ( 0 ); //shouldn't ever happen unless there's an error in the code
}

/// print debug info to the given ostream.  set p1, p2, etc if debug info part 1, and/or part 2, etc is desired
void CARDYNAMICS::DebugPrint ( std::ostream & out, bool p1, bool p2, bool p3, bool p4 )
{
	if ( p1 )
	{
		out.precision ( 2 );
		out << "---Body---" << std::endl;
		out << "Center of mass: " << center_of_mass << std::endl;
		out.precision ( 6 );
		out << "Total mass: " << body.GetMass() << std::endl;
		out << std::endl;
		engine.DebugPrint ( out );
		out << std::endl;
		fuel_tank.DebugPrint ( out );
		out << std::endl;
		clutch.DebugPrint ( out );
		out << std::endl;
		transmission.DebugPrint ( out );
		out << std::endl;
		if ( drive == RWD )
		{
			out << "(rear)" << std::endl;
			rear_differential.DebugPrint ( out );
		}
		else if ( drive == FWD )
		{
			out << "(front)" << std::endl;
			front_differential.DebugPrint ( out );
		}
		else if ( drive == AWD )
		{
			out << "(center)" << std::endl;
			center_differential.DebugPrint ( out );

			out << "(front)" << std::endl;
			front_differential.DebugPrint ( out );

			out << "(rear)" << std::endl;
			rear_differential.DebugPrint ( out );
		}
		out << std::endl;
	}

	if ( p2 )
	{
		out << "(front left)" << std::endl;
		suspension[FRONT_LEFT].DebugPrint ( out );
		out << std::endl;
		out << "(front right)" << std::endl;
		suspension[FRONT_RIGHT].DebugPrint ( out );
		out << std::endl;
		out << "(rear left)" << std::endl;
		suspension[REAR_LEFT].DebugPrint ( out );
		out << std::endl;
		out << "(rear right)" << std::endl;
		suspension[REAR_RIGHT].DebugPrint ( out );
		out << std::endl;

		out << "(front left)" << std::endl;
		brake[FRONT_LEFT].DebugPrint ( out );
		out << std::endl;
		out << "(front right)" << std::endl;
		brake[FRONT_RIGHT].DebugPrint ( out );
		out << std::endl;
		out << "(rear left)" << std::endl;
		brake[REAR_LEFT].DebugPrint ( out );
		out << std::endl;
		out << "(rear right)" << std::endl;
		brake[REAR_RIGHT].DebugPrint ( out );
	}

	if ( p3 )
	{
		out << std::endl;
		out << "(front left)" << std::endl;
		wheel[FRONT_LEFT].DebugPrint ( out );
		out << std::endl;
		out << "(front right)" << std::endl;
		wheel[FRONT_RIGHT].DebugPrint ( out );
		out << std::endl;
		out << "(rear left)" << std::endl;
		wheel[REAR_LEFT].DebugPrint ( out );
		out << std::endl;
		out << "(rear right)" << std::endl;
		wheel[REAR_RIGHT].DebugPrint ( out );

		out << std::endl;
		out << "(front left)" << std::endl;
		tire[FRONT_LEFT].DebugPrint ( out );
		out << std::endl;
		out << "(front right)" << std::endl;
		tire[FRONT_RIGHT].DebugPrint ( out );
		out << std::endl;
		out << "(rear left)" << std::endl;
		tire[REAR_LEFT].DebugPrint ( out );
		out << std::endl;
		out << "(rear right)" << std::endl;
		tire[REAR_RIGHT].DebugPrint ( out );
	}

	if ( p4 )
	{
		for ( std::vector <CARAERO<T> >::iterator i = aerodynamics.begin(); i != aerodynamics.end(); ++i )
		{
			i->DebugPrint ( out );
			out << std::endl;
		}
	}
}

MATHVECTOR <T, 3> CARDYNAMICS::GetTotalAero() const
{
	MATHVECTOR <T, 3> downforce = 0;

	for ( std::vector <CARAERO<T> >::const_iterator i = aerodynamics.begin(); i != aerodynamics.end(); ++i )
	{
		downforce = downforce + i->GetLiftVector();
		downforce = downforce + i->GetDragVector();
	}

	return downforce;
}

void CARDYNAMICS::AddMassParticle ( T newmass, MATHVECTOR <T, 3> newpos )
{
	mass_only_particles.push_back ( std::pair <T, MATHVECTOR <T, 3> > ( newmass, newpos ) );
	//std::cout << "adding mass particle " << newmass << " at " << newpos << std::endl;
}

///calculate the center of mass, calculate the total mass of the body, calculate the inertia tensor
/// then store this information in the rigid body
void CARDYNAMICS::UpdateMass()
{
	typedef std::pair <T, MATHVECTOR <T, 3> > MASS_PAIR;

	T total_mass ( 0 );

	center_of_mass.Set ( 0,0,0 );

	//calculate the total mass, and center of mass
	for ( std::list <MASS_PAIR>::iterator i = mass_only_particles.begin(); i != mass_only_particles.end(); ++i )
	{
		//add the current mass to the total mass
		total_mass += i->first;

		//incorporate the current mass into the center of mass
		center_of_mass = center_of_mass + i->second * i->first;
	}
	
	//account for fuel
	total_mass += fuel_tank.GetMass();
	center_of_mass =  center_of_mass + fuel_tank.GetPosition() * fuel_tank.GetMass();

	body.SetMass ( total_mass );

	center_of_mass = center_of_mass * ( 1.0/total_mass );

	//calculate the inertia tensor
	MATRIX3 <T> inertia;
	for ( int i = 0; i < 9; i++ )
		inertia[i] = 0;
	for ( std::list <MASS_PAIR>::iterator i = mass_only_particles.begin(); i != mass_only_particles.end(); ++i )
	{
		//transform into the rigid body coordinates
		MATHVECTOR <T, 3> position = CarLocalToRigidBodyLocal ( i->second );
		T mass = i->first;

		//add the current mass to the inertia tensor
		inertia[0] += mass * ( position[1] * position[1] + position[2] * position[2] );
		inertia[1] -= mass * ( position[0] * position[1] );
		inertia[2] -= mass * ( position[0] * position[2] );
		inertia[3] = inertia[1];
		inertia[4] += mass * ( position[2] * position[2] + position[0] * position[0] );
		inertia[5] -= mass * ( position[1] * position[2] );
		inertia[6] = inertia[2];
		inertia[7] = inertia[5];
		inertia[8] += mass * ( position[0] * position[0] + position[1] * position[1] );
	}
	//inertia.DebugPrint(std::cout);
	body.SetInertia ( inertia );
}

MATHVECTOR <T, 3> CARDYNAMICS::RigidBodyLocalToCarLocal ( const MATHVECTOR <T, 3> & rigidbodylocal ) const
{
	return rigidbodylocal + center_of_mass;
}

MATHVECTOR <T, 3> CARDYNAMICS::CarLocalToRigidBodyLocal ( const MATHVECTOR <T, 3> & carlocal ) const
{
	return carlocal - center_of_mass;
}

MATHVECTOR <T, 3> CARDYNAMICS::CarLocalToWorld ( const MATHVECTOR <T, 3> & carlocal ) const
{
	return body.TransformLocalToWorld ( carlocal-center_of_mass );
}

MATHVECTOR <T, 3> CARDYNAMICS::RigidBodyLocalToWorld ( const MATHVECTOR <T, 3> rigidbodylocal ) const
{
	return body.TransformLocalToWorld ( rigidbodylocal );
}

MATHVECTOR <T, 3> CARDYNAMICS::WorldToRigidBodyLocal ( const MATHVECTOR <T, 3> & world ) const
{
	return body.TransformWorldToLocal ( world );
}

MATHVECTOR <T, 3> CARDYNAMICS::WorldToCarLocal ( const MATHVECTOR <T, 3> & world ) const
{
	return RigidBodyLocalToCarLocal ( WorldToRigidBodyLocal ( world ) );
}

//TODO: adjustable ackermann-like parameters
///set the steering angle to "value", where 1.0 is maximum right lock and -1.0 is maximum left lock.
void CARDYNAMICS::SetSteering ( const T value )
{
	T steerangle = value * maxangle; //steering angle in degrees

	//ackermann stuff
	T alpha = std::abs ( steerangle * 3.141593/180.0 ); //outside wheel steering angle in radians
	T B = wheel[FRONT_LEFT].GetExtendedPosition() [1] - wheel[FRONT_RIGHT].GetExtendedPosition() [1]; //distance between front wheels
	T L = wheel[FRONT_LEFT].GetExtendedPosition() [0] - wheel[REAR_LEFT].GetExtendedPosition() [0]; //distance between front and rear wheels
	T beta = atan2 ( 1.0, ( ( 1.0/ ( tan ( alpha ) ) )-B/L ) ); //inside wheel steering angle in radians

	T left_wheel_angle = 0;
	T right_wheel_angle = 0;

	if ( value >= 0 )
	{
		left_wheel_angle = alpha;
		right_wheel_angle = beta;
	}
	else
	{
		right_wheel_angle = -alpha;
		left_wheel_angle = -beta;
	}

	left_wheel_angle *= 180.0/3.141593;
	right_wheel_angle *= 180.0/3.141593;

	wheel[FRONT_LEFT].SetSteerAngle ( left_wheel_angle );
	wheel[FRONT_RIGHT].SetSteerAngle ( right_wheel_angle );
}

///Get the maximum steering angle in degrees
T CARDYNAMICS::GetMaxSteeringAngle() const
{
	return maxangle;
}

///Set the maximum steering angle in degrees
void CARDYNAMICS::SetMaxSteeringAngle ( T newangle )
{
	maxangle = newangle;
}

///get the worldspace engine position
MATHVECTOR <T, 3> CARDYNAMICS::GetEnginePosition()
{
	return CarLocalToWorld ( engine.GetPosition() );
}

///translate by the given translation in worldspace
void CARDYNAMICS::Translate ( const MATHVECTOR <T, 3> & translation )
{
	body.SetPosition ( body.GetPosition() + translation );
}

///apply force from a contact to the body.  units are worldspace
void CARDYNAMICS::ProcessContact ( const MATHVECTOR <T, 3> & pos, const MATHVECTOR <T, 3> & normal, const MATHVECTOR <T, 3> & relative_velocity, T depth, T dt )
{
	MATHVECTOR <T, 3> translation;
	translation = normal* ( depth + 0.0 );

	Translate ( translation );

	MATHVECTOR <T, 3> v_perp = relative_velocity.project ( normal );
	MATHVECTOR <T, 3> v_par = relative_velocity - v_perp;
	MATHVECTOR <T, 3> impulse;
	if ( v_perp.Magnitude() > 0.001 )
		impulse = v_perp * -1.0 * body.GetMass() - v_par.Normalize() * v_perp.Magnitude();
	impulse = impulse*1.0;
	//std::cout << impulse.Magnitude() << ", " << depth << " -- " << relative_velocity << std::endl;
	MATHVECTOR <T, 3> newtorque;
	body.GetForceAtOffset ( impulse, pos-GetCenterOfMassPosition(), contact_force, newtorque );
	contact_torque = newtorque * 0.05;
}

///return the spedometer reading in m/s based on the driveshaft speed
T CARDYNAMICS::GetSpeedo()
{
	T left_front_wheel_speed = wheel[FRONT_LEFT].GetAngularVelocity();
	T right_front_wheel_speed = wheel[FRONT_RIGHT].GetAngularVelocity();
	T left_rear_wheel_speed = wheel[REAR_LEFT].GetAngularVelocity();
	T right_rear_wheel_speed = wheel[REAR_RIGHT].GetAngularVelocity();
	for ( int i = 0; i < 4; i++ ) assert ( !isnan ( wheel[WHEEL_POSITION ( i ) ].GetAngularVelocity() ) );
	if ( drive == RWD )
	{
		return ( left_rear_wheel_speed+right_rear_wheel_speed ) * 0.5 * tire[REAR_LEFT].GetRadius();
	}
	else if ( drive == FWD )
	{
		return ( left_front_wheel_speed+right_front_wheel_speed ) * 0.5 * tire[FRONT_LEFT].GetRadius();
	}
	else if ( drive == AWD )
	{
		return ( ( left_rear_wheel_speed+right_rear_wheel_speed ) * 0.5 * tire[REAR_LEFT].GetRadius() +
		         ( left_front_wheel_speed+right_front_wheel_speed ) * 0.5 * tire[FRONT_LEFT].GetRadius() ) *0.5;
	}

	assert ( 0 );
	return 0;
}

///return the magnitude of the car's velocity in m/s
T CARDYNAMICS::GetSpeed()
{
	return body.GetVelocity().Magnitude();
}

void CARDYNAMICS::AddAerodynamicDevice ( const MATHVECTOR <T, 3> & newpos, T drag_frontal_area, T drag_coefficient, T lift_surface_area,
                            T lift_coefficient, T lift_efficiency )
{
	aerodynamics.push_back ( CARAERO<T>() );
	aerodynamics.back().Set ( newpos, drag_frontal_area, drag_coefficient, lift_surface_area,
	                          lift_coefficient, lift_efficiency );
}

T CARDYNAMICS::CalculateDriveshaftRPM() const
{
	T driveshaft_speed = 0.0;
	T left_front_wheel_speed = wheel[FRONT_LEFT].GetAngularVelocity();
	T right_front_wheel_speed = wheel[FRONT_RIGHT].GetAngularVelocity();
	T left_rear_wheel_speed = wheel[REAR_LEFT].GetAngularVelocity();
	T right_rear_wheel_speed = wheel[REAR_RIGHT].GetAngularVelocity();
	for ( int i = 0; i < 4; i++ ) assert ( !isnan ( wheel[WHEEL_POSITION ( i ) ].GetAngularVelocity() ) );
	if ( drive == RWD )
	{
		driveshaft_speed = rear_differential.GetDriveshaftSpeed ( left_rear_wheel_speed, right_rear_wheel_speed );
	}
	else if ( drive == FWD )
	{
		driveshaft_speed = front_differential.GetDriveshaftSpeed ( left_front_wheel_speed, right_front_wheel_speed );
	}
	else if ( drive == AWD )
	{
		driveshaft_speed = center_differential.GetDriveshaftSpeed (
		                       front_differential.GetDriveshaftSpeed ( left_front_wheel_speed, right_front_wheel_speed ),
		                       rear_differential.GetDriveshaftSpeed ( left_rear_wheel_speed, right_rear_wheel_speed ) );
	}

	return transmission.GetClutchSpeed ( driveshaft_speed ) * 30.0 / 3.141593;
}

MATHVECTOR< T, 3 > CARDYNAMICS::GetCenterOfMass() const
{
	return center_of_mass;
}

///used by the AI
T CARDYNAMICS::GetAerodynamicDownforceCoefficient() const
{
	T coeff = 0.0;
	for ( std::vector <CARAERO <T> >::const_iterator i = aerodynamics.begin(); i != aerodynamics.end(); ++i )
		coeff += i->GetAerodynamicDownforceCoefficient();
	return coeff;
}

///used by the AI
T CARDYNAMICS::GetAeordynamicDragCoefficient() const
{
	T coeff = 0.0;
	for ( std::vector <CARAERO <T> >::const_iterator i = aerodynamics.begin(); i != aerodynamics.end(); ++i )
		coeff += i->GetAeordynamicDragCoefficient();
	return coeff;
}

bool CARDYNAMICS::Serialize ( joeserialize::Serializer & s )
{
	_SERIALIZE_ ( s,body );
	_SERIALIZE_ ( s,engine );
	_SERIALIZE_ ( s,clutch );
	_SERIALIZE_ ( s,transmission );
	_SERIALIZE_ ( s,front_differential );
	_SERIALIZE_ ( s,rear_differential );
	_SERIALIZE_ ( s,center_differential );
	_SERIALIZE_ ( s,fuel_tank );
	_SERIALIZE_ ( s,suspension );
	_SERIALIZE_ ( s,wheel );
	_SERIALIZE_ ( s,brake );
	_SERIALIZE_ ( s,tire );
	_SERIALIZE_ ( s,aerodynamics );
	_SERIALIZE_ ( s,wheel_velocity );
	_SERIALIZE_ ( s,abs );
	_SERIALIZE_ ( s,abs_active );
	_SERIALIZE_ ( s,tcs );
	_SERIALIZE_ ( s,tcs_active );
	_SERIALIZE_ ( s,last_suspension_force );
	//_SERIALIZE_ ( s,last_crankshaft_speed );
	return true;
}

MATHVECTOR< T, 3 > CARDYNAMICS::GetLastBodyForce() const
{
	return lastbodyforce;
}

bool CARDYNAMICS::WheelDriven ( WHEEL_POSITION pos ) const
{
	if ( drive == AWD )
		return true;
	else if ( drive == FWD && ( pos == FRONT_LEFT || pos == FRONT_RIGHT ) )
		return true;
	else if ( drive == RWD && ( pos == REAR_LEFT || pos == REAR_RIGHT ) )
		return true;

	return false;
}

