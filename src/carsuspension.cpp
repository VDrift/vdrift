#include "carsuspension.h"
#include "coordinatesystems.h"

template <typename T>
CARSUSPENSIONINFO<T>::CARSUSPENSIONINFO()
{
	spring_constant = 50000.0;
	bounce = 2500;
	rebound = 4000;
	travel = 0.2;
	anti_roll = 8000;
	damper_factors = 1;
	spring_factors = 1;
	max_steering_angle = 0;
	ackermann = 0; 
	camber = 0;
	caster = 0;
	toe = 0;
}

template <typename T>
void CARSUSPENSIONINFO<T>::SetDamperFactorPoints(std::vector <std::pair <T, T> > & curve)
{
	for (typename std::vector <std::pair <T, T> >::iterator i = curve.begin(); i != curve.end(); ++i)
	{
		damper_factors.AddPoint(i->first, i->second);
	}
}

template <typename T>
void CARSUSPENSIONINFO<T>::SetSpringFactorPoints(std::vector <std::pair <T, T> > & curve)
{
	for (typename std::vector <std::pair <T, T> >::iterator i = curve.begin(); i != curve.end(); ++i)
	{
		spring_factors.AddPoint(i->first, i->second);
	}
}

template <typename T>
CARSUSPENSION<T>::CARSUSPENSION()
{
	steering_angle = 0;
	spring_force = 0;
	damp_force = 0;
	force = 0;
	
	overtravel = 0;
	displacement = 0;
	wheel_velocity = 0;
	wheel_force = 0;
}

template <typename T>
void CARSUSPENSION<T>::Init(const CARSUSPENSIONINFO <T> & info)
{
	this->info = info;
	position = info.extended_position;
	orientation.LoadIdentity();
}

template <typename T>
void CARSUSPENSION<T>::SetSteering(const T & value)
{
	T alpha = -value * info.max_steering_angle * M_PI / 180.0;
	steering_angle = 0.0;
	if(alpha != 0.0)
	{
		steering_angle = atan(1.0 / (1.0 / tan(alpha) - tan(info.ackermann * M_PI / 180.0)));
	}
	
	QUATERNION <T> s;
	s.Rotate(steering_angle, sin(-info.caster * M_PI / 180.0), 0, cos(-info.caster * M_PI / 180.0));
	
	QUATERNION <T> t;
	t.Rotate(info.toe * M_PI / 180.0, 0, 0, 1);
	
	QUATERNION <T> c;
	c.Rotate(-info.camber * M_PI / 180.0, 1, 0, 0);
	
	orientation = c * t * s;
}

template <typename T>
void CARSUSPENSION<T>::Update(T ext_mass, T ext_velocity, T ext_displacement, T dt)
{
	overtravel = 0;
	displacement = displacement + ext_displacement;
	T velocity = ext_velocity;
	if (displacement > info.travel)
	{
		overtravel = displacement - info.travel;
		displacement = info.travel;
	}
	else if (displacement < 0)
	{
		displacement = 0;
		velocity = 0;
	}
	
	T spring = info.spring_constant;
	T springfactor = info.spring_factors.Interpolate(displacement);
	spring_force = displacement * spring * springfactor; 
	
	T damping = (velocity > 0) ? info.bounce : info.rebound;
	T dampfactor = info.damper_factors.Interpolate(std::abs(velocity));
	damp_force = -velocity * damping * dampfactor;
	
	force = spring_force + damp_force;
	
	// limit damping force (should never add energy)
	T force_limit = -ext_velocity / dt * ext_mass;
	if (force < 0)
	{
		force = 0;
	}
	// limit damping
	else if (force_limit >= 0 && force > force_limit && spring_force < force_limit)
	{
		//std::cerr << "clamp damping: " << spring_force << " " << damp_force << " " << force_limit << std::endl;
		force = force_limit;
	}
	// suspension bump
	else if (force_limit >= 0 && force < force_limit && overtravel > 0)
	{
		//std::cerr << "bump: " << spring_force << " " << damp_force << " " << force_limit << std::endl;
		force = force_limit;
	}
	
	// update wheel position
	position = GetWheelPosition(displacement / info.travel);
}

