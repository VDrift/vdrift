#include "carpowertrain.h"

#include "configfile.h"
#include "coordinatesystems.h"

#include "math.h"

#if defined(_WIN32) || defined(__APPLE__)
template <typename T> bool isnan(T number) {return (number != number);}
#endif

template <typename T>
CARPOWERTRAIN<T>::CARPOWERTRAIN() :
	drive(RWD),
	tacho_rpm(0),
	autoclutch(true),
	autoshift(false),
	shifted(true),
	shift_gear(0),
	last_auto_clutch(1.0),
	remaining_shift_time(0.0),
	abs(false),
	tcs(false)
{
	brake.resize(WHEEL_POSITION_SIZE);
	wheel.resize(WHEEL_POSITION_SIZE);
	tire.resize(WHEEL_POSITION_SIZE);
	abs_active.resize(WHEEL_POSITION_SIZE, false);
	tcs_active.resize(WHEEL_POSITION_SIZE, false);
}

template <typename T>
CARPOWERTRAIN<T>::~CARPOWERTRAIN()
{
	//dtor
}

template <typename T>
void CARPOWERTRAIN<T>::StartEngine()
{
	engine.StartEngine();
}

template <typename T>
void CARPOWERTRAIN<T>::ShiftGear(int value)
{
	if (transmission.GetGear() != value && shifted)
	{
		if (value <= transmission.GetForwardGears() && value >= -transmission.GetReverseGears())
		{
			remaining_shift_time = transmission.GetShiftTime();
			shift_gear = value;
			shifted = false;
		}
	}
}

template <typename T>
void CARPOWERTRAIN<T>::SetThrottle(float value)
{
	engine.SetThrottle(value);
}

template <typename T>
void CARPOWERTRAIN<T>::SetClutch(float value)
{
	clutch.SetClutch(value);
}

template <typename T>
void CARPOWERTRAIN<T>::SetBrake(float value)
{
	for(unsigned int i = 0; i < brake.size(); i++)
	{
		brake[i].SetBrakeFactor(value);
	}
}

template <typename T>
void CARPOWERTRAIN<T>::SetHandBrake(float value)
{
	for(unsigned int i = 0; i < brake.size(); i++)
	{
		brake[i].SetHandbrakeFactor(value);
	}
}

template <typename T>
void CARPOWERTRAIN<T>::SetAutoClutch(bool value)
{
	autoclutch = value;
}

template <typename T>
void CARPOWERTRAIN<T>::SetAutoShift(bool value)
{
	autoshift = value;
}

template <typename T>
void CARPOWERTRAIN<T>::SetABS(const bool newabs)
{
	abs = newabs;
}

template <typename T>
void CARPOWERTRAIN<T>::SetTCS ( const bool newtcs )
{
	tcs = newtcs;
}

template <typename T>
T CARPOWERTRAIN<T>::GetSpeedMPS() const
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

	assert(false);
	return 0;
/*
	// use max wheel speed
	T speed = 0;
	for (int i = 0; i < WHEEL_POSITION_SIZE; ++i)
	{
		T wheel_speed = wheel[i].GetAngularVelocity() * tire[i].GetRadius();
		if(speed < wheel_speed) speed = wheel_speed;
	}
	return speed;
*/
}

template <typename T>
T CARPOWERTRAIN<T>::GetTachoRPM() const
{
	return tacho_rpm;
}

template <typename T>
bool CARPOWERTRAIN<T>::GetABSEnabled() const
{
	return abs;
}

template <typename T>
bool CARPOWERTRAIN<T>::GetABSActive() const
{
	return abs && ( abs_active[0]||abs_active[1]||abs_active[2]||abs_active[3] );
}

template <typename T>
bool CARPOWERTRAIN<T>::GetTCSEnabled() const
{
	return tcs;
}

template <typename T>
bool CARPOWERTRAIN<T>::GetTCSActive() const
{
	return tcs && ( tcs_active[0]||tcs_active[1]||tcs_active[2]||tcs_active[3] );
}


template <typename T>
bool CARPOWERTRAIN<T>::WheelDriven(int i) const
{
	return (1 << i) & drive;
}

