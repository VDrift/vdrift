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

#include "cartire3.h"
#include "cartirebase.h"
#include "fastmath.h"
#include "minmax.h"
#include <cassert>

CarTire3::CarTire3():
	radius(0.28),
	width(0.2),
	ar(0.45),
	pt(2.3),
	ktx(700),
	kty(700),
	kcb(1.8E6),
	ccb(0.5),
	cfy(0.95),
	dz0(0.01),
	p0(2.0),
	mus(1.5),
	muc(1.0),
	vs(4.0),
	cr0(0.01),
	cr2(6E-6),
	tread(0.2)
{
	// ctor
}

void CarTire3::init()
{
	// vertical stiffness ~200000 N/m
	btScalar cf = 0.28f * btSqrt((1.03f - 0.4f * ar) * width * radius * 2);
	btScalar kz = 9.81f * (1E5f * pt * cf + 3450);

	rkz = 1 / kz;
	rkb = 1 / kcb;
	rp0 = 1 / p0;
	rvs = 1 / vs;
}

btScalar CarTire3::getRollingResistance(btScalar velocity, btScalar resistance_factor) const
{
	// surface influence on rolling resistance
	btScalar resistance = cr0 * resistance_factor;

	// heat due to tire deformation increases rolling resistance
	// approximate by quadratic function
	resistance += cr2 * velocity * velocity;

	return resistance;
}

static inline btScalar Poly4(btScalar c[5], btScalar x)
{
	return (((c[4] * x + c[3]) * x + c[2]) * x + c[1]) * x + c[0];
}

static inline btScalar Poly3(btScalar c[4], btScalar x)
{
	return ((c[3] * x + c[2]) * x + c[1]) * x + c[0];
}

static inline btScalar ComputeSlipPoint(btScalar qp2, btScalar qx2, btScalar qy, btScalar qb)
{
	btScalar c[5] = {
		-qx2 - qy * qy,
		2 * qy * qb,
		9 * qp2 - qb * qb,
		6 * qp2,
		qp2
	};
	btScalar d[4] = {
		c[1],
		2 * c[2],
		3 * c[3],
		4 * c[4]
	};
	btScalar qyb = qy - qb;
	if (qx2 + qyb * qyb > 0)
	{
		btScalar f1 = Poly4(c, 1);
		if (f1 > 0)
		{
			btScalar u = 1 - f1 / Poly3(d, 1);
			u -= Poly4(c, u) / Poly3(d, u);
			u -= Poly4(c, u) / Poly3(d, u);
			u -= Poly4(c, u) / Poly3(d, u);
			return u - 1;
		}
		btScalar f2 = Poly4(c, 2);
		if (f2 * f1 < 0)
		{
			btScalar u = 2 - f2 / Poly3(d, 2);
			u -= Poly4(c, u) / Poly3(d, u);
			u -= Poly4(c, u) / Poly3(d, u);
			u -= Poly4(c, u) / Poly3(d, u);
			return u - 1;
		}
		return 1;
	}
	return -1;
}

