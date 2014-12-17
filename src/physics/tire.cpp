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

template <typename T>
inline T sgn(T val)
{
	return (T(0) < val) - (val < T(0));
}

#define ENTRY(x) #x,
const char * TireInfo::coeffname[] = { TIRE_COEFF_LIST };
#undef ENTRY

TireInfo::TireInfo() :
	nominal_load(5000),
	max_load(10000),
	max_camber(15),
	roll_resistance_quad(1E-6),
	roll_resistance_lin(1E-3),
	tread(0)
{
	// ctor
}

Tire::Tire() :
	slip(0),
	slip_angle(0),
	ideal_slip(0),
	ideal_slip_angle(0),
	vx(0), vy(0),
	fx(0), fy(0), fz(0),
	mz(0)
{
	// ctor
}

void Tire::init(const TireInfo & info)
{
	TireInfo::operator=(info);
	initSigmaHatAlphaHat();
}

void Tire::getSigmaHatAlphaHat(btScalar load, btScalar & sh, btScalar & ah) const
{
	btScalar rload = load / max_load * tablesize - 1.0;
	btClamp(rload, btScalar(0.0), btScalar(tablesize - 1.0));
	int lbound = btMin(int(rload), tablesize - 2);

	btScalar blend = rload - lbound;
	sh = sigma_hat[lbound] * (1.0 - blend) + sigma_hat[lbound + 1] * blend;
	ah = alpha_hat[lbound] * (1.0 - blend) + alpha_hat[lbound + 1] * blend;
}

btVector3 Tire::getForce(
	btScalar normal_load,
	btScalar friction_coeff,
	btScalar camber,
	btScalar rot_velocity,
	btScalar lon_velocity,
	btScalar lat_velocity)
{
	if (normal_load * friction_coeff  < 1E-6)
	{
		slip = slip_angle = 0;
		ideal_slip = ideal_slip_angle = 1;
		fx = fy = fz = mz = 0;
		vx = vy = 0;
		return btVector3(0, 0, 0);
	}

	// limit input
	btClamp(normal_load, btScalar(0), btScalar(max_load));
	btClamp(camber, -max_camber, max_camber);

	// sigma and alpha
	btScalar denom = btMax(btFabs(lon_velocity), btScalar(1E-3));
	btScalar lon_slip_velocity = lon_velocity - rot_velocity;
	btScalar sigma = -lon_slip_velocity / denom;
	btScalar alpha = btAtan(lat_velocity / denom);

	// force parameters
	btScalar Fz = normal_load;
	btScalar Fz0 = nominal_load;
	btScalar dFz = (Fz - Fz0) / Fz0;

	// pure slip
	btScalar Dy, BCy, Shf;
	btScalar Fx0 = PacejkaFx(sigma, Fz, dFz, friction_coeff);
	btScalar Fy0 = PacejkaFy(alpha, camber, Fz, dFz, friction_coeff, Dy, BCy, Shf);
	btScalar Mz0 = PacejkaMz(alpha, camber, Fz, dFz, friction_coeff, Fy0, BCy, Shf);

	// combined slip
	btScalar Gx = PacejkaGx(sigma, alpha);
	btScalar Gy = PacejkaGy(sigma, alpha);
	btScalar Svy = PacejkaSvy(sigma, alpha, camber, dFz, Dy);
	btScalar Fx = Gx * Fx0;
	btScalar Fy = Gy * Fy0 + Svy;

	// ideal slip and angle
	btScalar sigma_hat(0), alpha_hat(0);
	getSigmaHatAlphaHat(normal_load, sigma_hat, alpha_hat);

	slip = sigma;
	slip_angle = alpha;
	ideal_slip = sigma_hat;
	ideal_slip_angle = alpha_hat;
	fx = Fx;
	fy = Fy;
	fz = Fz;
	mz = Mz0;
	vx = lon_slip_velocity;
	vy = lat_velocity;

	return btVector3(Fx, Fy, Mz0);
}

btScalar Tire::getSqueal() const
{
	btScalar squeal = 0.0;
	if (vx * vx > 1E-2 && slip * slip > 1E-6)
	{
		btScalar vx_body = vx / slip;
		btScalar vx_ideal = ideal_slip * vx_body;
		btScalar vy_ideal = ideal_slip_angle * vx_body; //btTan(ideal_slip_angle) * vx_body;
		btScalar vx_squeal = btFabs(vx / vx_ideal);
		btScalar vy_squeal = btFabs(vy / vy_ideal);
		// start squeal at 80% of the ideal slide/slip, max out at 160%
		squeal = 1.25 * btMax(vx_squeal, vy_squeal) - 1.0;
		btClamp(squeal, btScalar(0), btScalar(1));
	}
	return squeal;
}

