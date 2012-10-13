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

#include "tire.h"

namespace sim
{

TireInfo::TireInfo() :
	tread(0),
	max_load(10),
	max_camber(15),
	longitudinal(11),
	lateral(15),
	aligning(18)
{
	// ctor
}

Tire::Tire() :
	camber(0),
	slipratio(0),
	slipangle(0),
	slipratio_ideal(0),
	slipangle_ideal(0),
	fx(0),
	fy(0),
	fz(0),
	mz(0),
	vx(0),
	vy(0)
{
	// ctor
}

void Tire::init(const TireInfo & info)
{
	TireInfo::operator=(info);
	calculateSigmaHatAlphaHat();
}

void Tire::getSigmaHatAlphaHat(btScalar load, btScalar & sh, btScalar & ah) const
{
	btAssert(!sigma_hat.empty());
	btAssert(!alpha_hat.empty());

	int tablesize = sigma_hat.size();
	btScalar rload = load / max_load * tablesize - 1.0;
	btClamp(rload, btScalar(0.0), btScalar(tablesize - 1.0));
	int lbound = std::min(int(rload), tablesize - 2);

	btScalar blend = rload - lbound;
	sh = sigma_hat[lbound] * (1.0 - blend) + sigma_hat[lbound + 1] * blend;
	ah = alpha_hat[lbound] * (1.0 - blend) + alpha_hat[lbound + 1] * blend;
}

void Tire::update(
	btScalar normal_force,
	btScalar friction_coeff,
	btScalar inclination,
	btScalar ang_velocity,
	btScalar lon_velocity,
	btScalar lat_velocity)
{
	if (normal_force * friction_coeff < 1E-3)
	{
		slipratio = slipangle = 0;
		slipratio_ideal = slipangle_ideal = 1;
		fx = fy = fz = mz = 0;
		mux = muy = 0;
		vx = vy = 0;
		return;
	}

	// pacejka in kN
	fz = normal_force * 1E-3;

	// clamp tire load and camber
	btClamp(fz, btScalar(0), btScalar(max_load));
	btClamp(inclination, -max_camber, max_camber);

	// get ideal slip
	getSigmaHatAlphaHat(fz, slipratio_ideal, slipangle_ideal);

	camber = inclination; // positive when tire top tilts to the right, viewed from rear
	vx = lon_velocity - ang_velocity;
	vy = lat_velocity;
	btScalar denom = btMax(btFabs(lon_velocity), btScalar(1E-3));
	slipratio = -vx / denom; // longitudinal slip: negative in braking, positive in traction
	slipangle = -atan(vy / denom) * 180.0 / M_PI; // sideslip angle: positive in a right turn (opposite to SAE tire coords)

	// clamp slip
	btClamp(slipratio, btScalar(-1), btScalar(1));
	btClamp(slipangle, btScalar(-60), btScalar(60));

	// combining method 1: beckman method for pre-combining longitudinal and lateral forces
	// seems to be a variant of Bakker 1987:
	// refined Kamm Circle for non-isotropic tire characteristics
	// and slip normalization to guarantee simultaneous sliding
	btScalar s = slipratio / slipratio_ideal;
	btScalar a = slipangle / slipangle_ideal;
	btScalar rho = btMax(btSqrt(s * s + a * a), btScalar(1E-4)); // normalized slip
	btScalar max_Fx(0), max_Fy(0), max_Mz(0);
	fx = (s / rho) * PacejkaFx(rho * slipratio_ideal, fz, friction_coeff, max_Fx);
	fy = (a / rho) * PacejkaFy(rho * slipangle_ideal, fz, camber, friction_coeff, max_Fy);
	mz = PacejkaMz(slipratio, slipangle, fz, camber, friction_coeff, max_Mz);

	// friction coefficients
	mux = fx / normal_force;
	muy = fy / normal_force;
}

btScalar Tire::getSqueal() const
{
	if (btFabs(slipratio) < 1E-3)
		return 0.0;

	btScalar vsq = vx * vx + vy * vy;
	btScalar vx_ideal = vx * slipratio_ideal / slipratio;
	btScalar squeal = vsq - vx_ideal * vx_ideal;
	btClamp(squeal, btScalar(0), btScalar(1));
	return squeal;
}

btScalar Tire::getMaxFx(btScalar load) const
{
	const std::vector<btScalar> & b = longitudinal;
	btScalar Fz = load * 0.001;
	return (b[1] * Fz + b[2]) * Fz;
}

btScalar Tire::getMaxFy(btScalar load, btScalar camber) const
{
	const std::vector<btScalar> & a = lateral;
	btScalar Fz = load * 0.001;
	btScalar gamma = camber;

	btScalar D = (a[1] * Fz + a[2]) * Fz;
	btScalar Sv = ((a[11] * Fz + a[12]) * gamma + a[13] ) * Fz + a[14];

	return D + Sv;
}

btScalar Tire::getMaxMz(btScalar load, btScalar camber) const
{
	const std::vector<btScalar> & c = aligning;
	btScalar Fz = load * 0.001;
	btScalar gamma = camber;

	btScalar D = (c[1] * Fz + c[2]) * Fz;
	btScalar Sv = (c[14] * Fz * Fz + c[15] * Fz) * gamma + c[16] * Fz + c[17];

	return -(D + Sv);
}

btScalar Tire::PacejkaFx(btScalar sigma, btScalar Fz, btScalar friction_coeff, btScalar & max_Fx) const
{
	const std::vector<btScalar> & b = longitudinal;

	// shape factor
	btScalar C = b[0];

	// peak factor
	btScalar D = (b[1] * Fz + b[2]) * Fz;

	// slope at origin
	btScalar BCD = (b[3] * Fz + b[4]) * Fz * exp(-b[5] * Fz);

	// stiffness factor
	btScalar B =  BCD / (C * D);

	// curvature factor
	btScalar E = b[6] * Fz * Fz + b[7] * Fz + b[8];

	// curvature factor 1993
	//E = E * (1 - b[13] * sgn(S));

	// horizontal shift
	btScalar Sh = b[9] * Fz + b[10];

	// vertical shift 1993
	//btScalar Sv = b[11] * Fz + b[12];

	// composite
	btScalar S = 100 * sigma + Sh; // factor 100, percent?

	// longitudinal force
	btScalar Fx = D * sin(C * atan(B * S - E * (B * S - atan(B * S))));

	// scale by surface friction
	Fx = Fx * friction_coeff;
	max_Fx = D * friction_coeff;

	return Fx;
}

btScalar Tire::PacejkaFy(btScalar alpha, btScalar Fz, btScalar gamma, btScalar friction_coeff, btScalar & max_Fy) const
{
	const std::vector<btScalar> & a = lateral;

	// shape factor
	btScalar C = a[0];

	// peak factor
	btScalar D = (a[1] * Fz + a[2]) * Fz;

	// peak factor 1993
	// D = D * (1 - a[15] * gamma * gamma);

	// slope at origin
	btScalar BCD = a[3] * sin(2.0 * atan(Fz / a[4])) * (1.0 - a[5] * btFabs(gamma));

	// stiffness factor
	btScalar B = BCD / (C * D);

	// curvature factor
	btScalar E = a[6] * Fz + a[7];

	// curvature factor 1993
	// E = E * (1 - (a[16] * gamma + a[17]) * sgn(alpha + Sh));

	// horizontal shift
	btScalar Sh = a[8] * gamma + a[9] * Fz + a[10];

	// horizontal shift 1993
	// Sh = a[8] * Fz + a[9] + a[10] * gamma;

	// vertical shift
	btScalar Sv = ((a[11] * Fz + a[12]) * gamma + a[13]) * Fz + a[14];

	// vertical shift 1993
	// Sv = a[11] * Fz + a[12] + (a[13] * Fz * Fz + a[14] * Fz) * gamma;

	// composite
	btScalar S = alpha + Sh;

	// lateral force
	btScalar Fy = D * sin(C * atan(B * S - E * (B * S - atan(B * S)))) + Sv;

	// scale by surface friction
	Fy = Fy * friction_coeff;
	max_Fy = (D + Sv) * friction_coeff;

	return Fy;
}

btScalar Tire::PacejkaMz(btScalar sigma, btScalar alpha, btScalar Fz, btScalar gamma, btScalar friction_coeff, btScalar & max_Mz) const
{
	const std::vector<btScalar> & c = aligning;

	btScalar C = c[0];

	// peak factor
	btScalar D = (c[1] * Fz + c[2]) * Fz;

	// peak factor 1993
	// D = D * (1 - c[18] * gamma * gamma);

	// slope at origin
	btScalar BCD = (c[3] * Fz + c[4]) * Fz * (1.0 - c[6] * btFabs(gamma)) * exp (-c[5] * Fz);

	// stiffness factor
	btScalar B =  BCD / (C * D);

	// curvature factor
	btScalar E = (c[7] * Fz * Fz + c[8] * Fz + c[9]) * (1.0 - c[10] * btFabs(gamma));

	// curvature factor 1993
	// E = (c[7] * Fz * Fz + c[8] * Fz + c[9]) * (1.0 - (c[19] * gamma + c[20]) * sgn(S)) / (1.0 - c[10] * btFabs(gamma));

	// horizontal shift
	btScalar Sh = c[11] * gamma + c[12] * Fz + c[13];

	// horizontal shift 1993
	// Sh = c[11] * Fz + c[12] + c[13] * gamma;

	// vertical shift
	btScalar Sv = (c[14] * Fz * Fz + c[15] * Fz) * gamma + c[16] * Fz + c[17];

	// vertical shift 1993
	// Sv = c[14] * Fz + c[15] + (c[16] * Fz * Fz + c[17] * Fz) * gamma;

	// composite
	btScalar S = alpha + Sh;

	// self-aligning torque
	btScalar Mz = D * sin(c[0] * atan(B * S - E * (B * S - atan(B * S)))) + Sv;

	// scale by surface friction
	Mz = Mz * friction_coeff;
	max_Mz = (D + Sv) * friction_coeff;

	btAssert(Mz == Mz);
	return Mz;
}

void Tire::findSigmaHatAlphaHat(btScalar load, btScalar & output_sigmahat, btScalar & output_alphahat, int iterations)
{
	btScalar x, y, ymax, junk;
	ymax = 0;
	for (x = -1; x < 1; x += 2.0 / iterations)
	{
		y = PacejkaFx(x, load, 1.0, junk);
		if (y > ymax)
		{
			output_sigmahat = x;
			ymax = y;
		}
	}

	ymax = 0;
	for (x = -20; x < 20; x += 40.0 / iterations)
	{
		y = PacejkaFy(x, load, 0.0, 1.0, junk);
		if (y > ymax)
		{
			output_alphahat = x;
			ymax = y;
		}
	}
}

void Tire::calculateSigmaHatAlphaHat(int tablesize)
{
	btScalar load_delta = max_load / tablesize;
	sigma_hat.resize(tablesize, 0);
	alpha_hat.resize(tablesize, 0);
	for (int i = 0; i < tablesize; ++i)
	{
		findSigmaHatAlphaHat((btScalar)(i + 1) * load_delta, sigma_hat[i], alpha_hat[i]);
	}
}

}
