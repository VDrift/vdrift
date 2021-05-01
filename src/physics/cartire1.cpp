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
#include "fastmath.h"
#include <cassert>

static const btScalar deg2rad = M_PI / 180;
static const btScalar rad2deg = 180 / M_PI;

template <class T, int N>
constexpr int size(const T (&array)[N])
{
	return N;
}

// approximate asin(x) = x + x^3/6 for +-18 deg range
inline btScalar ComputeCamberAngle(btScalar sin_camber)
{
	btScalar sc = Clamp(sin_camber, btScalar(-0.3), btScalar(0.3));
	return (btScalar(1/6.0) * sc) * (sc * sc) + sc;
}

CarTireInfo1::CarTireInfo1() :
	sigma_hat(),
	alpha_hat(),
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

void CarTire1::init(const CarTireInfo1 & info)
{
	CarTireInfo1::operator=(info);
	initSigmaHatAlphaHat();
}

btVector3 CarTire1::getForce(
	btScalar normal_force,
	btScalar friction_coeff,
	btScalar sin_camber,
	btScalar rot_velocity,
	btScalar lon_velocity,
	btScalar lat_velocity)
{
	if (normal_force * friction_coeff < btScalar(1E-6))
	{
		return btVector3(0, 0, 0);
	}

	btScalar Fz = Min(normal_force * btScalar(1E-3), btScalar(30));

	camber = ComputeCamberAngle(sin_camber);

	// get ideal slip ratio and slip angle
	btScalar sigma_hat(0);
	btScalar alpha_hat(0);
	getSigmaHatAlphaHat(normal_force, sigma_hat, alpha_hat);

	// sigma: longitudinal slip is negative when braking, positive for acceleration
	// alpha: sideslip angle is positive in a right turn(opposite to SAE tire coords)
	// gamma: positive when tire top tilts to the right, viewed from rear in deg
	btScalar rcp_lon_velocity = 1 / Max(btFabs(lon_velocity), btScalar(1E-3));
	btScalar sigma = (rot_velocity - lon_velocity) * rcp_lon_velocity;
	btScalar alpha = -Atan(lat_velocity * rcp_lon_velocity) * rad2deg;
	btScalar gamma = camber * rad2deg;
	btScalar max_Fx(0), max_Fy(0), max_Mz(0);

	btScalar Fx0 = PacejkaFx(sigma, Fz, friction_coeff, max_Fx);
	btScalar Fy0 = PacejkaFy(alpha, Fz, gamma, friction_coeff, max_Fy);
	btScalar Mz = PacejkaMz(alpha, Fz, gamma, friction_coeff, max_Mz);
	btScalar Gx = PacejkaGx(sigma, alpha * deg2rad);
	btScalar Gy = PacejkaGy(sigma, alpha * deg2rad);
	btScalar Fx = Gx * Fx0;
	btScalar Fy = Gy * Fy0;

	slip = sigma;
	slip_angle = alpha * deg2rad;
	ideal_slip = sigma_hat;
	ideal_slip_angle = alpha_hat * deg2rad;
	fx = Fx;
	fy = Fy;
	mz = Mz;

	// Fx positive during traction, Fy positive in a right turn, Mz positive in a left turn
	return btVector3(Fx, Fy, Mz);
}

btScalar CarTire1::getRollingResistance(const btScalar velocity, const btScalar resistance_factor) const
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
	btScalar BCD = (b[3] * Fz + b[4]) * Fz * btExp(-b[5] * Fz);
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
	btScalar BCD = a[3] * Sin2Atan(Fz, a[4]) * (1 - a[5] * btFabs(gamma));
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

btScalar CarTire1::PacejkaFx(btScalar sigma, btScalar Fz, btScalar friction_coeff, btScalar & max_Fx) const
{
	auto & b = longitudinal;

	// shape factor
	btScalar C = b[0];

	// peak factor
	btScalar D = (b[1] * Fz + b[2]) * Fz;

	btScalar BCD = (b[3] * Fz + b[4]) * Fz * btExp(-b[5] * Fz);

	// stiffness factor
	btScalar B =  BCD / (C * D);

	// curvature factor
	btScalar E = b[6] * Fz * Fz + b[7] * Fz + b[8];

	// horizontal shift
	btScalar Sh = b[9] * Fz + b[10];

	// composite
	btScalar S = 100 * sigma + Sh;

	// longitudinal force
	btScalar Fx = D * Sin3Pi2(C * Atan(B * S - E * (B * S - Atan(B * S))));

	// scale by surface friction
	Fx = Fx * friction_coeff;
	max_Fx = D * friction_coeff;

	btAssert(Fx == Fx);
	return Fx;
}

