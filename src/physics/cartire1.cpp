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

#include "cartire1.h"
#include "cartirebase.h"
#include "fastmath.h"
#include <cassert>

static const btScalar deg2rad = M_PI / 180;
static const btScalar rad2deg = 180 / M_PI;

template <class T, int N>
constexpr int size(const T (&array)[N])
{
	return N;
}

CarTire1::CarTire1() :
	longitudinal(),
	lateral(),
	aligning(),
	combining(),
	rolling_resistance_quad(1E-6),
	rolling_resistance_lin(1E-3),
	tread(0)
{
	// ctor
}

void CarTire1::ComputeState(
		btScalar normal_force,
		btScalar rot_velocity,
		btScalar lon_velocity,
		btScalar lat_velocity,
		CarTireState & s) const
{
	if (normal_force * s.friction < btScalar(1E-6))
	{
		s.slip = s.slip_angle = 0;
		s.fx = s.fy = s.mz = 0;
		return;
	}

	btScalar Fz = Min(normal_force * btScalar(1E-3), btScalar(30));

	btScalar slip, slip_angle;
	ComputeSlip(lon_velocity, lat_velocity, rot_velocity, slip, slip_angle);

	// sigma: longitudinal slip is negative when braking, positive for acceleration
	// alpha: sideslip angle is positive in a right turn(opposite to SAE tire coords)
	// gamma: positive when tire top tilts to the right, viewed from rear in deg
	btScalar sigma = slip;
	btScalar alpha = slip_angle * rad2deg;
	btScalar gamma = s.camber * rad2deg;

	// pure slip
	btScalar camber_alpha;
	btScalar Fx0 = PacejkaFx(sigma, Fz, s.friction);
	btScalar Fy0 = PacejkaFy(alpha, Fz, gamma, s.friction, camber_alpha);
	//btScalar Mz = PacejkaMz(alpha, Fz, gamma, s.friction);

	// combined slip
	btScalar Gx = PacejkaGx(slip, slip_angle);
	btScalar Gy = PacejkaGy(slip, slip_angle);
	btScalar Fx = Gx * Fx0;
	btScalar Fy = Gy * Fy0;

	s.vcam = ComputeCamberVelocity(camber_alpha * deg2rad, lon_velocity);
	s.slip = slip;
	s.slip_angle = slip_angle;
	s.fx = Fx;
	s.fy = Fy;
	//s.mz = Mz;
}

void CarTire1::ComputeAligningTorque(
		btScalar normal_force,
		CarTireState & s) const
{
	if (normal_force * s.friction < btScalar(1E-6))
	{
		s.mz = 0;
		return;
	}
	btScalar Fz = Min(normal_force * btScalar(1E-3), btScalar(30));
	btScalar alpha = s.slip_angle * rad2deg;
	btScalar gamma = s.camber * rad2deg;
	btScalar Mz = PacejkaMz(alpha, Fz, gamma, s.friction);
	s.mz = Mz;
}

btScalar CarTire1::getRollingResistance(btScalar velocity, btScalar resistance_factor) const
{
	// surface influence on rolling resistance
	btScalar resistance = rolling_resistance_lin * resistance_factor;

	// heat due to tire deformation increases rolling resistance
	// approximate by quadratic function
	resistance += velocity * velocity * rolling_resistance_quad;

	return resistance;
}

btScalar CarTire1::getMaxFx(btScalar load) const
{
	auto & b = longitudinal;
	btScalar Fz = load * btScalar(1E-3);
	btScalar D = (b[1] * Fz + b[2]) * Fz;
	return D;
}

btScalar CarTire1::getMaxDx(btScalar load) const
{
	auto & b = longitudinal;
	btScalar Fz = load * btScalar(1E-3);
	btScalar BCD = (b[3] * Fz + b[4]) * Fz * std::exp(-b[5] * Fz);
	return BCD;
}

btScalar CarTire1::getMaxFy(btScalar load, btScalar camber) const
{
	auto & a = lateral;
	btScalar Fz = load * btScalar(1E-3);
	btScalar gamma = camber;
	btScalar D = (a[1] * Fz + a[2]) * Fz;
	btScalar Sv = ((a[11] * Fz + a[12]) * gamma + a[13] ) * Fz + a[14];
	return D + Sv;
}

btScalar CarTire1::getMaxDy(btScalar load, btScalar camber) const
{
	auto & a = lateral;
	btScalar Fz = load * btScalar(1E-3);
	btScalar gamma = camber;
	btScalar BCD = a[3] * Sin2Atan(Fz, a[4]) * (1 - a[5] * std::abs(gamma));
	return BCD;
}