template <typename T>
void CARPOWERTRAIN<T>::SetDrive(const std::string & value)
{
	if (value == "RWD")
		drive = RWD;
	else if (value == "FWD")
		drive = FWD;
	else if (value == "AWD")
		drive = AWD;
	else
		assert(false); //shouldn't ever happen unless there's an error in the code
}

template <typename T>
void CARPOWERTRAIN<T>::Update(T dt)
{
	UpdateTransmission(dt);

	fuel_tank.Consume(engine.FuelRate() * dt);
	engine.SetOutOfGas(fuel_tank.Empty());

	const float tacho_factor = 0.1;
	tacho_rpm = engine.GetRPM() * tacho_factor + tacho_rpm * (1.0 - tacho_factor);
}

template <typename T>
void CARPOWERTRAIN<T>::UpdateTransmission(T dt)
{
	driveshaft_rpm = CalculateDriveshaftRPM();

	if (autoshift)
	{
		int gear = NextGear();
		ShiftGear(gear);
	}

	remaining_shift_time -= dt;
	if (remaining_shift_time < 0) remaining_shift_time = 0;

	if (remaining_shift_time <= transmission.GetShiftTime() * 0.5 && !shifted)
	{
		shifted = true;
		transmission.Shift(shift_gear);
	}

	if (autoclutch)
	{
		if (!engine.GetCombustion())
		{
		    engine.StartEngine();
		    std::cout << "start engine" << std::endl;
		}

		T throttle = engine.GetThrottle();
		throttle = ShiftAutoClutchThrottle(throttle, dt);
		engine.SetThrottle(throttle);

		T new_clutch = AutoClutch(last_auto_clutch, dt);
		clutch.SetClutch(new_clutch);
		last_auto_clutch = new_clutch;
	}
}

template <typename T>
MATHVECTOR <T, 3> CARPOWERTRAIN<T>::GetTireForce(
	WHEEL_POSITION i,
	const T normal_force,
	const T friction_coeff,
	const MATHVECTOR <T, 3> & velocity,
	const T ang_velocity,
	const T inclination)
{
	return tire[i].GetForce(normal_force, friction_coeff, velocity, ang_velocity, inclination);
}

template <typename T>
T CARPOWERTRAIN<T>::IntegrateWheel(
	WHEEL_POSITION i,
	T tire_torque,
	T drive_torque,
	T dt)
{
	CARWHEEL<T> & wheel = this->wheel[WHEEL_POSITION(i)];
	CARBRAKE<T> & brake = this->brake[WHEEL_POSITION(i)];

	wheel.Integrate1(dt);

	T wheel_torque = drive_torque - tire_torque;
	T lock_up_torque = wheel.GetLockUpTorque(dt) - wheel_torque;	// torque needed to lock the wheel
	T brake_torque = brake.GetTorque();

	// brake and rolling resistance torque should never exceed lock up torque
	if(lock_up_torque >= 0 && lock_up_torque > brake_torque)
	{
		brake.WillLock(false);
		wheel_torque += brake_torque;   // brake torque has same direction as lock up torque
	}
	else if(lock_up_torque < 0 && lock_up_torque < -brake_torque)
	{
		brake.WillLock(false);
		wheel_torque -= brake_torque;
	}
	else
	{
		brake.WillLock(true);
		wheel_torque = wheel.GetLockUpTorque(dt);
	}
	wheel.SetTorque(wheel_torque);

	wheel.Integrate2(dt);

	return wheel_torque;
}

template <typename T>
void CARPOWERTRAIN<T>::IntegrateEngine(T drive_torque[], T dt)
{
	engine.Integrate1(dt);

	T driveshaft_speed = CalculateDriveshaftSpeed();
	T clutch_speed = transmission.CalculateClutchSpeed(driveshaft_speed);
	T crankshaft_speed = engine.GetAngularVelocity();
	T clutch_drag = clutch.GetTorqueMax(crankshaft_speed, clutch_speed);

	if(transmission.GetGear() == 0) clutch_drag = 0;

	clutch_drag = engine.ComputeForces(clutch_drag, clutch_speed, dt);

	CalculateDriveTorque(drive_torque, -clutch_drag);
	
	engine.Integrate2(dt);
}