template <typename T>
void CARSUSPENSION<T>::DebugPrint(std::ostream & out) const
{
	out << "---Suspension---" << "\n";
	out << "Displacement: " << displacement << "\n";
	out << "Force: " << force << "\n";
	out << "Steering angle: " << steering_angle * 180 / M_PI << "\n";
}

template <typename T>
class BASICSUSPENSION : public CARSUSPENSION<T> 
{
public:
	MATHVECTOR <T, 3> GetWheelPosition(T displacement_fraction)
	{
		//const
		MATHVECTOR <T, 3> up (0, 0, 1);
		MATHVECTOR <T, 3> relwheelext = CARSUSPENSION<T>::info.extended_position - hinge;
		MATHVECTOR <T, 3> rotaxis = up.cross(relwheelext.Normalize());
		T hingeradius = relwheelext.Magnitude();
		//const
		
		T displacementradians = displacement_fraction * CARSUSPENSION<T>::info.travel / hingeradius;
		QUATERNION <T> hingerotate;
		hingerotate.Rotate(-displacementradians, rotaxis[0], rotaxis[1], rotaxis[2]);
		MATHVECTOR <T, 3> localwheelpos = relwheelext;
		hingerotate.RotateVector(localwheelpos);
		return localwheelpos + hinge;
	}

	void Init(const CARSUSPENSIONINFO<T> & info, const std::vector<T> & h)
	{
		CARSUSPENSION<T>::Init(info);
		hinge.Set(h[0], h[1], h[2]);
	}

private: 
	MATHVECTOR <T, 3> hinge; ///< the point that the wheels are rotated around as the suspension compresses
};

template<typename T>
inline T angle_from_sides(T a, T h, T o)
{
	return acos((a * a + h * h - o * o) / (2 * a * h));
}

template<typename T>
inline T side_from_angle(T theta, T a, T h)
{
	return sqrt(a * a + h * h - 2 * a * h * cos(theta));
}

template <typename T>
class WISHBONESUSPENSION : public CARSUSPENSION<T>
{
public:
	MATHVECTOR<T, 3> GetWheelPosition(T displacement_fraction)
	{
		MATHVECTOR <T, 3> up (0, 0, 1);
		MATHVECTOR <T, 3> relucuh = (hinge[UPPER_HUB] - hinge[UPPER_CHASSIS]),
			rellclh = (hinge[LOWER_HUB] - hinge[LOWER_CHASSIS]),
			reluclh = (hinge[LOWER_HUB] - hinge[UPPER_CHASSIS]),
			rellcuh = (hinge[UPPER_HUB] - hinge[LOWER_CHASSIS]),
			reluclc = (hinge[LOWER_CHASSIS] - hinge[UPPER_CHASSIS]),
			rellhuh = (hinge[UPPER_HUB] - hinge[LOWER_HUB]),
			localwheelpos = (this->info.extended_position - hinge[LOWER_HUB]);
		
		T radlc = angle_from_sides(rellclh.Magnitude(), reluclc.Magnitude(), reluclh.Magnitude());
		T lrotrad = -angle_from_sides(rellclh.Magnitude(), rellclh.Magnitude(), displacement_fraction * this->info.travel);
		T radlcd = lrotrad - radlc;
		T duclh = side_from_angle(radlcd, rellclh.Magnitude(), reluclc.Magnitude());
		T radlh = angle_from_sides(rellclh.Magnitude(), rellhuh.Magnitude(), rellcuh.Magnitude());
		T dradlh = (angle_from_sides(rellclh.Magnitude(), duclh, reluclc.Magnitude()) +
					angle_from_sides(duclh, rellhuh.Magnitude(), relucuh.Magnitude()));
		
		MATHVECTOR<T, 3> axiswd = up.cross(rellhuh.Normalize());
		
		QUATERNION<T> hingerot;
		mountrot.LoadIdentity();
		hingerot.Rotate(lrotrad, axis[LOWER_CHASSIS][0], axis[LOWER_CHASSIS][1], axis[LOWER_CHASSIS][2]);
		mountrot.Rotate(radlh - dradlh, axiswd[0], axiswd[1], axiswd[2]);
		hingerot.RotateVector(rellclh);
		mountrot.RotateVector(localwheelpos);

		return (hinge[LOWER_CHASSIS] + rellclh + localwheelpos);	
	}

