#include "cartire.h"

#include <cmath>

#if defined(_WIN32) || defined(__APPLE__)
template <typename T> bool isnan(T number) {return (number != number);}
#endif

template <typename T>
CARTIREINFO<T>::CARTIREINFO() :
	aspect_ratio(0),
	sidewall_width(0),
	tread(0),
	longitudinal(11),
	lateral(15),
	aligning(18)
{

}

template <typename T>
CARTIRE<T>::CARTIRE() :
	camber(0),
	slide(0),
	slip(0),
	ideal_slide(0),
	ideal_slip(0)
{
	
}

template <typename T>
void CARTIRE<T>::Init(const CARTIREINFO <T> & info)
{
	this->info = info;
	CalculateSigmaHatAlphaHat();
}

template <typename T>
MATHVECTOR <T, 3> CARTIRE<T>::GetForce(
	const T normal_force,
	const T friction_coeff,
	const MATHVECTOR <T, 3> & velocity,
	const T ang_velocity,
	const T inclination)
{
	assert(friction_coeff > 0);

	// tire off ground
	if (normal_force < 1e-3) return MATHVECTOR <T, 3> (0);

	// get ideal slip ratio
	T sigma_hat(0);
	T alpha_hat(0);
	LookupSigmaHatAlphaHat(normal_force, sigma_hat, alpha_hat);

	// cap Fz at a magic number to prevent explosions
	T Fz = normal_force * 0.001;
	if (Fz > 30) Fz = 30;

	T gamma = inclination;
	T denom = std::max(std::abs(velocity[0]), T(0.1));
	T sigma = (ang_velocity * info.radius - velocity[0]) / denom;
	T alpha = -atan2(velocity[1], denom) * 180.0 / M_PI;

	// beckman method for pre-combining longitudinal and lateral forces
	T s = sigma / sigma_hat;
	T a = alpha / alpha_hat;
	T rho = std::max(T(sqrt(s * s + a * a)), T(0.0001)); // avoid divide-by-zero
	assert(!isnan(s));
	assert(!isnan(a));
	assert(!isnan(rho));
	
	T max_Fx(0);
	T max_Fy(0);
	T max_Mz(0);
	T Fx = (s / rho) * Pacejka_Fx(rho * sigma_hat, Fz, friction_coeff, max_Fx);
	T Fy = (a / rho) * Pacejka_Fy(rho * alpha_hat, Fz, gamma, friction_coeff, max_Fy);

	//std::cout << "s=" << s << ", rho=" << rho << ", sigma_hat=" << sigma_hat;
	//std::cout << ", Fz=" << Fz << ", friction_coeff=" << friction_coeff << ", Fx=" << Fx << std::endl;
	//std::cout << "s=" << s << ", a=" << a << ", rho=" << rho << ", Fy=" << Fy << std::endl;
	
	//T tan_alpha = hub_velocity [1] / denom;
	//T slip_x = -sigma / ( 1.0 + generic_abs ( sigma ) );
	//T slip_y = tan_alpha / ( 1.0+generic_abs ( sigma-1.0 ) );
	//T total_slip = std::sqrt ( slip_x * slip_x + slip_y * slip_y );
	//T maxforce = longitudinal_parameters[2] * 7.0;
	//std::cout << maxforce << ", " << max_Fx << ", " << max_Fy << ", " << Fx << ", " << Fy << std::endl;

	//combining method 0: no combining! :-)

/*
	//combining method 1: traction circle
	//determine to what extent the tires are long (x) gripping vs lat (y) gripping
	float longfactor = 1.0;
	float combforce = std::abs(Fx)+std::abs(Fy);
	if (combforce > 1) //avoid divide by zero (assume longfactor = 1 for this case)
		longfactor = std::abs(Fx)/combforce; //1.0 when Fy is zero, 0.0 when Fx is zero
	//determine the maximum force for this amount of long vs lat grip
	float maxforce = std::abs(max_Fx)*longfactor + (1.0-longfactor)*std::abs(max_Fy); //linear interpolation
	if (combforce > maxforce) //cap forces
	{
		//scale down forces to fit into the maximum
		Fx *= maxforce / combforce;
		Fy *= maxforce / combforce;
		//std::cout << "Limiting " << combforce << " to " << maxforce << std::endl;
	}
*/
/*	
	//combining method 2: traction ellipse (prioritize Fx)
	//std::cout << "Fy0=" << Fy << ", ";
	if (Fx >= max_Fx)
	{
		Fx = max_Fx;
		Fy = 0;
	}
	else
		Fy = Fy*sqrt(1.0-(Fx/max_Fx)*(Fx/max_Fx));
	//std::cout << "Fy=" << Fy << ", Fx=Fx0=" << Fx << ", Fxmax=" << max_Fx << ", Fymax=" << max_Fy << std::endl;
*/
/*
	//combining method 3: traction ellipse (prioritize Fy)
	if (Fy >= max_Fy)
	{
		Fy = max_Fy;
		Fx = 0;
	}
	else
	{
		T scale = sqrt(1.0-(Fy/max_Fy)*(Fy/max_Fy));
		if (isnan(scale))
			Fx = 0;
		else
			Fx = Fx*scale;
	}
*/
/*
	// modified Nicolas-Comstock Model(Modeling Combined Braking and Steering Tire Forces)
	T Fx0 = Pacejka_Fx(sigma, Fz, friction_coeff, max_Fx);
	T Fy0 = Pacejka_Fy(alpha, Fz, gamma, friction_coeff, max_Fy);
	// 0 <= a <= pi/2 and 0 <= s <= 1
	// Cs = a0 and Ca = b0 longitudinal and lateral slip stiffness ?
	T Fc = Fx0 * Fy0 / sqrt(s * s * Fy0 * Fy0 + Fx0 * Fx0 * tana * tana);
	T Fx = Fc * sqrt(s * s * Ca * Ca + (1-s) * (1-s) * cosa * cosa * Fx0 * Fx0) / Ca; 
	T Fy = Fc * sqrt((1-s) * (1-s) * cosa * cosa * Fy0 * Fy0 + sina * sina * Cs * Cs) / (Cs * cosa);
*/

	T Mz = Pacejka_Mz(sigma, alpha, Fz, gamma, friction_coeff, max_Mz);
	assert(!isnan(Fx));
	assert(!isnan(Fy));

	feedback = Mz;
	camber = inclination;
	slide = sigma;
	slip = alpha;
	ideal_slide = sigma_hat;
	ideal_slip = alpha_hat;

	return MATHVECTOR <T, 3> (Fx, Fy, Mz);
}