btScalar Tire::getMaxFx(btScalar load) const
{
	const btScalar * p = coefficients;
	btScalar Fz = load;
	btScalar Fz0 = nominal_load;
	btScalar dFz = (Fz - Fz0) / Fz0;
	btScalar D = Fz * (p[PDX1] + p[PDX2] * dFz);
	btScalar Sv = Fz * (p[PVX1] + p[PVX2] * dFz);
	return D + Sv;
}

btScalar Tire::getMaxFy(btScalar load, btScalar camber) const
{
	const btScalar * p = coefficients;
	btScalar Fz = load;
	btScalar Fz0 = nominal_load;
	btScalar dFz = (Fz - Fz0) / Fz0;
	btScalar gamma = camber;
	btScalar D = Fz * (p[PDY1] + p[PDY2] * dFz) * (1 - p[PDY3] * gamma * gamma);
	btScalar Sv = Fz * (p[PVY1] + p[PVY2] * dFz + (p[PVY3] + p[PVY4] * dFz) * gamma);
	return D + Sv;
}

btScalar Tire::PacejkaFx(
	btScalar sigma,
	btScalar Fz,
	btScalar dFz,
	btScalar friction_coeff) const
{
	const btScalar * p = coefficients;

	// vertical shift
	btScalar Sv = Fz * (p[PVX1] + p[PVX2] * dFz);

	// horizontal shift
	btScalar Sh = p[PHX1] + p[PHX2] * dFz;

	// composite slip
	btScalar S = sigma + Sh;

	// slope at origin
	btScalar K = Fz * (p[PKX1] + p[PKX2] * dFz) * btExp(-p[PKX3] * dFz);

	// curvature factor
	btScalar E = (p[PEX1] + p[PEX2] * dFz + p[PEX3] * dFz * dFz) * (1 - p[PEX4] * sgn(S));

	// peak factor
	btScalar D = Fz * (p[PDX1] + p[PDX2] * dFz);

	// shape factor
	btScalar C = p[PCX1];

	// stiffness factor
	btScalar B =  K / (C * D);

	// force
	btScalar F = D * btSin(C * btAtan(B * S - E * (B * S - btAtan(B * S)))) + Sv;

	// scale by surface friction
	F *= friction_coeff;

	return F;
}

btScalar Tire::PacejkaFy(
	btScalar alpha,
	btScalar gamma,
	btScalar Fz,
	btScalar dFz,
	btScalar friction_coeff,
	btScalar & Dy,
	btScalar & BCy,
	btScalar & Shf) const
{
	const btScalar * p = coefficients;
	btScalar Fz0 = nominal_load;

	// vertical shift
	btScalar Sv = Fz * (p[PVY1] + p[PVY2] * dFz + (p[PVY3] + p[PVY4] * dFz) * gamma);

	// horizontal shift
	btScalar Sh = p[PHY1] + p[PHY2] * dFz + p[PHY3] * gamma;

	// composite slip angle
	btScalar A = alpha + Sh;

	// slope at origin
	btScalar K = p[PKY1] * Fz0 * btSin(2 * btAtan(Fz / (p[PKY2] * Fz0))) * (1 - p[PKY3] * btFabs(gamma));

	// curvature factor
	btScalar E = (p[PEY1] + p[PEY2] * dFz) * (1 - (p[PEY3] + p[PEY4] * gamma) * sgn(A));

	// peak factor
	btScalar D = Fz * (p[PDY1] + p[PDY2] * dFz) * (1 - p[PDY3] * gamma * gamma);

	// shape factor
	btScalar C = p[PCY1];

	// stiffness factor
	btScalar B = K / (C * D);

	// force
	btScalar F = D * btSin(C * btAtan(B * A - E * (B * A - btAtan(B * A)))) + Sv;

	// scale by surface friction
	F *= friction_coeff;

	// aligning torque params
	Dy = D;
	BCy = B * C;
	Shf = Sh + Sv / K;

	return F;
}