	void Init(
		const CARSUSPENSIONINFO<T> & info,
		const std::vector<T> & up_ch0, const std::vector<T> & up_ch1,
		const std::vector<T> & lo_ch0, const std::vector<T> & lo_ch1,
		const std::vector<T> & up_hub, const std::vector<T> & lo_hub)
	{
		static MATHVECTOR<T, 3> hingev[3];
		static T innerangle[2];

		CARSUSPENSION<T>::Init(info);

		hingev[0].Set(up_ch0[0], up_ch0[1], up_ch0[2]);
		hingev[1].Set(up_ch1[0], up_ch1[1], up_ch1[2]);
		hingev[2].Set(up_hub[0], up_hub[1], up_hub[2]);

		axis[UPPER_CHASSIS] = (hingev[1] - hingev[0]).Normalize();

		innerangle[0] = angle_from_sides(
								(hingev[2] - hingev[0]).Magnitude(),
								(hingev[1] - hingev[0]).Magnitude(),
								(hingev[2] - hingev[1]).Magnitude());

		hinge[UPPER_CHASSIS] = hingev[0] + (axis[UPPER_CHASSIS]	* 
				((hingev[2] - hingev[0]).Magnitude() * cos(innerangle[0])));

		if (up_hub[1] < hinge[UPPER_CHASSIS][1])
			axis[UPPER_CHASSIS] = -axis[UPPER_CHASSIS];

		hingev[0].Set(lo_ch0[0], lo_ch0[1], lo_ch0[2]);
		hingev[1].Set(lo_ch1[0], lo_ch1[1], lo_ch1[2]);
		hingev[2].Set(lo_hub[0], lo_hub[1], lo_hub[2]);

		axis[LOWER_CHASSIS] = (hingev[1] - hingev[0]).Normalize();

		innerangle[1] = angle_from_sides(
								(hingev[2] - hingev[0]).Magnitude(),
								(hingev[1] - hingev[0]).Magnitude(),
								(hingev[2] - hingev[1]).Magnitude());

		hinge[LOWER_CHASSIS] = hingev[0] + (axis[LOWER_CHASSIS]	* 
				((hingev[2] - hingev[0]).Magnitude() * cos(innerangle[1])));

		if (lo_hub[1] < hinge[LOWER_CHASSIS][1])			
			axis[LOWER_CHASSIS] = -axis[LOWER_CHASSIS];

		hinge[UPPER_HUB].Set(up_hub[0], up_hub[1], up_hub[2]);
		hinge[LOWER_HUB].Set(lo_hub[0], lo_hub[1], lo_hub[2]);

	}

	void SetSteering(const T & value)
	{
		CARSUSPENSION<T>::SetSteering(value);
		this->orientation = this->orientation * mountrot;
	}

private:
	enum {
		UPPER_CHASSIS = 0,
		LOWER_CHASSIS,
		UPPER_HUB,
		LOWER_HUB
	};

	MATHVECTOR<T, 3> hinge[4], axis[2];
	QUATERNION<T> mountrot;
};

template <typename T>
class MACPHERSONSUSPENSION : public CARSUSPENSION<T>
{
public:
	MATHVECTOR<T, 3> GetWheelPosition(T displacement_fraction)
	{
		MATHVECTOR<T, 3> up (0, 0, 1);
		MATHVECTOR<T, 3> hinge_end = strut.end - strut.hinge;
		MATHVECTOR<T, 3> end_top = strut.top - strut.end;
		MATHVECTOR<T, 3> hinge_top = strut.top - strut.hinge;

		MATHVECTOR<T, 3> rotaxis = up.cross(hinge_end.Normalize());
		MATHVECTOR<T, 3> localwheelpos = this->info.extended_position - strut.end;

		T hingeradius = hinge_end.Magnitude();
		T disp_rad = asin(displacement_fraction * this->info.travel / hingeradius);

		QUATERNION<T> hingerotate;
		hingerotate.Rotate(-disp_rad, rotaxis[0], rotaxis[1], rotaxis[2]);

		T e_angle = angle_from_sides(end_top.Magnitude(), hinge_end.Magnitude(), hinge_top.Magnitude());
	
		hingerotate.RotateVector(hinge_end);

		T e_angle_disp = angle_from_sides(end_top.Magnitude(), hinge_end.Magnitude(), hinge_top.Magnitude());

		rotaxis = up.cross(end_top.Normalize());

		mountrot.LoadIdentity();
		mountrot.Rotate(e_angle_disp - e_angle, rotaxis[0], rotaxis[1], rotaxis[2]);
		mountrot.RotateVector(localwheelpos);

		return localwheelpos + strut.hinge + hinge_end;
	}