btScalar CarTire1::PacejkaFy(btScalar alpha, btScalar Fz, btScalar gamma, btScalar friction_coeff, btScalar & max_Fy) const
{
	auto & a = lateral;

	// shape factor
	btScalar C = a[0];

	// peak factor
	btScalar D = (a[1] * Fz + a[2]) * Fz;

	btScalar BCD = a[3] * Sin2Atan(Fz, a[4]) * (1 - a[5] * btFabs(gamma));

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
	max_Fy = (D + Sv) * friction_coeff;

	btAssert(Fy == Fy);
	return Fy;
}

btScalar CarTire1::PacejkaGx(btScalar sigma, btScalar alpha) const
{
	auto & p = combining;
	btScalar B = p[2] * CosAtan(p[3] * sigma);
	btScalar G = CosAtan(B * alpha);
	return G;
}

btScalar CarTire1::PacejkaGy(btScalar sigma, btScalar alpha) const
{
	auto & p = combining;
	btScalar B = p[0] * CosAtan(p[1] * alpha);
	btScalar G = CosAtan(B * sigma);
	return G;
}

btScalar CarTire1::PacejkaMz(btScalar alpha, btScalar Fz, btScalar gamma, btScalar friction_coeff, btScalar & max_Mz) const
{
	auto & c = aligning;

	btScalar C = c[0];

	// peak factor
	btScalar D = (c[1] * Fz + c[2]) * Fz;

	btScalar BCD = (c[3] * Fz + c[4]) * Fz * (1 - c[6] * btFabs(gamma)) * btExp(-c[5] * Fz);

	// stiffness factor
	btScalar B =  BCD / (C * D);

	// curvature factor
	btScalar E = (c[7] * Fz * Fz + c[8] * Fz + c[9]) * (1 - c[10] * btFabs(gamma));

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
	max_Mz = (D + Sv) * friction_coeff;

	btAssert(Mz == Mz);
	return Mz;
}

void CarTire1::getSigmaHatAlphaHat(btScalar load, btScalar & sh, btScalar & ah) const
{
	const btScalar rdelta = 1 / 500.0; // 500 N table delta
	const int table_size = size(sigma_hat);
	assert(table_size > 1);

	btScalar n = Clamp(load * rdelta - 1, btScalar(0), btScalar(table_size - 1 - 1E-3));
	int i = n;
	btScalar blend = n - i;

	sh = sigma_hat[i] * (1-blend) + sigma_hat[i+1] * blend;
	ah = alpha_hat[i] * (1-blend) + alpha_hat[i+1] * blend;
}

void CarTire1::findSigmaHatAlphaHat(
	btScalar load,
	btScalar & output_sigmahat,
	btScalar & output_alphahat,
	int iterations) const
{
	btScalar junk;
	btScalar fmax = 0;
	btScalar xmax = 1;
	btScalar dx = xmax / iterations;
	for (btScalar x = 0; x < xmax; x += dx)
	{
		btScalar f = PacejkaFx(x, load, 1, junk);
		if (f > fmax)
		{
			output_sigmahat = x;
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
	for (btScalar x = 0; x < xmax; x += dx)
	{
		btScalar f = PacejkaFy(x, load, 0, 1, junk);
		if (f > fmax)
		{
			output_alphahat = x;
			fmax = f;
		}
		else if (f < fmax && fmax > 0)
		{
			break;
		}
	}
	btAssert(fmax > 0);
}

void CarTire1::initSigmaHatAlphaHat()
{
	for (int i = 0; i < size(sigma_hat); i++)
	{
		btScalar hat_load = (i + 1) * btScalar(0.5);
		findSigmaHatAlphaHat(hat_load, sigma_hat[i], alpha_hat[i]);
	}
}