template <typename T>
void CARPOWERTRAIN<T>::CalculateDriveTorque(T wheel_drive_torque[], T clutch_torque)
{
	T driveshaft_torque = transmission.GetTorque(clutch_torque);
	assert(!isnan(driveshaft_torque));

	for (int i = 0; i < WHEEL_POSITION_SIZE; ++i)
		wheel_drive_torque[i] = 0;

	if (drive == RWD)
	{
		rear_differential.ComputeWheelTorques(driveshaft_torque);
		wheel_drive_torque[REAR_LEFT] = rear_differential.GetSide1Torque();
		wheel_drive_torque[REAR_RIGHT] = rear_differential.GetSide2Torque();
	}
	else if (drive == FWD)
	{
		front_differential.ComputeWheelTorques(driveshaft_torque);
		wheel_drive_torque[FRONT_LEFT] = front_differential.GetSide1Torque();
		wheel_drive_torque[FRONT_RIGHT] = front_differential.GetSide2Torque();
	}
	else if (drive == AWD)
	{
		center_differential.ComputeWheelTorques(driveshaft_torque);
		front_differential.ComputeWheelTorques(center_differential.GetSide1Torque());
		rear_differential.ComputeWheelTorques(center_differential.GetSide2Torque());
		wheel_drive_torque[FRONT_LEFT] = front_differential.GetSide1Torque();
		wheel_drive_torque[FRONT_RIGHT] = front_differential.GetSide2Torque();
		wheel_drive_torque[REAR_LEFT] = rear_differential.GetSide1Torque();
		wheel_drive_torque[REAR_RIGHT] = rear_differential.GetSide2Torque();
	}

	for (int i = 0; i < WHEEL_POSITION_SIZE; ++i)
	{
		assert(!isnan(wheel_drive_torque[WHEEL_POSITION(i)]));
	}
}

template <typename T>
T CARPOWERTRAIN<T>::CalculateDriveshaftSpeed()
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

template <typename T>
T CARPOWERTRAIN<T>::CalculateDriveshaftRPM() const
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
		T front_speed = front_differential.GetDriveshaftSpeed ( left_front_wheel_speed, right_front_wheel_speed );
		T rear_speed = rear_differential.GetDriveshaftSpeed ( left_rear_wheel_speed, right_rear_wheel_speed );
		driveshaft_speed = center_differential.GetDriveshaftSpeed ( front_speed, rear_speed );
	}

	return transmission.GetClutchSpeed ( driveshaft_speed ) * 30.0 / 3.141593;
}

template <typename T>
T CARPOWERTRAIN<T>::AutoClutch(T last_clutch, T dt) const
{
	const T threshold = 1000.0;
	const T margin = 100.0;
	const T geareffect = 1.0; //zero to 1, defines special consideration of first/reverse gear

	//take into account locked brakes
	bool willlock(true);
	for (int i = 0; i < WHEEL_POSITION_SIZE; i++)
	{
		if (WheelDriven(WHEEL_POSITION(i)))
		{
            willlock = willlock && brake[i].WillLock();
		}
	}
	if (willlock) return 0;

	const T rpm = engine.GetRPM();
	const T maxrpm = engine.GetRPMLimit();
	const T stallrpm = engine.GetStallRPM() + margin * (maxrpm / 2000.0);
	const int gear = transmission.GetGear();

	T gearfactor = 1.0;
	if (gear <= 1)
		gearfactor = 2.0;
	T thresh = threshold * (maxrpm/7000.0) * ((1.0-geareffect)+gearfactor*geareffect) + stallrpm;
	if (clutch.IsLocked())
		thresh *= 0.5;
	T clutch = (rpm-stallrpm) / (thresh-stallrpm);

	//std::cout << rpm << ", " << stallrpm << ", " << threshold << ", " << clutch << std::endl;

	if (clutch < 0)
		clutch = 0;
	if (clutch > 1.0)
		clutch = 1.0;

	T newauto = clutch * ShiftAutoClutch();

	//rate limit the autoclutch
	const T min_engage_time = 0.05; //the fastest time in seconds for auto-clutch engagement
	const T engage_rate_limit = 1.0/min_engage_time;
	const T rate = (last_clutch - newauto)/dt; //engagement rate in clutch units per second
	if (rate > engage_rate_limit)
		newauto = last_clutch - engage_rate_limit*dt;

    return newauto;
}