void CarTire1::getCamberShift(btScalar load, btScalar camber, btScalar & sh, btScalar & sv) const
{
	auto & a = lateral;
	btScalar Fz = load * btScalar(1E-3);
	btScalar gamma = camber;
	sh = a[8] * gamma + a[9] * Fz + a[10];
	sv = ((a[11] * Fz + a[12]) * gamma + a[13]) * Fz + a[14];
}

btScalar CarTire1::getMaxMz(btScalar load, btScalar camber) const
{
	auto & c = aligning;
	btScalar Fz = load * btScalar(1E-3);
	btScalar gamma = camber;
	btScalar D = (c[1] * Fz + c[2]) * Fz;
	btScalar Sv = (c[14] * Fz * Fz + c[15] * Fz) * gamma + c[16] * Fz + c[17];
	return -(D + Sv);
}

void CarTire1::PacejkaParamFx(btScalar Fz, btScalar p[6]) const
{
	auto & b = longitudinal;

	// shape factor C
	p[1] = b[0];

	// peak factor D
	p[2] = (b[1] * Fz + b[2]) * Fz;

	// slope at zero slip
	auto bcd = (b[3] * Fz + b[4]) * Fz * std::exp(-b[5] * Fz);

	// stiffness factor B
	p[0] =  bcd / (p[1] * p[2]);

	// curvature factor E
	p[3] = (b[6] * Fz + b[7]) * Fz + b[8];

	// horizontal shift Sh
	p[4] = b[9] * Fz + b[10];

	// vertical shift Sv
	p[5] = 0;
}

void CarTire1::PacejkaParamFy(btScalar Fz, btScalar gamma, btScalar p[6]) const
{
	auto & a = lateral;

	// shape factor C
	p[1] = a[0];

	// peak factor D
	p[2] = (a[1] * Fz + a[2]) * Fz;

	// slope at zero slip
	auto bcd = a[3] * Sin2Atan(Fz, a[4]) * (1 - a[5] * std::abs(gamma));

	// stiffness factor B
	p[0] = bcd / (p[1] * p[2]);

	// curvature factor E
	p[3] = a[6] * Fz + a[7];

	// horizontal shift Sh
	p[4] = a[8] * gamma + a[9] * Fz + a[10];

	// vertical shift Sv
	p[5] = ((a[11] * Fz + a[12]) * gamma + a[13]) * Fz + a[14];
}

btScalar CarTire1::Pacejka(const btScalar p[6], btScalar s) const
{
	btScalar BS = p[0] * (s + p[4]);
	return p[2] * Sin3Pi2(p[1] * Atan(BS - p[3] * (BS - Atan(BS)))) + p[5];
}

btScalar CarTire1::PacejkaFx(btScalar sigma, btScalar Fz, btScalar friction_coeff) const
{
	auto & b = longitudinal;

	// shape factor
	btScalar C = b[0];

	// peak factor
	btScalar D = (b[1] * Fz + b[2]) * Fz;

	// stiffness at sigma = 0
	btScalar BCD = (b[3] * Fz + b[4]) * Fz * std::exp(-b[5] * Fz);

	// stiffness factor
	btScalar B =  BCD / (C * D);

	// curvature factor
	btScalar E = (b[6] * Fz + b[7]) * Fz + b[8];

	// horizontal shift
	btScalar Sh = b[9] * Fz + b[10];

	// composite
	btScalar S = 100 * sigma + Sh;

	// longitudinal force
	btScalar BS = B * S;
	btScalar Fx = D * Sin3Pi2(C * Atan(BS - E * (BS - Atan(BS))));

	// scale by surface friction
	Fx = Fx * friction_coeff;

	btAssert(Fx == Fx);
	return Fx;
}

btScalar CarTire1::PacejkaFy(btScalar alpha, btScalar Fz, btScalar gamma, btScalar friction_coeff, btScalar & camber_alpha) const
{
	auto & a = lateral;

	// shape factor
	btScalar C = a[0];

	// peak factor
	btScalar D = (a[1] * Fz + a[2]) * Fz;

	// stiffness at alpha = 0
	btScalar BCD = a[3] * Sin2Atan(Fz, a[4]) * (1 - a[5] * std::abs(gamma));

	// stiffness factor
	btScalar B = BCD / (C * D);

	// curvature factor
	btScalar E = a[6] * Fz + a[7];

	// horizontal shift
	btScalar Sh = a[8] * gamma + a[9] * Fz + a[10];

	// vertical shift
	btScalar Sv = ((a[11] * Fz + a[12]) * gamma + a[13]) * Fz + a[14];

	// composite slip angle
	btScalar S = alpha + Sh;

	// lateral force
	btScalar BS = B * S;
	btScalar Fy = D * Sin3Pi2(C * Atan(BS - E * (BS - Atan(BS)))) + Sv;

	// scale by surface friction
	Fy = Fy * friction_coeff;
	camber_alpha = Sh + Sv / BCD * friction_coeff;

	btAssert(Fy == Fy);
	return Fy;
}