	void Init(
		const CARSUSPENSIONINFO<T> & info,
		const std::vector<T> & top,
		const std::vector<T> & end,
		const std::vector<T> & hinge)
	{
		CARSUSPENSION<T>::Init(info);
		strut.top.Set(top[0], top[1], top[2]);
		strut.end.Set(end[0], end[1], end[2]);
		strut.hinge.Set(hinge[0], hinge[1], hinge[2]);
	}

	void SetSteering(const T & value)
	{
		CARSUSPENSION<T>::SetSteering(value);
		this->orientation = this->orientation * mountrot;
	}

private:
	enum {
		UPPER_CHASSIS = 0,
		LOWER_CHASSIS,
		UPPER_HUB,
		LOWER_HUB
	};

	struct {
		MATHVECTOR<T, 3> top;
		MATHVECTOR<T, 3> end;
		MATHVECTOR<T, 3> hinge;
	} strut;

	QUATERNION<T> mountrot;
};

// 1-9 points
template <typename T>
static void LoadPoints(
	const CONFIG & c,
	const CONFIG::const_iterator & it,
	const std::string & name,
	std::vector <std::pair <T, T> > & points)
{
	int i = 1;
	std::stringstream s;
	s << std::setw(1) << i;
	std::vector<T> point(2);
	while(c.GetParam(it, name+s.str(), point) && i < 10)
	{
		s.clear();
		s << std::setw(1) << ++i;
		points.push_back(std::pair<T, T>(point[0], point[1]));
	}
}

template <typename T>
static bool LoadCoilover(
	const CONFIG & c,
	const CONFIG::const_iterator & iwheel,
	CARSUSPENSIONINFO <T> & info,
	std::ostream & error_output)
{
	std::string coilovername;
	if (!c.GetParam(iwheel, "coilover", coilovername, error_output)) return false;
	
	CONFIG::const_iterator it;
	if (!c.GetSection(coilovername, it, error_output)) return false;
	if (!c.GetParam(it, "spring-constant", info.spring_constant, error_output)) return false;
	if (!c.GetParam(it, "bounce", info.bounce, error_output)) return false;
	if (!c.GetParam(it, "rebound", info.rebound, error_output)) return false;
	if (!c.GetParam(it, "travel", info.travel, error_output)) return false;
	if (!c.GetParam(it, "anti-roll", info.anti_roll, error_output)) return false;
	
	std::vector<std::pair <T, T> > damper_factor_points;
	std::vector<std::pair <T, T> > spring_factor_points;
	LoadPoints(c, it, "damper-factor-", damper_factor_points);
	LoadPoints(c, it, "spring-factor-", spring_factor_points);
	info.SetDamperFactorPoints(damper_factor_points);
	info.SetSpringFactorPoints(spring_factor_points);
	
	return true;
}

