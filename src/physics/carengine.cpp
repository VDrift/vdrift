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

#include "carengine.h"
#include "cfg/ptree.h"
#include "linearinterp.h"

CarEngineInfo::CarEngineInfo():
	displacement(2E-3),
	maxpower(184000),
	redline(7800),
	rpm_limit(9000),
	idle_throttle(0.02),
	idle_throttle_slope(0),
	start_rpm(1000),
	stall_rpm(350),
	fuel_rate(4E7),
	friction{15.438, 2.387, 0.7958},
	inertia(0.25),
	mass(200),
	nos_mass(0),
	nos_boost(0),
	nos_fuel_ratio(5)
{
	// ctor
}

bool CarEngineInfo::Load(const PTree & cfg, std::ostream & error_output)
{
	std::vector<btScalar> pos(3, 0.0f);
	if (!cfg.get("displacement", displacement, error_output)) return false;
	if (!cfg.get("max-power", maxpower, error_output)) return false;
	if (!cfg.get("peak-engine-rpm", redline, error_output)) return false;
	if (!cfg.get("rpm-limit", rpm_limit, error_output)) return false;
	if (!cfg.get("inertia", inertia, error_output)) return false;
	if (!cfg.get("start-rpm", start_rpm, error_output)) return false;
	if (!cfg.get("stall-rpm", stall_rpm, error_output)) return false;
	if (!cfg.get("position", pos, error_output)) return false;
	if (!cfg.get("mass", mass, error_output)) return false;

	position.setValue(pos[0], pos[1], pos[2]);

	// fuel consumption
	btScalar fuel_heating_value = 4.5E7; // Ws/kg
	btScalar engine_efficiency = 0.35;
	cfg.get("fuel-heating-value", fuel_heating_value);
	cfg.get("efficiency", engine_efficiency);
	fuel_rate = 1 /	(engine_efficiency * fuel_heating_value);

	// nos parameters
	cfg.get("nos-mass", nos_mass);
	cfg.get("nos-boost", nos_boost);
	cfg.get("nos-ratio", nos_fuel_ratio);

	// friction (Heywood 1988 tfmep)
	friction[0] = 97000 / (4 * M_PI) * displacement;
	friction[1] = 15000 / (4 * M_PI) * displacement;
	friction[2] =  5000 / (4 * M_PI) * displacement;
	std::vector<btScalar> f(3, 0.0f);
	if (cfg.get("torque-friction", f))
	{
		friction[0] = f[0];
		friction[1] = f[1];
		friction[2] = f[2];
	}

	// torque
	int curve_num = 0;
	std::vector<btScalar> torque_point(2);
	std::string torque_str("torque-curve-00");
	std::vector<std::pair<btScalar, btScalar> > torque;
	while (cfg.get(torque_str, torque_point))
	{
		torque.push_back(std::pair<btScalar, btScalar>(torque_point[0], torque_point[1]));

		curve_num++;
		std::ostringstream s;
		s << "torque-curve-";
		s.width(2);
		s.fill('0');
		s << curve_num;
		torque_str = s.str();
	}
	if (torque.size() <= 1)
	{
		error_output << "You must define at least 2 torque curve points." << std::endl;
		return false;
	}

	// set torque curve
	torque_curve.Clear();
	if (torque[0].first > stall_rpm)
	{
		btScalar dx = torque[1].first - torque[0].first;
		btScalar dy = torque[1].second - torque[0].second;
		btScalar stall_torque = dy / dx * (stall_rpm - torque[0].first) + torque[0].second;
		torque_curve.AddPoint(stall_rpm,  stall_torque);

		error_output << "Torque curve begins above stall rpm.\n"
			<< "Extrapolating to " << stall_rpm << ", " << stall_torque << std::endl;
	}
	for (std::vector<std::pair<btScalar, btScalar> >::const_iterator i = torque.begin(); i != torque.end(); ++i)
	{
		torque_curve.AddPoint(i->first, i->second);
	}
	if (torque[torque.size() - 1].first < rpm_limit)
	{
		btScalar r = torque[torque.size() - 1].first + 10000.0f;
		btScalar t = 0.0f;
		torque_curve.AddPoint(r , t);

		error_output << "Torque curve ends below rpm limit.\n"
			<< "Extrapolating to " << r << ", " << t << std::endl;
	}

	// calculate idle throttle position
	for (idle_throttle = 0.0f; idle_throttle < 1.0f; idle_throttle += 0.01f)
	{
		if (GetTorque(idle_throttle, start_rpm) > -GetFrictionTorque(idle_throttle, start_rpm))
			break;
	}

	// calculate idle throttle slope
	btScalar stall_throttle;
	for (stall_throttle = idle_throttle; stall_throttle < 1.0f; stall_throttle += 0.01f)
	{
		if (GetTorque(stall_throttle, stall_rpm) > -GetFrictionTorque(stall_throttle, stall_rpm))
			break;
	}
	idle_throttle_slope = 1.5f * (idle_throttle - stall_throttle) / (start_rpm - stall_rpm);

	return true;
}