btScalar Tire::PacejkaMz(
	btScalar alpha,
	btScalar gamma,
	btScalar Fz,
	btScalar dFz,
	btScalar friction_coeff,
	btScalar Fy,
	btScalar BCy,
	btScalar Shf) const
{
	const btScalar * p = coefficients;
	btScalar Fz0 = nominal_load;
	btScalar R0 = 0.3;
	btScalar yz = gamma;
	btScalar cos_alpha = btCos(alpha);

	btScalar Sht = p[QHZ1] + p[QHZ2] * dFz + (p[QHZ3] + p[QHZ4] * dFz) * yz;

	btScalar At = alpha + Sht;

	btScalar Bt = (p[QBZ1] + p[QBZ2] * dFz + p[QBZ3] * dFz * dFz) * (1 + p[QBZ4] * yz + p[QBZ5] * btFabs(yz));

	btScalar Ct = p[QCZ1];

	btScalar Dt = Fz * (p[QDZ1] + p[QDZ2] * dFz) * (1 + p[QDZ3] * yz + p[QDZ4] * yz * yz) * (R0 / Fz0);

	btScalar Et = (p[QEZ1] + p[QEZ2] * dFz + p[QEZ3] * dFz * dFz) * (1 + (p[QEZ4] + p[QEZ5] * yz) * btAtan(Bt * Ct * At));

	btScalar Mzt = -Fy * Dt * btCos(Ct * btAtan(Bt * At - Et * (Bt * At - btAtan(Bt * At)))) * cos_alpha;

	btScalar Ar = alpha + Shf;

	btScalar Br = p[QBZ10] * BCy;

	btScalar Dr = Fz * (p[QDZ6] + p[QDZ7] * dFz + (p[QDZ8] + p[QDZ9] * dFz) * yz) * R0;

	btScalar Mzr = Dr * btCos(btAtan(Br * Ar)) * cos_alpha * friction_coeff;

	return Mzt + Mzr;
}

btScalar Tire::PacejkaGx(
	btScalar sigma,
	btScalar alpha)
{
	const btScalar * p = coefficients;
	btScalar B = p[RBX1] * btCos(btAtan(p[RBX2] * sigma));
	btScalar C = p[RCX1];
	btScalar Sh = p[RHX1];
	btScalar S = alpha + Sh;
	btScalar G0 = btCos(C * btAtan(B * Sh));
	btScalar G = btCos(C * btAtan(B * S)) / G0;
	return G;
}

btScalar Tire::PacejkaGy(
	btScalar sigma,
	btScalar alpha)
{
	const btScalar * p = coefficients;
	btScalar B = p[RBY1] * btCos(btAtan(p[RBY2] * (alpha - p[RBY3])));
	btScalar C = p[RCY1];
	btScalar Sh = p[RHY1];
	btScalar S = sigma + Sh;
	btScalar G0 = btCos(C * btAtan(B * Sh));
	btScalar G = btCos(C * btAtan(B * S)) / G0;
	return G;
}

btScalar Tire::PacejkaSvy(
	btScalar sigma,
	btScalar alpha,
	btScalar gamma,
	btScalar dFz,
	btScalar Dy)
{
	const btScalar * p = coefficients;
	btScalar Dv = Dy * (p[RVY1] + p[RVY2] * dFz + p[RVY3] * gamma) * btCos(btAtan(p[RVY4] * alpha));
	btScalar Sv = Dv * btSin(p[RVY5] * btAtan(p[RVY6] * sigma));
	return Sv;
}

void Tire::findSigmaHatAlphaHat(
	btScalar load,
	btScalar & output_sigmahat,
	btScalar & output_alphahat,
	int iterations)
{
	btScalar Fz = load;
	btScalar Fz0 = nominal_load;
	btScalar dFz = (Fz - Fz0) / Fz0;
	btScalar camber = 0.0;
	btScalar mu = 1.0;
	btScalar Dy, BCy, Shf; // unused

	btScalar Fxmax = 0.0;
	btScalar smax = 2.0;
	for (btScalar s = -smax; s < smax; s += 2 * smax / iterations)
	{
		btScalar Fx = PacejkaFx(s, Fz, dFz, mu);
		if (Fx > Fxmax)
		{
			output_sigmahat = btFabs(s);
			Fxmax = Fx;
		}
	}

	btScalar Fymax = 0.0;
	btScalar amax = 30.0 * SIMD_RADS_PER_DEG;
	for (btScalar a = -amax; a < amax; a += 2 * amax / iterations)
	{
		btScalar Fy = PacejkaFy(a, camber, Fz, dFz, mu, Dy, BCy, Shf);
		if (Fy > Fymax)
		{
			output_alphahat = btFabs(a);
			Fymax = Fy;
		}
	}
}

void Tire::initSigmaHatAlphaHat()
{
	btScalar load_delta = max_load / tablesize;
	for (int i = 0; i < tablesize; ++i)
	{
		findSigmaHatAlphaHat((btScalar)(i + 1) * load_delta, sigma_hat[i], alpha_hat[i]);
	}
}