void CarTire3::ComputeState(
	btScalar normal_force,
	btScalar rot_velocity,
	btScalar lon_velocity,
	btScalar lat_velocity,
	CarTireState & s) const
{
	btScalar vrx = lon_velocity - rot_velocity;
	btScalar vry = lat_velocity;
	btScalar vr2 = vrx * vrx + vry * vry;
	if (normal_force < 1E-6f || s.friction < 1E-6f || vr2 < 1E-12f)
	{
		s.slip = s.slip_angle = 0;
		s.fx = s.fy = s.mz = 0;
		return;
	}

	btScalar fz = Min(normal_force, btScalar(20E3));
	btScalar sin_camber = Clamp(btScalar(s.camber), btScalar(-0.3f), btScalar(0.3f)); // +-18 deg

	btScalar vr = btSqrt(vr2);
	btScalar rvr = 1 / vr;
	btScalar nx = -vrx * rvr;
	btScalar ny = -vry * rvr;

	btScalar wr = std::copysign(Max(btScalar(std::abs(rot_velocity)), btScalar(1E-12f)), rot_velocity);
	btScalar rwr = 1 / wr;
	btScalar sx = -vrx * rwr;
	btScalar sy = -vry * rwr;

	// vertical deflection
	btScalar dz = fz * rkz;

	// patch width
	btScalar w = width;
	if (dz < dz0)
	{
		btScalar sz = 1 - dz / dz0;
		w *= btSqrt(1 - sz * sz);
	}

	// patch half length
	btScalar a = 0.3f * (dz + 2.25f * btSqrt(radius * dz));
	btScalar wa = w * a * 1E5f;

	// contact pressure
	// p(u) = p * (1 - u^2) * (1 + u / 4) with u = x / a
	// fz = w * a * p * 4 / 3
	btScalar p = 0.75f * fz / wa;

	// friction coeff
	btScalar mu = muc + (mus - muc) * btExp(-btSqrt(vr * rvs));
	mu *= btPow(p * rp0, -1/3.0f);
	mu *= s.friction;

	// carcass bending: yb = fy / kcb * (1 - u^2)
	//                  ym = ccb * dz * sin(camber) * (1 - u^2)
	// displacement: dx(x) = sx * (a - x)
	//               dy(x) = sy * (a - x) - ycb
	// shear stress: qx(x) = ktx * dx(x)
	//               qy(x) = kty * dy(x)
	// shear limit:  |q(x)| = mu * p(x)
	btScalar qx = ktx * sx * a;
	btScalar qy = kty * sy * a;
	btScalar qp = mu * p;
	btScalar qx2 = qx * qx;
	btScalar qp2 = qp * qp * (1/16.0f);
	btScalar waq = qp * wa;
	btScalar wah = 0.5f * wa;
	btScalar wak = 1/3.0f * wa * kty;
	btScalar ym = ccb * dz * sin_camber;

	btScalar fyn = 0, fyo = 0;
	btScalar yb, qb, uc, ud, ue, uf, tc, fc, ts, tb, fcy;
	for (int i = 0; i < 3; i++)
	{
		fyn = 0.5f * (fyn + fyo);
		fyo = fyn;

		yb = fyn * rkb + ym;
		qb = kty * yb;

		// (qy - qb * (1 + u))^2 + qx^2 = qp^2 * (1 + u)^2 * (1 + u / 4)^2
		uc = ComputeSlipPoint(qp2, qx2, qy, qb);
		ud = uc + 1;
		ue = uc - 1;
		uf = 3 * uc + 5;

		// fc = integrate w * mu * p(x) from -a to xc
		// fs = integrate w * q(x) from xc to a
		tc = waq * (ud * ud);
		fc = tc * (7/9.0f - 1/144.0f * (uf * uf));
		ts = wah * (ue * ue);
		tb = wak * ((uc * uc - 3) * uc + 2);
		fcy = fc * ny;
		fyn = cfy * (ts * qy - tb * ym + fcy) / (tb * rkb + 1);
	}
	btScalar fy = fyn;

	btScalar fcx = fc * nx;
	btScalar fsx = ts * qx;
	btScalar fx = fsx + fcx;

	// msz = integrate w * (x * qy(x) - ycb(x) * qx(x)) from xc to a
	btScalar tsy = ((4 * uc + 2) * qy - (3 * qb) * (ud * ud)) * (a * ts);
	btScalar tsx = (uf * ue) * (yb * fsx);
	btScalar msz = 1/6.0f * (tsy + tsx);

	// mcz = integrate w * mu * p(x) * (x * ny - ycb(x) * nx) from -a to xc
	btScalar tcy = (((3 * uc + 9) * uc - 26) * uc + 13) * (a * ny);
	btScalar tcx = (((5 * uc + 9) * uc - 57) * uc + 59) * (0.5f * ud) * (yb * nx);
	btScalar mcz = -1/60.0f * tc * (tcy + tcx);

	btScalar mz = msz + mcz;

	s.vcam = 0; // FIXME
	ComputeSlip(lon_velocity, lat_velocity, rot_velocity, s.slip, s.slip_angle);
	s.fx = fx;
	s.fy = fy;
	s.mz = mz;
}

btScalar CarTire3::getMaxFx(btScalar load) const
{
	btScalar fx;
	getMaxForce(load, &fx, 0, 0);
	return fx;
}

btScalar CarTire3::getMaxFy(btScalar load, btScalar /*camber*/) const
{
	btScalar fy, mz;
	getMaxForce(load, 0, &fy, &mz);
	return fy;
}