btScalar CarTire1::PacejkaMz(btScalar alpha, btScalar Fz, btScalar gamma, btScalar friction_coeff) const
{
	auto & c = aligning;

	// shape factor
	btScalar C = c[0];

	// peak factor
	btScalar D = (c[1] * Fz + c[2]) * Fz;

	// stiffness at alpha = 0
	btScalar BCD = (c[3] * Fz + c[4]) * Fz * (1 - c[6] * std::abs(gamma)) * std::exp(-c[5] * Fz);

	// stiffness factor
	btScalar B =  BCD / (C * D);

	// curvature factor
	btScalar E = (c[7] * Fz * Fz + c[8] * Fz + c[9]) * (1 - c[10] * std::abs(gamma));

	// horizontal shift
	btScalar Sh = c[11] * gamma + c[12] * Fz + c[13];

	// composite slip angle
	btScalar S = alpha + Sh;

	// vertical shift
	btScalar Sv = (c[14] * Fz * Fz + c[15] * Fz) * gamma + c[16] * Fz + c[17];

	// self-aligning torque
	btScalar BS = B * S;
	btScalar Mz = D * Sin3Pi2(C * Atan(BS - E * (BS - Atan(BS)))) + Sv;

	// scale by surface friction
	Mz = Mz * friction_coeff;

	btAssert(Mz == Mz);
	return Mz;
}

btScalar CarTire1::PacejkaGx(btScalar sigma, btScalar alpha) const
{
	auto & p = combining;
	//btScalar B = p[2] * CosAtan(p[3] * sigma);
	//btScalar G = CosAtan(B * alpha);
	//return G;
	btScalar a = p[3] * sigma;
	btScalar b = p[2] * alpha;
	btScalar c = a * a + 1;
	return std::sqrt(c / (c + b * b));
}

btScalar CarTire1::PacejkaGy(btScalar sigma, btScalar alpha) const
{
	auto & p = combining;
	//btScalar B = p[0] * CosAtan(p[1] * alpha);
	//btScalar G = CosAtan(B * sigma);
	//return G;
	btScalar a = p[1] * alpha;
	btScalar b = p[0] * sigma;
	btScalar c = a * a + 1;
	return std::sqrt(c / (c + b * b));
}

// solve PI/2 = C * Atan(BS - E * (BS - Atan(BS)))
btScalar FindPacejkaMax(const btScalar p[6])
{
	btScalar pc = btScalar(M_PI_2) / p[1];
	btAssert(pc < btScalar(M_PI_2));
	btScalar tc = TanPi2(pc);
	btScalar bs = tc;
	for (int i = 0; i < 8; ++i)
	{
		// F = BS - E * (BS - Atan(BS)) - tan(PI/2C);
		// dF = 1 - E + 1 / (1 + BS * BS);
		// dBS = F / dF
		btScalar t = 1 + bs * bs;
		btScalar dbs = (bs - tc - p[3] * (bs - Atan(bs))) * t / ((1 - p[3]) * t + 1);
		if (std::abs(dbs) < btScalar(1E-5))
			break;
		bs -= dbs; // newton raphson step
	}
	return bs / p[0] - p[4];
}

void CarTire1::findIdealSlip(btScalar load, btScalar output_slip[2], int iterations) const
{
	btScalar p[6];

	PacejkaParamFx(load, p);
	btScalar s = FindPacejkaMax(p) * btScalar(0.01);

	PacejkaParamFy(load, 0, p);
	btScalar a = FindPacejkaMax(p) * deg2rad;

	output_slip[0] = s;
	output_slip[1] = a;
}

void CarTire1::initSlipLUT(CarTireSlipLUT & t) const
{
	for (int i = 0; i < t.size(); i++)
	{
		btScalar load = (i + 1) * t.delta() * btScalar(1E-3);
		findIdealSlip(load, t.ideal_slip_lut[i]);
	}
}