btScalar CarEngineInfo::GetTorque(const btScalar throttle, const btScalar rpm) const
{
	if (rpm < 1) return 0.0;
	return torque_curve.Interpolate(rpm) * throttle;
}

btScalar CarEngineInfo::GetFrictionTorque(btScalar throttle, btScalar rpm) const
{
	btScalar s = rpm < 0 ? -1 : 1;
	btScalar r = s * rpm * 0.001;
	btScalar f = friction[0] + friction[1] * r + friction[2] * r * r;
	return -s * f * (1.0 - throttle);
}

CarEngine::CarEngine()
{
	Init(info);
}

void CarEngine::Init(const CarEngineInfo & info)
{
	this->info = info;
	shaft.inv_inertia = 1 / info.inertia;
	combustion_torque = 0;
	friction_torque = 0;
	clutch_torque = 0;

	throttle_position = 0;
	nos_boost_factor = 0;
	nos_mass = info.nos_mass;
	out_of_gas = false;
	rev_limit_exceeded = false;
	stalled = false;
}

btScalar CarEngine::Integrate(btScalar clutch_drag, btScalar clutch_angvel, btScalar dt)
{
	btScalar rpm = GetRPM();

	clutch_torque = clutch_drag;

	btScalar torque_limit = shaft.getMomentum(clutch_angvel) / dt;
	if ((clutch_torque > 0 && clutch_torque > torque_limit) ||
		(clutch_torque < 0 && clutch_torque < torque_limit))
	{
		clutch_torque = torque_limit;
	}

	stalled = (rpm < info.stall_rpm);

	//make sure the throttle is at least idling
	btScalar idle_position = info.idle_throttle + info.idle_throttle_slope * (rpm - info.start_rpm);
	if (throttle_position < idle_position)
		throttle_position = idle_position;

	//engine drive torque
	btScalar rev_limit = info.rpm_limit;
	if (rev_limit_exceeded)
		rev_limit -= 100.0; //tweakable
	rev_limit_exceeded = rpm > rev_limit;

	combustion_torque = info.GetTorque(throttle_position, rpm);

	//nitrous injection
	if (nos_mass > 0 && nos_boost_factor > 0)
	{
		btScalar boost = nos_boost_factor * info.nos_boost;
		combustion_torque += boost / shaft.ang_velocity;

		btScalar fuel_consumed = boost * info.fuel_rate * dt;
		btScalar nos_consumed = info.nos_fuel_ratio * fuel_consumed;
		nos_mass = btMax(btScalar(0), nos_mass - nos_consumed);
	}

	if (out_of_gas || rev_limit_exceeded || stalled)
		combustion_torque = 0.0;

	friction_torque = info.GetFrictionTorque(throttle_position, rpm);

	//try to model the static friction of the engine
	if (stalled)
		friction_torque *= 2;

	btScalar total_torque = combustion_torque + friction_torque + clutch_torque;
	shaft.applyMomentum(total_torque * dt);

	return clutch_torque;
}

void CarEngine::DebugPrint(std::ostream & out) const
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

bool CarEngine::Serialize(joeserialize::Serializer & s)
{
	_SERIALIZE_(s, shaft.ang_velocity);
	_SERIALIZE_(s, throttle_position);
	_SERIALIZE_(s, clutch_torque);
	_SERIALIZE_(s, out_of_gas);
	_SERIALIZE_(s, rev_limit_exceeded);
	return true;
}
