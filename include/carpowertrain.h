#ifndef CARPOWERTRAIN_H
#define CARPOWERTRAIN_H

#include "carengine.h"
#include "carclutch.h"
#include "cartransmission.h"
#include "cardifferential.h"
#include "carfueltank.h"
#include "carbrake.h"
#include "carwheel.h"
#include "cartire.h"
#include "carwheelposition.h"

#include <string>
#include <ostream>

class CONFIGFILE;

template <typename T>
class CARPOWERTRAIN
{
public:
	CARPOWERTRAIN();
	~CARPOWERTRAIN();

	bool Load(CONFIGFILE & c, const std::string & partspath, std::ostream & error_output);
	
	void StartEngine();
	void ShiftGear(int value);
	void SetThrottle(float value);
	void SetClutch(float value);
	void SetBrake(float value);
	void SetHandBrake(float value);
	void SetAutoClutch(bool value);
	void SetAutoShift(bool value);
	void SetABS(bool value);
	void SetTCS(bool value);

	T GetSpeedMPS() const;
	T GetTachoRPM() const;
	bool GetABSEnabled() const;
	bool GetABSActive() const;
	bool GetTCSEnabled() const;
	bool GetTCSActive() const;
	bool WheelDriven(int i) const;

	const CARFUELTANK <T> & GetFuelTank() const {return fuel_tank;}
	const CARENGINE <T> & GetEngine() const {return engine;}
	const CARCLUTCH <T> & GetClutch() const {return clutch;}
	const CARTRANSMISSION <T> & GetTransmission() const {return transmission;}
	const CARBRAKE <T> & GetBrake(WHEEL_POSITION i) const {return brake[i];}
	const CARWHEEL <T> & GetWheel(WHEEL_POSITION i) const {return wheel[i];}
	const CARTIRE <T> & GetTire(WHEEL_POSITION i) const {return tire[i];}

	/// update transmission, fuel, tacho (can be run slower than driveline functions)
	void Update(T dt);

	/// update engine, return wheel drive torque
	void UpdateDriveline(T drive_torque[], T dt);

	/// return wheel shaft torques
	void IntegrateEngine(T drive_torque[], T dt);

	/// calculate tire friction force
	MATHVECTOR <T, 3> GetTireForce(
		WHEEL_POSITION i,
		const T normal_force,
		const T friction_coeff,
		const MATHVECTOR <T, 3> & velocity,
		const T ang_velocity,
		const T inclination);

	/// return wheel torque
	T IntegrateWheel(
		WHEEL_POSITION i,
		T tire_torque,
		T drive_torque,
		T dt);

	/// traction control system calculations and modify the throttle position if necessary
	void UpdateTCS(int i, T normal_force);

	/// anti-lock brake system calculations and modify the brake force if necessary
	void UpdateABS(int i, T normal_force);

	/// print debug info to the given ostream
	void DebugPrint(std::ostream & out, bool p1, bool p2) const;

	bool Serialize(joeserialize::Serializer & s);

private:
	CARFUELTANK <T> fuel_tank;
	CARENGINE <T> engine;
	CARCLUTCH <T> clutch;
	CARTRANSMISSION <T> transmission;
	CARDIFFERENTIAL <T> front_differential;
	CARDIFFERENTIAL <T> rear_differential;
	CARDIFFERENTIAL <T> center_differential;
	std::vector < CARBRAKE <T> > brake;
	std::vector < CARWHEEL <T> > wheel;
	std::vector < CARTIRE <T> > tire;

	enum {FWD = 3, RWD = 12, AWD = 15} drive;
	T driveshaft_rpm;
	T tacho_rpm;

	bool autoclutch;
	bool autoshift;
	bool shifted;
	int shift_gear;
	T last_auto_clutch;
	T remaining_shift_time;

	bool abs;
	bool tcs;
	std::vector <int> abs_active;
	std::vector <int> tcs_active;

	/// update transmission state
	void UpdateTransmission(T dt);
	
	/// calculate wheel drive torque
	void CalculateDriveTorque(T wheel_drive_torque[], T clutch_torque);

	/// calculate driveshaft speed given wheel angular velocity
	T CalculateDriveshaftSpeed();

	/// calculate clutch driveshaft rpm
	T CalculateDriveshaftRPM() const;

	T AutoClutch(T last_clutch, T dt) const;

	T ShiftAutoClutch() const;

	T ShiftAutoClutchThrottle(T throttle, T dt);

	/// calculate next gear based on engine rpm
	int NextGear() const;

	/// calculate downshift point based on gear, engine rpm
	T DownshiftRPM(int gear) const;

	/// FWD, RWD or AWD
	void SetDrive(const std::string & value);
};

#endif // CARPOWERTRAIN_H