template <typename T>
bool CARSUSPENSION<T>::LoadSuspension(
	const CONFIG & c,
	const CONFIG::const_iterator & iwheel,
	CARSUSPENSION<T> *& suspension,
	std::ostream & error_output)
{
	CARSUSPENSIONINFO<T> info;
	std::vector<T> p(3);
	std::string s_type = "basic";

	if (!LoadCoilover(c, iwheel, info, error_output)) return false;
	if (!c.GetParam(iwheel, "position", p, error_output)) return false;
	if (!c.GetParam(iwheel, "camber", info.camber, error_output)) return false;
	if (!c.GetParam(iwheel, "caster", info.caster, error_output)) return false;
	if (!c.GetParam(iwheel, "toe", info.toe, error_output)) return false;
	c.GetParam(iwheel, "steering", info.max_steering_angle);
	c.GetParam(iwheel, "ackermann", info.ackermann);

	COORDINATESYSTEMS::ConvertV2toV1(p[0], p[1], p[2]);
	info.extended_position.Set(p[0], p[1], p[2]);

	if(c.GetParam(iwheel, "macpherson-strut", s_type))
	{
		std::vector<T> strut_top(3), strut_end(3), hinge(3);
		CONFIG::const_iterator iwb;

		if (!c.GetSection(s_type, iwb, error_output)) return false;
		if(!c.GetParam(iwb, "hinge", hinge, error_output)) return false;
		if(!c.GetParam(iwb, "strut-top", strut_top, error_output)) return false;
		if(!c.GetParam(iwb, "strut-end", strut_end))
		{
			strut_end = p;
		}
		else
		{
			COORDINATESYSTEMS::ConvertV2toV1(strut_end[0], strut_end[1], strut_end[2]);
		}

		COORDINATESYSTEMS::ConvertV2toV1(hinge[0], hinge[1], hinge[2]);
		COORDINATESYSTEMS::ConvertV2toV1(strut_top[0], strut_top[1], strut_top[2]);

		suspension = new MACPHERSONSUSPENSION<T>();
		dynamic_cast<MACPHERSONSUSPENSION<T>* >(suspension)->Init(
			info, strut_top, strut_end, hinge);
	}
	else if (c.GetParam(iwheel, "double-wishbone",  s_type))
	{
		std::vector<T> up_ch0(3), up_ch1(3), lo_ch0(3), lo_ch1(3), up_hub(3), lo_hub(3);
		CONFIG::const_iterator iwb;

		if (!c.GetSection(s_type, iwb, error_output)) return false;
		if (!c.GetParam(iwb, "upper-chassis-front", up_ch0, error_output)) return false;
		if (!c.GetParam(iwb, "upper-chassis-rear", up_ch1, error_output)) return false;
		if (!c.GetParam(iwb, "lower-chassis-front", lo_ch0, error_output)) return false;
		if (!c.GetParam(iwb, "lower-chassis-rear", lo_ch1, error_output)) return false;
		if (!c.GetParam(iwb, "upper-hub", up_hub, error_output)) return false;
		if (!c.GetParam(iwb, "lower-hub", lo_hub, error_output)) return false;

		COORDINATESYSTEMS::ConvertV2toV1(up_ch0[0], up_ch0[1], up_ch0[2]);
		COORDINATESYSTEMS::ConvertV2toV1(up_ch1[0], up_ch1[1], up_ch1[2]);
		COORDINATESYSTEMS::ConvertV2toV1(lo_ch0[0], lo_ch0[1], lo_ch0[2]);
		COORDINATESYSTEMS::ConvertV2toV1(lo_ch1[0], lo_ch1[1], lo_ch1[2]);
		COORDINATESYSTEMS::ConvertV2toV1(up_hub[0], up_hub[1], up_hub[2]);
		COORDINATESYSTEMS::ConvertV2toV1(lo_hub[0], lo_hub[1], lo_hub[2]);
		
		suspension = new WISHBONESUSPENSION<T>();
		dynamic_cast<WISHBONESUSPENSION<T> *>(suspension)->Init(
			info, up_ch0, up_ch1, lo_ch0, lo_ch1, up_hub, lo_hub);
	}
	else
	{
		std::vector<T> h(3);
		if (!c.GetParam(iwheel, "hinge", h, error_output)) return false;
		COORDINATESYSTEMS::ConvertV2toV1(h[0], h[1], h[2]);	
		
		suspension = new BASICSUSPENSION<T>();	
		dynamic_cast<BASICSUSPENSION<T>* >(suspension)->Init(info, h);
	}

	return true;
}

/// explicit instantiation
template class CARSUSPENSIONINFO <float>;
template class CARSUSPENSIONINFO <double>;
template class CARSUSPENSION <float>;
template class CARSUSPENSION <double>;