btScalar CarTire3::getMaxMz(btScalar load, btScalar /*camber*/) const
{
	btScalar fy, mz;
	getMaxForce(load, 0, &fy, &mz);
	return mz;
}

void CarTire3::getMaxForce(
	btScalar fz,
	btScalar * pfx,
	btScalar * pfy,
	btScalar * pmz) const
{
	btScalar vcx = 20; // sample at 72 kph

	btScalar s = 0.12, sa = 0.21;
	//getIdealSlip(fz, s, sa); FIXME

	btScalar dz = fz * rkz;
	btScalar w = width;
	if (dz < dz0)
	{
		btScalar sz = 1 - dz / dz0;
		w *= btSqrt(1 - sz * sz);
	}
	btScalar a = 0.3f * (dz + 2.25f * btSqrt(radius * dz));
	btScalar wa = w * a * 1E5f;
	btScalar wa2 = 0.5f * wa;
	btScalar wa3 = 1/3.0f * wa;

	btScalar p = 0.75f * fz / wa;
	btScalar mup = p * btPow(p * rp0, -1/3.0f);

	btScalar fx = 0;
	if (pfx)
	{
		btScalar vr = vcx * s;
		btScalar nx = 1;
		btScalar sx = s / (1 - s);
		btScalar qx = ktx * a * sx;

		btScalar muv = muc + (mus - muc) * btExp(-btSqrt(vr * rvs));
		btScalar qp = mup * muv;
		btScalar rq4 = 4 / qp;
		btScalar waq = qp * wa;

		// qx * (1 - u) = qp/4 * (1 - u^2) * (4 + u)
		btScalar t = btSqrt(qx * rq4 + 4.5f);
		btScalar uc = Min(btScalar(t - 2.5f), btScalar(1.0f));
		btScalar ud = uc + 1;
		btScalar ue = uc - 1;
		btScalar uf = 3 * uc + 5;

		btScalar ts = wa2 * (ue * ue);
		btScalar tc = waq * (ud * ud);
		btScalar fc = tc * (7/9.0f - 1/144.0f * (uf * uf));
		btScalar fcx = fc * nx;
		btScalar fsx = ts * qx;
		fx = fsx + fcx;

		*pfx = fx;
	}

	btScalar fy = 0;
	btScalar mz = 0;
	if (pfy)
	{
		btScalar ta = btTan(sa);
		btScalar vr = vcx * ta;
		btScalar ny = 1;
		btScalar sy = ta;
		btScalar qy = kty * a * sy;

		btScalar muv = muc + (mus - muc) * btExp(-btSqrt(vr * rvs));
		btScalar qp = mup * muv;
		btScalar rq4 = 4 / qp;
		btScalar rp4 = 4 * rq4 * qy;
		btScalar waq = qp * wa;

		btScalar fyo = 0;
		btScalar qb, uc, ud, ts, tc;
		for (int i = 0; i < 3; i++)
		{
			fy = 0.5f * (fy + fyo);
			fyo = fy;

			qb = kty * fy * rkb;

			// qy - qb * (1 + u) = qp/4 * (1 + u) * (4 + u)
			btScalar t0 = qb * rq4;
			btScalar t1 = t0 + 3;
			btScalar t2 = btSqrt(t1 * t1 + rp4);
			uc = Min(btScalar(0.5f * (t2 - t0 - 5)), btScalar(1.0f));
			ud = uc + 1;
			btScalar ue = uc - 1;
			btScalar uf = 3 * uc + 5;

			ts = wa2 * (ue * ue);
			tc = waq * (ud * ud);
			btScalar tb = wa3 * ((uc * uc - 3) * uc + 2);
			btScalar fc = tc * (7/9.0f - 1/144.0f * (uf * uf));
			btScalar fcy = fc * ny;
			btScalar fsy = ts * qy - tb * qb;
			fy = fsy + fcy;
		}

		btScalar tsy = ((4 * uc + 2) * qy - (3 * qb) * (ud * ud)) * (a * ts);
		btScalar tcy = (((3 * uc + 9) * uc - 26) * uc + 13) * (a * ny);
		btScalar msz = 1/6.0f * tsy;
		btScalar mcz = -1/60.0f * tc * tcy;
		mz = msz + mcz;

		*pfy = fy;
		*pmz = mz;
	}
}