template <typename T>
T CARPOWERTRAIN<T>::ShiftAutoClutch() const
{
	const T shift_time = transmission.GetShiftTime();
	T shift_clutch = 1.0;
	if (remaining_shift_time > shift_time * 0.5)
	    shift_clutch = 0.0;
	else if (remaining_shift_time > 0.0)
	    shift_clutch = 1.0 - remaining_shift_time / (shift_time * 0.5);
	return shift_clutch;
}


template <typename T>
T CARPOWERTRAIN<T>::ShiftAutoClutchThrottle(T throttle, T dt)
{
	if(remaining_shift_time > 0.0)
	{
	    if(engine.GetRPM() < driveshaft_rpm && engine.GetRPM() < engine.GetRedline())
	    {
	        remaining_shift_time += dt;
            return 1.0;
	    }
	    else
	    {
	        return 0.5 * throttle;
	    }
	}
	return throttle;
}

template <typename T>
int CARPOWERTRAIN<T>::NextGear() const
{
	int gear = transmission.GetGear();

	// only autoshift if a shift is not in progress
	if (shifted)
	{
        if (clutch.GetClutch() == 1.0)
        {
            // shift up when driveshaft speed exceeds engine redline
            // we do not shift up from neutral/reverse
            if (driveshaft_rpm > engine.GetRedline() && gear > 0)
            {
                return gear + 1;
            }
            // shift down when driveshaft speed below shift_down_point
            // we do not auto shift down from 1st gear to neutral
            if(driveshaft_rpm < DownshiftRPM(gear) && gear > 1)
            {
                return gear - 1;
            }
        }
    }
	return gear;
}

template <typename T>
T CARPOWERTRAIN<T>::DownshiftRPM(int gear) const
{
	T shift_down_point = 0.0;
	if (gear > 1)
	{
        T current_gear_ratio = transmission.GetGearRatio(gear);
        T lower_gear_ratio = transmission.GetGearRatio(gear - 1);
		T peak_engine_speed = engine.GetRedline();
		shift_down_point = 0.7 * peak_engine_speed / lower_gear_ratio * current_gear_ratio;
	}
	return shift_down_point;
}

template <typename T>
void CARPOWERTRAIN<T>::UpdateTCS(int i, T suspension_force)
{
	T gasthresh = 0.1;
	T gas = engine.GetThrottle();

	//only active if throttle commanded past threshold
	if (gas > gasthresh)
	{
		//see if we're spinning faster than the rest of the wheels
		T maxspindiff = 0;
		T myrotationalspeed = wheel[i].GetAngularVelocity();
		for (int i2 = 0; i2 < WHEEL_POSITION_SIZE; i2++)
		{
			T spindiff = myrotationalspeed - wheel[i2].GetAngularVelocity();
			if (spindiff < 0)
				spindiff = -spindiff;
			if (spindiff > maxspindiff)
				maxspindiff = spindiff;
		}

		//don't engage if all wheels are moving at the same rate
		if ( maxspindiff > 1.0 )
		{
			T sp = tire[i].GetIdealSlide();
			//T ah = tire[i].GetIdealSlip();

			T sense = 1.0;
			if (transmission.GetGear() < 0)
				sense = -1.0;

			T error = tire[i].GetSlide() * sense - sp;
			T thresholdeng = 0.0;
			T thresholddis = -sp/2.0;

			if (error > thresholdeng && !tcs_active[i])
				tcs_active[i] = true;

			if (error < thresholddis && tcs_active[i])
				tcs_active[i] = false;

			if (tcs_active[i])
			{
				T curclutch = clutch.GetClutch();
				if (curclutch > 1) curclutch = 1;
				if (curclutch < 0) curclutch = 0;

				gas = gas - error * 10.0 * curclutch;
				if (gas < 0) gas = 0;
				if (gas > 1) gas = 1;
				engine.SetThrottle(gas);
			}
		}
		else
			tcs_active[i] = false;
	}
	else
		tcs_active[i] = false;
}

