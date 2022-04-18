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

#include "cartire2.h"
#include "cartirebase.h"
#include "minmax.h"

template <typename T>
inline T sgn(T v)
{
	return std::copysign(T(1), v);
}

#define ENTRY(x) #x,
const char * CarTire2::coeffname[] = { TIRE_COEFF_LIST };
#undef ENTRY

CarTire2::CarTire2() :
	nominal_load(5000),
	max_load(10000),
	max_camber(15),
	roll_resistance_quad(1E-6),
	roll_resistance_lin(1E-3),
	tread(0)
{
	// ctor
}

void CarTire2::ComputeState(
	btScalar normal_load,
	btScalar rot_velocity,
	btScalar lon_velocity,
	btScalar lat_velocity,
	CarTireState & s) const
{
	if (normal_load * s.friction < btScalar(1E-6))
	{
		s.slip = s.slip_angle = 0;
		s.fx = s.fy = s.mz = 0;
		return;
	}

	btScalar sigma, alpha;
	ComputeSlip(lon_velocity, lat_velocity, rot_velocity, sigma, alpha);
	alpha = -alpha; // fixup

	// force parameters
	btScalar Fz = Clamp(normal_load, btScalar(0), btScalar(max_load));
	btScalar Fz0 = nominal_load;
	btScalar dFz = (Fz - Fz0) / Fz0;

	// pure slip
	btScalar Dy, BCy, Shf;
	btScalar Fx0 = PacejkaFx(sigma, Fz, dFz, s.friction);
	btScalar Fy0 = PacejkaFy(alpha, s.camber, Fz, dFz, s.friction, Dy, BCy, Shf);
	btScalar Mz0 = PacejkaMz(alpha, s.camber, Fz, dFz, s.friction, Fy0, BCy, Shf);

	// combined slip
	btScalar Gx = PacejkaGx(sigma, alpha);
	btScalar Gy = PacejkaGy(sigma, alpha);
	btScalar Svy = PacejkaSvy(sigma, alpha, s.camber, dFz, Dy);
	btScalar Fx = Gx * Fx0;
	btScalar Fy = Gy * Fy0 + Svy;

	s.vcam = 0; // FIXME
	s.slip = sigma;
	s.slip_angle = alpha;
	s.fx = Fx;
	s.fy = Fy;
	s.mz = Mz0;
}

btScalar CarTire2::getRollingResistance(btScalar velocity, btScalar resistance_factor) const
{
	// surface influence on rolling resistance
	btScalar resistance = resistance_factor * roll_resistance_lin;

	// heat due to tire deformation increases rolling resistance
	// approximate by quadratic function
	resistance += velocity * velocity * roll_resistance_quad;

	return resistance;
}

btScalar CarTire2::getMaxFx(btScalar load) const
{
	const btScalar * p = coefficients;
	btScalar Fz = load;
	btScalar Fz0 = nominal_load;
	btScalar dFz = (Fz - Fz0) / Fz0;
	btScalar D = Fz * (p[PDX1] + p[PDX2] * dFz);
	btScalar Sv = Fz * (p[PVX1] + p[PVX2] * dFz);
	return D + Sv;
}

btScalar CarTire2::getMaxFy(btScalar load, btScalar camber) const
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

btScalar CarTire2::getMaxMz(btScalar load, btScalar camber) const
{
	const btScalar * p = coefficients;
	btScalar Fz = load;
	btScalar Fz0 = nominal_load;
	btScalar dFz = (Fz - Fz0) / Fz0;
	btScalar yz = camber;
	btScalar R0 = 0.3;
	btScalar Dr = Fz * (p[QDZ6] + p[QDZ7] * dFz + (p[QDZ8] + p[QDZ9] * dFz) * yz) * R0;
	return Dr;
}

btScalar CarTire2::PacejkaFx(
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

btScalar CarTire2::PacejkaFy(
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

btScalar CarTire2::PacejkaMz(
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

btScalar CarTire2::PacejkaGx(
	btScalar sigma,
	btScalar alpha) const
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

btScalar CarTire2::PacejkaGy(
	btScalar sigma,
	btScalar alpha) const
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

btScalar CarTire2::PacejkaSvy(
	btScalar sigma,
	btScalar alpha,
	btScalar gamma,
	btScalar dFz,
	btScalar Dy) const
{
	const btScalar * p = coefficients;
	btScalar Dv = Dy * (p[RVY1] + p[RVY2] * dFz + p[RVY3] * gamma) * btCos(btAtan(p[RVY4] * alpha));
	btScalar Sv = Dv * btSin(p[RVY5] * btAtan(p[RVY6] * sigma));
	return Sv;
}

void CarTire2::findIdealSlip(
	btScalar load,
	btScalar output_slip[2],
	int iterations) const
{
	btScalar Fz = load;
	btScalar Fz0 = nominal_load;
	btScalar dFz = (Fz - Fz0) / Fz0;
	btScalar camber = 0;
	btScalar mu = 1;
	btScalar Dy, BCy, Shf; // unused

	btScalar Fxmax = 0;
	btScalar smax = 1;
	btScalar ds = smax / iterations;
	btScalar sigmahat = 0;
	for (btScalar s = 0; s < smax; s += ds)
	{
		btScalar Fx = PacejkaFx(s, Fz, dFz, mu);
		if (Fx > Fxmax)
		{
			sigmahat = s;
			Fxmax = Fx;
		}
		else if (Fx < Fxmax && Fxmax > 0)
		{
			break;
		}
	}
	btAssert(Fxmax > 0);

	btScalar Fymax = 0;
	btScalar amax = 40 * (M_PI / 180.0);
	btScalar da = amax / iterations;
	btScalar alphahat = 0;
	for (btScalar a = 0; a < amax; a += da)
	{
		btScalar Fy = PacejkaFy(a, camber, Fz, dFz, mu, Dy, BCy, Shf);
		if (Fy > Fymax)
		{
			alphahat = a;
			Fymax = Fy;
		}
		else if (Fy < Fymax && Fymax > 0)
		{
			break;
		}
	}
	btAssert(Fymax > 0);

	output_slip[0] = sigmahat;
	output_slip[1] = alphahat;
}

void CarTire2::initSlipLUT(CarTireSlipLUT & t) const
{
	for (int i = 0; i < t.size(); ++i)
	{
		btScalar load = (i + 1) * t.delta();
		findIdealSlip(load, t.ideal_slip_lut[i]);
	}
}