template <typename T>
T CARTIRE<T>::GetRollingResistance(const T velocity, const T normal_force, const T rolling_resistance_factor) const
{
	// surface influence on rolling resistance
	T rolling_resistance = info.rolling_resistance_linear * rolling_resistance_factor;
	
	// heat due to tire deformation increases rolling resistance, approximate by quadratic function
	rolling_resistance += velocity * velocity * info.rolling_resistance_quadratic;
	
	return normal_force * rolling_resistance;
}

template <typename T>
T CARTIRE<T>::GetMaximumFx(T load) const
{
	const std::vector <T> & b = info.longitudinal;
	T Fz = load * 0.001;
	return (b[1] * Fz + b[2]) * Fz;
}

template <typename T>
T CARTIRE<T>::GetMaximumFy(T load, T camber) const
{
	const std::vector <T> & a = info.lateral;
	T Fz = load * 0.001;
	T gamma = camber;

	T D = (a[1] * Fz + a[2]) * Fz;
	//T Sv = a[11] * Fz * gamma + a[12] * Fz + a[13]; // pacejka89
	T Sv = ((a[11] * Fz + a[12]) * gamma + a[13] ) * Fz + a[14]; // pacejka96 ?

	return D + Sv;
}

template <typename T>
T CARTIRE<T>::GetMaximumMz(T load, T camber) const
{
	const std::vector <T> & c = info.aligning;
	T Fz = load * 0.001;
	T gamma = camber;

	T D = (c[1] * Fz + c[2]) * Fz;
	T Sv = (c[14] * Fz * Fz + c[15] * Fz) * gamma + c[16] * Fz + c[17];

	return -(D + Sv);
}

template <typename T>
void CARTIRE<T>::LookupSigmaHatAlphaHat(T load, T & sh, T & ah) const
{
	assert(!sigma_hat.empty());
	assert(!alpha_hat.empty());
	assert(sigma_hat.size() == alpha_hat.size());

	int HAT_ITERATIONS = sigma_hat.size();

	T HAT_LOAD = 0.5;
	T nf = load * 0.001;
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
		T blend = (nf-HAT_LOAD*(lbound+1))/HAT_LOAD;
		sh = sigma_hat[lbound]*(1.0-blend)+sigma_hat[lbound+1]*blend;
		ah = alpha_hat[lbound]*(1.0-blend)+alpha_hat[lbound+1]*blend;
	}
}

