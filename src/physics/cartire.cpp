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

#include "cartire.h"
#include "fastmath.h"
#include <cassert>

#ifndef VDRIFTN

static const btScalar deg2rad = M_PI / 180;
static const btScalar rad2deg = 180 / M_PI;

CarTireInfo::CarTireInfo() :
	longitudinal(11),
	lateral(15),
	aligning(18),
	combining(4),
	rolling_resistance_quad(1E-6),
	rolling_resistance_lin(1E-3),
	tread(0)
{
	// ctor
}

CarTire::CarTire() :
	camber(0),
	slide(0),
	slip(0),
	ideal_slide(0),
	ideal_slip(0),
	fx(0),
	fy(0),
	fz(0),
	mz(0)
{
	// ctor
}

void CarTire::init(const CarTireInfo & info)
{
	CarTireInfo::operator=(info);
	initSigmaHatAlphaHat();
}

btVector3 CarTire::getForce(
	btScalar normal_force,
	btScalar friction_coeff,
	btScalar inclination,
	btScalar rot_velocity,
	btScalar lon_velocity,
	btScalar lat_velocity)
{
	if (normal_force * friction_coeff < btScalar(1E-6))
	{
		return btVector3(0, 0, 0);
	}

	btScalar Fz = normal_force * btScalar(1E-3);

	// limit input
	btSetMin(Fz, btScalar(30));
	btClamp(inclination, btScalar(-0.1 * M_PI), btScalar(0.1 * M_PI));

	// get ideal slip ratio
	btScalar sigma_hat(0);
	btScalar alpha_hat(0);
	getSigmaHatAlphaHat(normal_force, sigma_hat, alpha_hat);

	// gamma: positive when tire top tilts to the right, viewed from rear in deg
	// sigma: longitudinal slip is negative when braking, positive for acceleration
	// alpha: sideslip angle is positive in a right turn(opposite to SAE tire coords)
	btScalar gamma = inclination * rad2deg;
	btScalar denom = btMax(btFabs(lon_velocity), btScalar(1E-3));
	btScalar sigma = (rot_velocity - lon_velocity) / denom;
	btScalar alpha = -Atan(lat_velocity / denom) * rad2deg;
	btScalar max_Fx(0), max_Fy(0), max_Mz(0);

	btScalar Fx0 = PacejkaFx(sigma, Fz, friction_coeff, max_Fx);
	btScalar Fy0 = PacejkaFy(alpha, Fz, gamma, friction_coeff, max_Fy);
	btScalar Mz = PacejkaMz(alpha, Fz, gamma, friction_coeff, max_Mz);
	btScalar Gx = PacejkaGx(sigma, alpha * deg2rad);
	btScalar Gy = PacejkaGy(sigma, alpha * deg2rad);
	btScalar Fx = Gx * Fx0;
	btScalar Fy = Gy * Fy0;

	camber = inclination;
	slide = sigma;
	slip = alpha * deg2rad;
	ideal_slide = sigma_hat;
	ideal_slip = alpha_hat * deg2rad;
	fx = Fx;
	fy = Fy;
	fz = Fz;
	mz = Mz;

	// Fx positive during traction, Fy positive in a right turn, Mz positive in a left turn
	return btVector3(Fx, Fy, Mz);
}

btScalar CarTire::getRollingResistance(const btScalar velocity, const btScalar resistance_factor) const
{
	// surface influence on rolling resistance
	btScalar rolling_resistance = rolling_resistance_lin * resistance_factor;

	// heat due to tire deformation increases rolling resistance
	// approximate by quadratic function
	rolling_resistance += velocity * velocity * rolling_resistance_quad;

	// rolling resistance direction
	btScalar resistance = (velocity < 0) ? rolling_resistance : -rolling_resistance;

	return resistance;
}

btScalar CarTire::getMaxFx(btScalar load) const
{
	const std::vector<btScalar> & b = longitudinal;
	btScalar Fz = load * btScalar(1E-3);
	btScalar D = (b[1] * Fz + b[2]) * Fz;
	return D;
}

btScalar CarTire::getMaxFy(btScalar load, btScalar camber) const
{
	const std::vector<btScalar> & a = lateral;
	btScalar Fz = load * btScalar(1E-3);
	btScalar gamma = camber;
	btScalar D = (a[1] * Fz + a[2]) * Fz;
	btScalar Sv = ((a[11] * Fz + a[12]) * gamma + a[13] ) * Fz + a[14];
	return D + Sv;
}

btScalar CarTire::getMaxMz(btScalar load, btScalar camber) const
{
	const std::vector<btScalar> & c = aligning;
	btScalar Fz = load * btScalar(1E-3);
	btScalar gamma = camber;
	btScalar D = (c[1] * Fz + c[2]) * Fz;
	btScalar Sv = (c[14] * Fz * Fz + c[15] * Fz) * gamma + c[16] * Fz + c[17];
	return -(D + Sv);
}

