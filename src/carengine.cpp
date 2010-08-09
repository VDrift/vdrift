#include "carengine.h"

#include "linearinterp.h"
#include "matrix3.h"

template <typename T>
CARENGINEINFO<T>::CARENGINEINFO()
{
	redline = 7800;
	rpm_limit = 9000;
	idle = 0.02;
	start_rpm = 1000;
	stall_rpm = 350;
	fuel_consumption = 1e-9;
	friction = 0.000328;
	inertia = 0.25;
	mass = 200;
}

template <typename T>
void CARENGINEINFO<T>::SetTorqueCurve(T redline, std::vector < std::pair <T, T> > torque)
{
	torque_curve.Clear();
	
	//this value accounts for the fact that the torque curves are usually measured
	// on a dyno, but we're interested in the actual crankshaft power
	const T dyno_correction_factor = 1.0;//1.14;
	
	assert(torque.size() > 1);
	
	//ensure we have a smooth curve down to 0 RPM
	if (torque[0].first != 0)
		torque_curve.AddPoint(0,0);
	
	for (typename std::vector <std::pair <T, T> >::iterator i = torque.begin(); i != torque.end(); ++i)
	{
		torque_curve.AddPoint(i->first, i->second*dyno_correction_factor);
	}
	
	//ensure we have a smooth curve for over-revs
	torque_curve.AddPoint(torque[torque.size()-1].first + 10000, 0);
	
	//write out a debug torque curve file
	/*std::ofstream f("out.dat");
	for (T i = 0; i < curve[curve.size()-1].first+1000; i+= 20) f << i << " " << torque_curve.Interpolate(i) << std::endl;*/
	//for (unsigned int i = 0; i < curve.size(); i++) f << curve[i].first << " " << curve[i].second << std::endl;
	
	//calculate engine friction
	T max_power_angvel = redline*3.14153/30.0;
	T max_power = torque_curve.Interpolate(redline) * max_power_angvel;
	friction = max_power / (max_power_angvel*max_power_angvel*max_power_angvel);
	
	//calculate idle throttle position
	for (idle = 0; idle < 1.0; idle += 0.01)
	{
		if (GetTorque(idle, start_rpm) > -GetFrictionTorque(start_rpm*3.141593/30.0, 1.0, idle))
		{
			//std::cout << "Found idle throttle: " << idle << ", " << GetTorqueCurve(idle, start_rpm) << ", " << friction_torque << std::endl;
			break;
		}
	}
}

template <typename T>
T CARENGINEINFO<T>::GetTorque(const T throttle, const T rpm) const
{
	if (rpm < 1) return 0.0;
	
	T torque = torque_curve.Interpolate(rpm);
	
	//make sure the real function only returns values > 0
	return torque * throttle;
}

template <typename T>
T CARENGINEINFO<T>::GetFrictionTorque(T angvel, T friction_factor, T throttle_position)
{
	T velsign = 1.0;
	if (angvel < 0)
		velsign = -1.0;
	T A = 0;
	T B = -1300*friction;
	T C = 0;
	return (A + angvel * B + -velsign * C * angvel * angvel) *
			(1.0 - friction_factor*throttle_position);
}

template <typename T>
CARENGINE<T>::CARENGINE()
{
	Init(this->info);
}

template <typename T>
void CARENGINE<T>::Init(const CARENGINEINFO<T> & info)
{
	this->info = info;
	
	MATRIX3<T> inertia;
	inertia.Scale(info.inertia);
	crankshaft.SetInertia(inertia);
	
	throttle_position = 0.0;
	clutch_torque = 0.0;
	out_of_gas = false;
	rev_limit_exceeded = false;
	
	friction_torque = 0;
	combustion_torque = 0;
	stalled = false;
}

template <typename T>
T CARENGINE<T>::ComputeForces(T clutch_drag, T clutch_angvel, T dt)
{
	clutch_torque = clutch_drag;
	
	// clamp clutch friction torque(static friction)
	MATHVECTOR<T,3> target_angvel(clutch_angvel, 0, 0);
	MATHVECTOR<T,3> torque_delta = crankshaft.GetTorque(target_angvel, dt);
	if ((clutch_torque > 0 && clutch_torque > torque_delta[0]) ||
		(clutch_torque < 0 && clutch_torque < torque_delta[0]))
	{
		clutch_torque = torque_delta[0];
	}
	
	stalled = (GetRPM() < info.stall_rpm);
	
	//make sure the throttle is at least idling
	if (throttle_position < info.idle)
		throttle_position = info.idle;
	
	//engine drive torque
	T friction_factor = 1.0; //used to make sure we allow friction to work if we're out of gas or above the rev limit
	T rev_limit = info.rpm_limit;
	if (rev_limit_exceeded)
		rev_limit -= 100.0; //tweakable
	
	if (GetRPM() < rev_limit)
		rev_limit_exceeded = false;
	else
		rev_limit_exceeded = true;
	
	combustion_torque = info.GetTorque(throttle_position, GetRPM());
	
	if (out_of_gas || rev_limit_exceeded || stalled)
	{
		friction_factor = 0.0;
		combustion_torque = 0.0;
	}

	T cur_angvel = crankshaft.GetAngularVelocity()[0];
	friction_torque = info.GetFrictionTorque(cur_angvel, friction_factor, throttle_position);
	if (stalled)
	{
		//try to model the static friction of the engine
		friction_torque *= 100.0;
	}

	MATHVECTOR <T, 3> total_torque(combustion_torque + friction_torque + clutch_torque, 0, 0);
	crankshaft.SetTorque(total_torque);
	
	return clutch_torque;
}

template <typename T>
void CARENGINE<T>::DebugPrint(std::ostream & out) const
{
	out << "---Engine---" << "\n";
	out << "Throttle position: " << throttle_position << "\n";
	out << "Combustion torque: " << combustion_torque << "\n";
	out << "Clutch torque: " << -clutch_torque << "\n";
	out << "Friction torque: " << friction_torque << "\n";
	out << "Total torque: " << GetTorque() << "\n";
	out << "RPM: " << GetRPM() << "\n";
	out << "Rev limit exceeded: " << rev_limit_exceeded << "\n";
	out << "Running: " << !stalled << "\n";
}

template <typename T>
bool CARENGINE<T>::Serialize(joeserialize::Serializer & s)
{
	_SERIALIZE_(s, throttle_position);
	_SERIALIZE_(s, clutch_torque);
	_SERIALIZE_(s, out_of_gas);
	_SERIALIZE_(s, rev_limit_exceeded);
	_SERIALIZE_(s, crankshaft);
	return true;
}

/// explicit instantiation
template struct CARENGINEINFO <float>;
template struct CARENGINEINFO <double>;
template class CARENGINE <float>;
template class CARENGINE <double>;