template <typename T>
void CARPOWERTRAIN<T>::UpdateABS(int i, T suspension_force)
{
	T braketresh = 0.1;
	T brakesetting = brake[i].GetBrakeFactor();

	//only active if brakes commanded past threshold
	if (brakesetting > braketresh)
	{
		T maxspeed = 0;
		for (int i2 = 0; i2 < WHEEL_POSITION_SIZE; i2++)
		{
			if (wheel[i2].GetAngularVelocity() > maxspeed)
				maxspeed = wheel[i2].GetAngularVelocity();
		}

		//don't engage ABS if all wheels are moving slowly
		if (maxspeed > 6.0)
		{
			T sp = tire[i].GetIdealSlide();
			//T ah = tire[i].GetIdealSlip();

			T error = - tire[i].GetSlide() - sp;
			T thresholdeng = 0.0;
			T thresholddis = -sp/2.0;

			if (error > thresholdeng && !abs_active[i])
				abs_active[i] = true;

			if (error < thresholddis && abs_active[i])
				abs_active[i] = false;
		}
		else
			abs_active[i] = false;
	}
	else
		abs_active[i] = false;

	if (abs_active[i])
		brake[i].SetBrakeFactor(0.0);
}


template <typename T>
void CARPOWERTRAIN<T>::DebugPrint(std::ostream & out, bool p1, bool p2) const
{
	if (p1)
	{
		engine.DebugPrint(out);
		out << std::endl;
		fuel_tank.DebugPrint(out);
		out << std::endl;
		clutch.DebugPrint(out);
		out << std::endl;
		transmission.DebugPrint(out);
		out << std::endl;
		if (drive == RWD)
		{
			out << "(rear)" << std::endl;
			rear_differential.DebugPrint(out);
		}
		else if (drive == FWD)
		{
			out << "(front)" << std::endl;
			front_differential.DebugPrint(out);
		}
		else if (drive == AWD)
		{
			out << "(center)" << std::endl;
			center_differential.DebugPrint(out);
			out << "(front)" << std::endl;
			front_differential.DebugPrint(out);
			out << "(rear)" << std::endl;
			rear_differential.DebugPrint(out);
		}
		out << std::endl;
	}

	if (p2)
	{
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
}

template <typename T>
bool CARPOWERTRAIN<T>::Serialize(joeserialize::Serializer & s)
{
	_SERIALIZE_(s, engine);
	_SERIALIZE_(s, clutch);
	_SERIALIZE_(s, transmission);
	_SERIALIZE_(s, front_differential);
	_SERIALIZE_(s, rear_differential);
	_SERIALIZE_(s, center_differential);
	_SERIALIZE_(s, fuel_tank);
	_SERIALIZE_(s, wheel);
	_SERIALIZE_(s, brake);
	_SERIALIZE_(s, tire);
	_SERIALIZE_(s, abs);
	_SERIALIZE_(s, abs_active);
	_SERIALIZE_(s, tcs);
	_SERIALIZE_(s, tcs_active);
	_SERIALIZE_(s, last_auto_clutch);
	_SERIALIZE_(s, remaining_shift_time);
	_SERIALIZE_(s, shift_gear);
	_SERIALIZE_(s, shifted);
	_SERIALIZE_(s, autoshift);
	return true;
}

template <typename T>
bool LoadEngine(
	const CONFIGFILE & c,
	CARENGINE<T> & engine,
	std::ostream & error_output)
{
	CARENGINEINFO<T> info;
	std::vector < std::pair <T, T> > torque;
	float temp_vec3[3];
	int version(1);
	
	if (!c.GetParam("engine.peak-engine-rpm", info.redline, error_output)) return false; //used only for the redline graphics
	if (!c.GetParam("engine.rpm-limit", info.rpm_limit, error_output)) return false;
	if (!c.GetParam("engine.inertia", info.inertia, error_output)) return false;
	if (!c.GetParam("engine.start-rpm", info.start_rpm, error_output)) return false;
	if (!c.GetParam("engine.stall-rpm", info.stall_rpm, error_output)) return false;
	if (!c.GetParam("engine.fuel-consumption", info.fuel_consumption, error_output)) return false;
	if (!c.GetParam("engine.mass", info.mass, error_output)) return false;
	if (!c.GetParam("engine.position", temp_vec3, error_output)) return false;
	c.GetParam("version", version);
	if (version == 2)
	{
		COORDINATESYSTEMS::ConvertCarCoordinateSystemV2toV1(temp_vec3[0],temp_vec3[1],temp_vec3[2]);
	}
	info.position.Set(temp_vec3[0],temp_vec3[1],temp_vec3[2]);
	
	int curve_num = 0;
	float torque_point[3];
	std::string torque_str("engine.torque-curve-00");
	while (c.GetParam(torque_str, torque_point))
	{
		torque.push_back(std::pair <float, float> (torque_point[0], torque_point[1]));

		curve_num++;
		std::stringstream str;
		str << "engine.torque-curve-";
		str.width(2);
		str.fill('0');
		str << curve_num;
		torque_str = str.str();
	}
	if (torque.size() <= 1)
	{
		error_output << "You must define at least 2 torque curve points." << std::endl;
		return false;
	}
	info.SetTorqueCurve(info.redline, torque);
	
	engine.Init(info);
	
	return true;
}

template <typename T>
bool LoadClutch(
	const CONFIGFILE & c,
	CARCLUTCH<T> & clutch,
	std::ostream & error_output)
{
	float sliding, radius, area, max_pressure;

	if (!c.GetParam("clutch.sliding", sliding, error_output)) return false;
	if (!c.GetParam("clutch.radius", radius, error_output)) return false;
	if (!c.GetParam("clutch.area", area, error_output)) return false;
	if (!c.GetParam("clutch.max-pressure", max_pressure, error_output)) return false;
	
	clutch.SetSlidingFriction(sliding);
	clutch.SetRadius(radius);
	clutch.SetArea(area);
	clutch.SetMaxPressure(max_pressure);
	
	return true;
}

template <typename T>
bool LoadTransmission(
	const CONFIGFILE & c,
	CARTRANSMISSION<T> & transmission,
	std::ostream & error_output)
{
	float shift_time = 0;
	float ratio;
	int gears;

	c.GetParam("transmission.shift-time", shift_time);
	transmission.SetShiftTime(shift_time);
	
	if (!c.GetParam("transmission.gear-ratio-r", ratio, error_output)) return false;
	transmission.SetGearRatio(-1, ratio);
	
	if (!c.GetParam("transmission.gears", gears, error_output)) return false;
	for (int i = 0; i < gears; i++)
	{
		std::stringstream s;
		s << "transmission.gear-ratio-" << i+1;
		if (!c.GetParam(s.str(), ratio, error_output)) return false;
		transmission.SetGearRatio(i+1, ratio);
	}
	
	return true;
}

template <typename T>
bool LoadFuelTank(
	const CONFIGFILE & c,
	CARFUELTANK<T> & fuel_tank,
	std::ostream & error_output)
{
	float pos[3];
	MATHVECTOR <double, 3> position;
	float capacity;
	float volume;
	float fuel_density;
	int version(1);
	
	c.GetParam("version", version);
	if (!c.GetParam("fuel-tank.capacity", capacity, error_output)) return false;
	if (!c.GetParam("fuel-tank.volume", volume, error_output)) return false;
	if (!c.GetParam("fuel-tank.fuel-density", fuel_density, error_output)) return false;
	if (!c.GetParam("fuel-tank.position", pos, error_output)) return false;
	if (version == 2)
	{
		COORDINATESYSTEMS::ConvertCarCoordinateSystemV2toV1(pos[0],pos[1],pos[2]);
	}
	position.Set(pos[0],pos[1],pos[2]);
	
	fuel_tank.SetCapacity(capacity);
	fuel_tank.SetVolume(volume);
	fuel_tank.SetDensity(fuel_density);
	fuel_tank.SetPosition(position);
	
	return true;
}

template <typename T>
bool LoadBrake(
	const CONFIGFILE & c,
	const std::string & brakename,
	CARBRAKE<T> & brake,
	std::ostream & error_output)
{
	float friction, max_pressure, area, bias, radius, handbrake(0);

	if (!c.GetParam(brakename+".friction", friction, error_output)) return false;
	if (!c.GetParam(brakename+".area", area, error_output)) return false;
	if (!c.GetParam(brakename+".radius", radius, error_output)) return false;
	if (!c.GetParam(brakename+".bias", bias, error_output)) return false;
	if (!c.GetParam(brakename+".max-pressure", max_pressure, error_output)) return false;
	c.GetParam(brakename+".handbrake", handbrake);
	
	brake.SetFriction(friction);
	brake.SetArea(area);
	brake.SetRadius(radius);
	brake.SetBias(bias);
	brake.SetMaxPressure(max_pressure*bias);
	brake.SetHandbrake(handbrake);

	return true;
}

template <typename T>
bool LoadTireParameters(
	const CONFIGFILE & c,
	CARTIREINFO <T> & info,
	std::ostream & error_output)
{
	//read lateral
	int numinfile;
	for (int i = 0; i < 15; i++)
	{
		numinfile = i;
		if (i == 11)
			numinfile = 111;
		else if (i == 12)
			numinfile = 112;
		else if (i > 12)
			numinfile -= 1;
		std::stringstream str;
		str << "a" << numinfile;
		float value;
		if (!c.GetParam(str.str(), value, error_output)) return false;
		info.lateral[i] = value;
	}

	//read longitudinal, error_output)) return false;
	for (int i = 0; i < 11; i++)
	{
		std::stringstream str;
		str << "b" << i;
		float value;
		if (!c.GetParam(str.str(), value, error_output)) return false;
		info.longitudinal[i] = value;
	}

	//read aligning, error_output)) return false;
	for (int i = 0; i < 18; i++)
	{
		std::stringstream str;
		str << "c" << i;
		float value;
		if (!c.GetParam(str.str(), value, error_output)) return false;
		info.aligning[i] = value;
	}

	float rolling_resistance[3];
	if (!c.GetParam("rolling-resistance", rolling_resistance, error_output)) return false;
	info.rolling_resistance_linear = rolling_resistance[0];
	info.rolling_resistance_quadratic = rolling_resistance[1];

	if (!c.GetParam("tread", info.tread, error_output)) return false;

	return true;
}

template <typename T>
bool LoadTire(
	const CONFIGFILE & c,
	const std::string & tirename,
	const std::string & sharedpartspath,
	CARTIRE<T> & tire,
	std::ostream & error_output)
{
	std::string tiresize;
	std::string tiretype;
	CONFIGFILE tc;
	CARTIREINFO <T> info;
	float section_width(0);
	float aspect_ratio(0);
	float rim_diameter(0);
	
	if (!c.GetParam(tirename+".size", tiresize, error_output)) return false;
	if (!c.GetParam(tirename+".type", tiretype, error_output)) return false;
	if (!tc.Load(sharedpartspath+"/tire/"+tiretype)) return false;
	if (!LoadTireParameters(tc, info, error_output)) return false;
	
	// tire dimensions
	std::string modsize = tiresize;
	for (unsigned int i = 0; i < modsize.length(); i++)
	{
		if (modsize[i] < '0' || modsize[i] > '9')
			modsize[i] = ' ';
	}
	std::stringstream parser(modsize);
	parser >> section_width >> aspect_ratio >> rim_diameter;
	if (section_width <= 0 || aspect_ratio <= 0 || rim_diameter <= 0)
	{
		error_output << "Error parsing " << tirename
					<< ".size, expected something like 225/50r16 but got: "
					<< tiresize << std::endl;
		return false;
	}
	
	info.radius = section_width * 0.001 * aspect_ratio * 0.01 + rim_diameter * 0.0254 * 0.5;
	info.sidewall_width = section_width * 0.001;
	info.aspect_ratio = aspect_ratio * 0.01;
	tire.Init(info);
	
	return true;
}

template <typename T>
bool LoadWheel(
	const CONFIGFILE & c,
	const std::string & wheelname,
	const std::string & partspath,
	CARWHEEL<T> & wheel,
	CARTIRE<T> & tire,
	CARBRAKE<T> & brake,
	std::ostream & error_output)
{
	std::string brakename;
	std::string tirename;
	
	if (!c.GetParam(wheelname+".brake", brakename, error_output)) return false;
	if (!LoadBrake(c, brakename, brake, error_output)) return false;
	if (!c.GetParam(wheelname+".tire", tirename, error_output)) return false;
	if (!LoadTire(c, tirename, partspath, tire, error_output)) return false;
	
	// calculate wheel inertia/mass
	float tire_radius = tire.GetRadius();
	float tire_width = tire.GetSidewallWidth();
	float tire_thickness = 0.05;
	float tire_density = 8E3;
	
	float rim_radius = tire_radius - tire_width * tire.GetAspectRatio();
	float rim_width = tire_width;
	float rim_thickness = 0.01;
	float rim_density = 3E5;
	
	float tire_volume = tire_width * M_PI * tire_thickness * tire_thickness * (2 * tire_radius  - tire_thickness);
	float rim_volume = rim_width * M_PI * rim_thickness * rim_thickness * (2 * rim_radius - rim_thickness);
	float tire_mass = tire_density * tire_volume;
	float rim_mass = rim_density * rim_volume;
	float tire_inertia = tire_mass * tire_radius * tire_radius;
	float rim_inertia = rim_mass * rim_radius * rim_radius;
	
	wheel.SetMass(tire_mass + rim_mass);
	wheel.SetInertia((tire_inertia + rim_inertia)*4); // scale inertia fixme

	return true;
}

template <typename T>
bool CARPOWERTRAIN<T>::Load(CONFIGFILE & c, const std::string & partspath, std::ostream & error_output)
{
	if (!LoadClutch(c, clutch, error_output)) return false;
	if (!LoadTransmission(c, transmission, error_output)) return false;
	if (!LoadEngine(c, engine, error_output)) return false;
	if (!LoadFuelTank(c, fuel_tank, error_output)) return false;

	// load wheels
	for (int i = 0; i < WHEEL_POSITION_SIZE; i++)
	{
		std::stringstream num;
		num << i;
		const std::string wheelname("wheel-"+num.str());
		if (!LoadWheel(c, wheelname, partspath, wheel[i], tire[i], brake[i], error_output)) return false;
	}

	//load the differential(s)
	{
		float final_drive, anti_slip, anti_slip_torque(0), anti_slip_torque_deceleration_factor(0);

		if (!c.GetParam("differential.final-drive", final_drive, error_output)) return false;
		if (!c.GetParam("differential.anti-slip", anti_slip, error_output)) return false;
		c.GetParam("differential.anti-slip-torque", anti_slip_torque);
		c.GetParam("differential.anti-slip-torque-deceleration-factor", anti_slip_torque_deceleration_factor);

		std::string drivetype;
		if (!c.GetParam("drive", drivetype, error_output)) return false;
		SetDrive(drivetype);

		if (drivetype == "RWD")
		{
			rear_differential.SetFinalDrive(final_drive);
			rear_differential.SetAntiSlip(anti_slip, anti_slip_torque, anti_slip_torque_deceleration_factor);
		}
		else if (drivetype == "FWD")
		{
			front_differential.SetFinalDrive(final_drive);
			front_differential.SetAntiSlip(anti_slip, anti_slip_torque, anti_slip_torque_deceleration_factor);
		}
		else if (drivetype == "AWD")
		{
			rear_differential.SetFinalDrive(1.0);
			rear_differential.SetAntiSlip(anti_slip, anti_slip_torque, anti_slip_torque_deceleration_factor);
			front_differential.SetFinalDrive(1.0);
			front_differential.SetAntiSlip(anti_slip, anti_slip_torque, anti_slip_torque_deceleration_factor);
			center_differential.SetFinalDrive(final_drive);
			center_differential.SetAntiSlip(anti_slip, anti_slip_torque, anti_slip_torque_deceleration_factor);
		}
		else
		{
			error_output << "Unknown drive type: " << drive << std::endl;
			return false;
		}
	}

	return true;
}

// explicit instantiation
template class CARPOWERTRAIN <float>;
template class CARPOWERTRAIN <double>;