btScalar CarTire::PacejkaFx(btScalar sigma, btScalar Fz, btScalar friction_coeff, btScalar & max_Fx) const
{
	const std::vector<btScalar> & b = longitudinal;

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
	btScalar Fx = D * SinPi(C * Atan(B * S - E * (B * S - Atan(B * S))));

	// scale by surface friction
	Fx = Fx * friction_coeff;
	max_Fx = D * friction_coeff;

	btAssert(Fx == Fx);
	return Fx;
}

btScalar CarTire::PacejkaFy(btScalar alpha, btScalar Fz, btScalar gamma, btScalar friction_coeff, btScalar & max_Fy) const
{
	const std::vector<btScalar> & a = lateral;

	// shape factor
	btScalar C = a[0];

	// peak factor
	btScalar D = (a[1] * Fz + a[2]) * Fz;

	btScalar BCD = a[3] * SinPi(2 * Atan(Fz / a[4])) * (1 - a[5] * btFabs(gamma));

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
	btScalar Fy = D * SinPi(C * Atan(B * S - E * (B * S - Atan(B * S)))) + Sv;

	// scale by surface friction
	Fy = Fy * friction_coeff;
	max_Fy = (D + Sv) * friction_coeff;

	btAssert(Fy == Fy);
	return Fy;
}

inline btScalar btCosAtan(btScalar x)
{
	return btRecipSqrt(1 + x * x);
}

btScalar CarTire::PacejkaGx(btScalar sigma, btScalar alpha)
{
	const std::vector<btScalar> & p = combining;
	btScalar B = p[2] * btCosAtan(p[3] * sigma);
	btScalar G = btCosAtan(B * alpha);
	return G;
}

btScalar CarTire::PacejkaGy(btScalar sigma, btScalar alpha)
{
	const std::vector<btScalar> & p = combining;
	btScalar B = p[0] * btCosAtan(p[1] * alpha);
	btScalar G = btCosAtan(B * sigma);
	return G;
}

btScalar CarTire::PacejkaMz(btScalar alpha, btScalar Fz, btScalar gamma, btScalar friction_coeff, btScalar & max_Mz) const
{
	const std::vector<btScalar> & c = aligning;

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

void CarTire::getSigmaHatAlphaHat(btScalar load, btScalar & sh, btScalar & ah) const
{
	assert(!sigma_hat.empty());
	assert(!alpha_hat.empty());

	int HAT_ITERATIONS = sigma_hat.size();

	btScalar HAT_LOAD = 0.5;
	btScalar nf = load * btScalar(1E-3);
	if (nf < HAT_LOAD)
	{
		sh = sigma_hat[0];
		ah = alpha_hat[0];
	}
	else if (nf >= HAT_LOAD * HAT_ITERATIONS)
	{
		sh = sigma_hat[HAT_ITERATIONS-1];
		ah = alpha_hat[HAT_ITERATIONS-1];
	}
	else
	{
		int lbound = (int)(nf/HAT_LOAD);
		lbound--;
		if (lbound < 0)
			lbound = 0;
		if (lbound >= (int)sigma_hat.size())
			lbound = (int)sigma_hat.size()-1;
		btScalar blend = (nf-HAT_LOAD*(lbound+1))/HAT_LOAD;
		sh = sigma_hat[lbound]*(1-blend)+sigma_hat[lbound+1]*blend;
		ah = alpha_hat[lbound]*(1-blend)+alpha_hat[lbound+1]*blend;
	}
}

void CarTire::findSigmaHatAlphaHat(
	btScalar load,
	btScalar & output_sigmahat,
	btScalar & output_alphahat,
	int iterations)
{
	btScalar x, y, ymax, junk;
	ymax = 0;
	for (x = -2; x < 2; x += 4.0/iterations)
	{
		y = PacejkaFx(x, load, 1.0, junk);
		if (y > ymax)
		{
			output_sigmahat = x;
			ymax = y;
		}
	}

	ymax = 0;
	for (x = -20; x < 20; x += 40.0/iterations)
	{
		y = PacejkaFy(x, load, 0, 1.0, junk);
		if (y > ymax)
		{
			output_alphahat = x;
			ymax = y;
		}
	}
}

void CarTire::initSigmaHatAlphaHat(int tablesize)
{
	btScalar HAT_LOAD = 0.5;
	sigma_hat.resize(tablesize, 0);
	alpha_hat.resize(tablesize, 0);
	for (int i = 0; i < tablesize; i++)
	{
		findSigmaHatAlphaHat((btScalar)(i+1)*HAT_LOAD, sigma_hat[i], alpha_hat[i]);
	}
}

#endif
