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
	btScalar alpha = RadToDeg(slip_angle);
	btScalar gamma = RadToDeg(s.camber);

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

	s.vcam = ComputeCamberVelocity(DegToRad(camber_alpha), lon_velocity);
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
	btScalar alpha = RadToDeg(s.slip_angle);
	btScalar gamma = RadToDeg(s.camber);
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

btScalar CarTire1::PacejkaFx(btScalar sigma, btScalar Fz, btScalar friction_coeff) const
{
	auto & b = longitudinal;

	// shape factor
	btScalar C = b[0];

	// peak factor
	btScalar D = (b[1] * Fz + b[2]) * Fz;

	// stifness at sigma = 0
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
	btScalar Fx = D * Sin3Pi2(C * Atan(B * S - E * (B * S - Atan(B * S))));

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

	// stifness at alpha = 0
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
	btScalar Fy = D * Sin3Pi2(C * Atan(B * S - E * (B * S - Atan(B * S)))) + Sv;

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

	// stifness at alpha = 0
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
	btScalar Mz = D * Sin3Pi2(c[0] * Atan(B * S - E * (B * S - Atan(B * S)))) + Sv;

	// scale by surface friction
	Mz = Mz * friction_coeff;

	btAssert(Mz == Mz);
	return Mz;
}

btScalar CarTire1::PacejkaGx(btScalar s, btScalar a) const
{
	return PacejkaG(&combining[2], s, a);
}

btScalar CarTire1::PacejkaGy(btScalar s, btScalar a) const
{
	return PacejkaG(combining, a, s);
}

void CarTire1::PacejkaParamFx(btScalar Fz, btScalar p[6]) const
{
	auto & b = longitudinal;

	// shape factor C
	p[1] = b[0];

	// peak factor D
	p[2] = (b[1] * Fz + b[2]) * Fz;

	// slope at zero slip
	btScalar bcd = (b[3] * Fz + b[4]) * Fz * std::exp(-b[5] * Fz);

	// stiffness factor B
	p[0] =  bcd / (p[1] * p[2]);

	// curvature factor E
	p[3] = Min((b[6] * Fz + b[7]) * Fz + b[8], btScalar(1));

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
	btScalar bcd = a[3] * Sin2Atan(Fz, a[4]) * (1 - a[5] * std::abs(gamma));

	// stiffness factor B
	p[0] = bcd / (p[1] * p[2]);

	// curvature factor E
	p[3] = Min(a[6] * Fz + a[7], btScalar(1));

	// horizontal shift Sh
	p[4] = a[8] * gamma + a[9] * Fz + a[10];

	// vertical shift Sv
	p[5] = ((a[11] * Fz + a[12]) * gamma + a[13]) * Fz + a[14];
}

void CarTire1::PacejkaParamMz(btScalar Fz, btScalar gamma, btScalar p[6]) const
{
	auto & c = aligning;

	// shape factor C
	p[1] = c[0];

	// peak factor D
	p[2] = (c[1] * Fz + c[2]) * Fz;

	// slope at zero
	auto bcd = (c[3] * Fz + c[4]) * Fz * (1 - c[6] * std::abs(gamma)) * std::exp(-c[5] * Fz);

	// stiffness factor B
	p[0] = bcd / (p[1] * p[2]);

	// curvature factor E
	p[3] = (c[7] * Fz * Fz + c[8] * Fz + c[9]) * (1 - c[10] * std::abs(gamma));

	// horizontal shift Sh
	p[4] = c[11] * gamma + c[12] * Fz + c[13];

	// vertical shift Sv
	p[5] = (c[14] * Fz * Fz + c[15] * Fz) * gamma + c[16] * Fz + c[17];
}

void CarTire1::findIdealSlip(btScalar load, btScalar output_slip[2], int iterations) const
{
	btScalar junk;
	btScalar fmax = 0;
	btScalar xmax = 1;
	btScalar dx = xmax / iterations;
	btScalar sigmahat = 0;
	for (btScalar x = 0; x < xmax; x += dx)
	{
		btScalar f = PacejkaFx(x, load, 1);
		if (f > fmax)
		{
			sigmahat = x;
			fmax = f;
		}
		else if (f < fmax && fmax > 0)
		{
			break;
		}
	}
	btAssert(fmax > 0);

	fmax = 0;
	xmax = 40;
	dx = xmax / iterations;
	btScalar alphahat = 0;
	for (btScalar x = 0; x < xmax; x += dx)
	{
		btScalar f = PacejkaFy(x, load, 0, 1, junk);
		if (f > fmax)
		{
			alphahat = x;
			fmax = f;
		}
		else if (f < fmax && fmax > 0)
		{
			break;
		}
	}
	btAssert(fmax > 0);

	output_slip[0] = sigmahat;
	output_slip[1] = DegToRad(alphahat);
}

void CarTire1::initSlipLUT(CarTireSlipLUT & t) const
{
	for (int i = 0; i < t.size(); i++)
	{
		btScalar load = (i + 1) * t.delta() * btScalar(1E-3);
		findIdealSlip(load, t.ideal_slip_lut[i]);
	}
}

btScalar PacejkaF(const btScalar p[6], btScalar s)
{
	btScalar bs = p[0] * (s + p[4]);
	btScalar g = bs - p[3] * (bs - Atan(bs));
	btScalar h = p[1] * Atan(g);
	return p[2] * Sin3Pi2(h) + p[5];
}

btScalar PacejkaF(const btScalar p[6], btScalar s, btScalar & df)
{
	btScalar bs = p[0] * (s + p[4]);
	btScalar g = bs - p[3] * (bs - Atan(bs));
	btScalar h = p[1] * Atan(g);
	btScalar j = (g * g + 1) * (bs * bs + 1);
	btScalar k = (1 - p[3]) * (bs * bs) + 1;
	btScalar bcd = p[0] * p[1] * p[2];
	df = Cos3Pi2(h) * (bcd * k) / j;
	return p[2] * Sin3Pi2(h) + p[5];
}

btScalar PacejkaG(const btScalar p[2], btScalar s, btScalar a)
{
	btScalar u = p[0] * a;
	btScalar v = p[1] * s;
	btScalar w = v * v + 1;
	return std::sqrt(w / (w + u * u));
}

btScalar PacejkaG(const btScalar p[2], btScalar s, btScalar a, btScalar & dgs)
{
	btScalar u = p[0] * a;
	btScalar v = p[1] * s;
	btScalar w = v * v + 1;
	btScalar n = u * u;
	btScalar m = w + n;
	btScalar g = std::sqrt(w / m);
	dgs = p[1] * v * n / (g * m * m);
	return g;
}