template <typename T>
T CARTIRE<T>::Pacejka_Fx(T sigma, T Fz, T friction_coeff, T & max_Fx)
{
	const std::vector <T> & b = info.longitudinal;

	// peak factor
	T D = (b[1] * Fz + b[2]) * Fz * friction_coeff;

	// stiffness factor
	T B = (b[3] * Fz + b[4]) * exp(-b[5] * Fz) / (b[0] * (b[1] * Fz + b[2]));

	// curvature factor
	T E = (b[6] * Fz * Fz + b[7] * Fz + b[8]);
	
	// slip + horizontal shift
	T S = (100 * sigma + b[9] * Fz + b[10]);
	
	// longitudinal force
	T Fx = D * sin(b[0] * atan(B * S - E * (B * S - atan(B * S))));

	max_Fx = D;

	assert(!isnan(Fx));
	return Fx;
}

template <typename T>
T CARTIRE<T>::Pacejka_Fy(T alpha, T Fz, T gamma, T friction_coeff, T & max_Fy)
{
	const std::vector <T> & a = info.lateral;

	// peak factor
	T D = (a[1] * Fz + a[2]) * Fz * friction_coeff;
	
	// stiffness factor
	T B = a[3] * sin(2.0 * atan(Fz / a[4])) * (1.0 - a[5] * std::abs(gamma)) / (a[0] * (a[1] * Fz + a[2]) * Fz);
	
	// curvature factor
	T E = a[6] * Fz + a[7];
	
	// slip angle + horizontal shift
	T S = alpha + a[8] * gamma + a[9] * Fz + a[10];
	
	// vertical shift
	//T Sv = a[11] * Fz * gamma + a[12] * Fz + a[13]; // pacejka89
	T Sv = ((a[11] * Fz + a[12]) * gamma + a[13]) * Fz + a[14]; // pacejka96?
	
	// lateral force
	T Fy = D * sin(a[0] * atan(B * S - E * (B * S - atan(B * S)))) + Sv;

	max_Fy = D + Sv;

	assert(!isnan(Fy));
	return Fy;
}

template <typename T>
T CARTIRE<T>::Pacejka_Mz(T sigma, T alpha, T Fz, T gamma, T friction_coeff, T & max_Mz)
{
	const std::vector <T> & c = info.aligning;

	// peak factor
	T D = (c[1] * Fz + c[2]) * Fz * friction_coeff;
	
	// stiffness factor
	T B = (c[3] * Fz * Fz + c[4] * Fz) * (1.0 - c[6] * std::abs(gamma)) * exp (-c[5] * Fz) / (c[0] * D);
	
	// curvature factor
	T E = (c[7] * Fz * Fz + c[8] * Fz + c[9] ) * (1.0 - c[10] * std::abs(gamma));
	
	// slip angle + horizontal shift
	T S = alpha + c[11] * gamma + c[12] * Fz + c[13];
	
	// vertical shift
	T Sv = (c[14] * Fz * Fz + c[15] * Fz) * gamma + c[16] * Fz + c[17];
	
	// self-aligning torque
	T Mz = D * sin(c[0] * atan(B * S - E * (B * S - atan(B * S)))) + Sv;

	max_Mz = D + Sv;

	assert(!isnan(Mz));
	return Mz;
}

template <typename T>
void CARTIRE<T>::FindSigmaHatAlphaHat(T load, T & output_sigmahat, T & output_alphahat, int iterations)
{
	T x, y, ymax, junk;
	ymax = 0;
	for (x = -2; x < 2; x += 4.0/iterations)
	{
		y = Pacejka_Fx(x, load, 1.0, junk);
		if (y > ymax)
		{
			output_sigmahat = x;
			ymax = y;
		}
	}

	ymax = 0;
	for (x = -20; x < 20; x += 40.0/iterations)
	{
		y = Pacejka_Fy(x, load, 0, 1.0, junk);
		if (y > ymax)
		{
			output_alphahat = x;
			ymax = y;
		}
	}
}

template <typename T>
void CARTIRE<T>::CalculateSigmaHatAlphaHat(int tablesize)
{
	T HAT_LOAD = 0.5;
	sigma_hat.resize(tablesize, 0);
	alpha_hat.resize(tablesize, 0);
	for (int i = 0; i < tablesize; i++)
	{
		FindSigmaHatAlphaHat((T)(i+1)*HAT_LOAD, sigma_hat[i], alpha_hat[i]);
	}
}

/// explicit instantiation
template class CARTIREINFO <float>;
template class CARTIREINFO <double>;

template class CARTIRE <float>;
template class CARTIRE <double>;