void CarTire3::findIdealSlip(btScalar fz, btScalar slip[2]) const
{
	btScalar vcx = 20; // sample at 72 kph

	btScalar dz = fz * rkz;
	btScalar w = width;
	if (dz < dz0)
	{
		btScalar sz = 1 - dz / dz0;
		w *= btSqrt(1 - sz * sz);
	}
	btScalar a = 0.3f * (dz + 2.25f * btSqrt(radius * dz));
	btScalar wa = w * a * 1E5f;
	btScalar wa2 = 0.5f * wa;
	btScalar wa3 = 1/3.0f * wa;

	btScalar p = 0.75f * fz / wa;
	btScalar mup = p * btPow(p * rp0, -1/3.0f);

	btScalar ds = 5E-3;
	btScalar s = ds;
	btScalar fxn = 0;
	btScalar fcx;
	btScalar fsx;
	while (true)
	{
		btScalar vr = vcx * s;
		btScalar nx = 1;
		btScalar sx = s / (1 - s);
		btScalar qx = ktx * a * sx;

		btScalar muv = muc + (mus - muc) * btExp(-btSqrt(vr * rvs));
		btScalar qp = mup * muv;
		btScalar rq4 = 4 / qp;
		btScalar waq = qp * wa;

		// qx * (1 - u) = qp/4 * (1 - u^2) * (4 + u)
		btScalar t = btSqrt(qx * rq4 + 4.5f);
		btScalar uc = Min(btScalar(t - 2.5f), btScalar(1.0f));
		btScalar ud = uc + 1;
		btScalar ue = uc - 1;
		btScalar uf = 3 * uc + 5;

		btScalar ts = wa2 * (ue * ue);
		btScalar tc = waq * (ud * ud);
		btScalar fc = tc * (7/9.0f - 1/144.0f * (uf * uf));
		fcx = fc * nx;
		fsx = ts * qx;
		btScalar fx = fsx + fcx;

		// stop if slip > 0.5
		if (fx < fxn || s > 0.5f)
			break;

		fxn = fx;
		s += ds;
	}
	slip[0] = s - ds;

	btScalar dta = 0.02f;
	btScalar ta = dta;
	btScalar fyn = 0;
	while (true)
	{
		btScalar vr = vcx * ta;
		btScalar ny = 1;
		btScalar sy = ta;
		btScalar qy = kty * a * sy;

		btScalar muv = muc + (mus - muc) * btExp(-btSqrt(vr * rvs));
		btScalar qp = mup * muv;
		btScalar rq4 = 4 / qp;
		btScalar rp4 = 4 * rq4 * qy;
		btScalar waq = qp * wa;

		btScalar fy = 0;
		btScalar fyo = 0;
		for (int i = 0; i < 3; i++)
		{
			fy = 0.5f * (fy + fyo);
			fyo = fy;

			btScalar yb = fy * rkb;
			btScalar qb = kty * yb;

			// qy - qb * (1 + u) = qp/4 * (1 + u) * (4 + u)
			btScalar t0 = qb * rq4;
			btScalar t1 = t0 + 3;
			btScalar t2 = btSqrt(t1 * t1 + rp4);
			btScalar uc = Min(btScalar(0.5f * (t2 - t0 - 5)), btScalar(1.0f));
			btScalar ud = uc + 1;
			btScalar ue = uc - 1;
			btScalar uf = 3 * uc + 5;

			btScalar ts = wa2 * (ue * ue);
			btScalar tb = wa3 * ((uc * uc - 3) * uc + 2);
			btScalar tc = waq * (ud * ud);
			btScalar fc = tc * (7/9.0f - 1/144.0f * (uf * uf));
			btScalar fcy = fc * ny;
			btScalar fsy = ts * qy - tb * qb;
			fy = fsy + fcy;
		}

		// stop if slip angle > 60 deg
		if (fy < fyn || ta > 1.732f)
			break;

		fyn = fy;
		ta += dta;
	}
	slip[1] = Atan(ta - dta);
}

void CarTire3::initSlipLUT(CarTireSlipLUT & t) const
{
	for (int i = 0; i < t.size(); i++)
	{
		btScalar load = (i + 1) * t.delta();
		findIdealSlip(load, t.ideal_slip_lut[i]);
	}
}
