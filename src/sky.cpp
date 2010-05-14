#include "sky.h"
#include "configfile.h"
#include "graphics.h"

#ifdef WIN32
#include "../tools/win/strptime.h"
#endif

#define pi 3.14159265358979323846f

// default parameters are for laguna seca raceway
SKY::SKY(GRAPHICS_SDLGL & gs, std::ostream & info, std::ostream & error)
: graphics(gs), info_output(info), error_output(error)
{
	wavelength.Set(0.65f, 0.57f, 0.475f);
	turbidity = 4;
	exposure = 1;
	timezone = -8;
	longitude = -121;
	latitude = 36;
	time_t seconds = time(NULL);
	datetime = *localtime(&seconds);
}

bool SKY::Load(const std::string & path)
{
	std::string cpath = path + "/sky.txt";
	CONFIGFILE c;
	if (!c.Load(cpath))
	{
		error_output << "Can't find configfile: " << cpath << std::endl;
		return false;
	}
	
	c.GetParam("longitude", longitude);
	c.GetParam("latitude", latitude);
	c.GetParam("timezone", timezone);
	c.GetParam("azimuth", azdelta);
	azdelta = azdelta / 180 * pi;
	c.GetParam("turbidity", turbidity);
	c.GetParam("exposure", exposure);
	
	std::string timestr;
	if(c.GetParam("time", timestr))
	{
		strptime(timestr.c_str(), "%Y-%m-%d %H:%M:%S", &datetime);
		mktime(&datetime);
	}
	else
	{
		time_t seconds = time(NULL);
		datetime = *localtime(&seconds);
	}
	
	sky_texture.Init(512, 512, FBTEXTURE::NORMAL, false, false, false, true, error_output);
	
	Update();
	
	return true;
}

void SKY::Update()
{
	UpdateSunDir();
	UpdateSunColor();
	UpdateSkyTexture();
}

void SKY::UpdateSunDir()
{
	float day = (float)datetime.tm_yday+1;
	float hour = (float)datetime.tm_hour;
	float minute = (float)datetime.tm_min;
	float second = (float)datetime.tm_sec;
	float lat = latitude * pi / 180;

	// fractional year
	float y = (2 * pi / 365) * (day - 1 + (hour - 12) / 24);

	// equation of time(minutes) corrects for the eccentricity of the Earth's orbit and axial tilt
	float eot = 229.18f * (0.000075f + 0.001868f * cos(y) - 0.032077f * sin(y) - 0.014615f * cos(2*y) - 0.040849f * sin(2*y));

	// solar declination angle
	float decl = 0.006918f - 0.399912f * cos(y) + 0.070257f * sin(y) - 0.006758f * cos(2*y) + 0.000907f * sin(2*y) - 0.002697f * cos(3*y) + 0.00148f * sin(3*y);

	// solar time offset in minutes (logitude in degrees, timezone in hours from UTC)
	float time_offset = eot - 4 * longitude + 60 * timezone;

	// true solar time in minutes
	float solar_time = hour * 60 + minute + second / 60 + time_offset;

	// solar hour angle (15 degrees per hour)
	float ha = (solar_time / 4 - 180) * pi / 180;

	// solar zenith angle
	float cos_ze = sin(lat) * sin(decl) + cos(lat) * cos(decl) * cos(ha);
	ze = acos(cos_ze);

	// solar azimuth angle (clockwise from north)
	float cos_az = 0;
	az = 0;
	float denom = cos(lat) * sin(ze);
	if (denom != 0)
	{
		cos_az = (sin(lat) * cos_ze - sin(decl)) / denom;
		az = acos(cos_az);
		if (ha < 0) az = pi - az;
		else az = pi + az;
	}
	
	az += azdelta;
	sundir[0] = sin(az) * sin(ze);   // east
	sundir[1] = cos(ze);             // up
	sundir[2] = cos(az) * sin(ze);   // north
}

void SKY::UpdateSunColor()
{
	// relative optical mass(Kasten and Young)
	float m = 38;
	if (ze < pi / 2)
	{
		m = 1 / (cos(ze) + 0.50572f * pow(96.07995f - ze / pi * 180.0f, -1.6364f));
	}
	
	// angstrom turbidity coefficient (0-0.5)
	float beta = 0.04608365822050f * turbidity - 0.04586025928522f;
	
	// rayleigh scattering + aerosol transmittance
	float tau[] = { 0, 0, 0 };
	for (int i = 0; i < 3; i++)
	{
		float tauR = exp(-m * 0.008735f * pow(wavelength[i], 4.0f));
		float tauA = exp(-m * beta * pow(wavelength[i], -1.3f));
		tau[i] = tauR * tauA;
	}

	suncolor[0] = tau[0];
	suncolor[1] = tau[1];
	suncolor[2] = tau[2];
}

void SKY::UpdateSkyTexture()
{/*
	if (!graphics.GetUsingShaders()) return;
	SHADER_GLSL & shader = graphics.GetShader("sky_scatter");
	shader.UploadActiveShaderParameter1f("exposure", exposure);
	shader.UploadActiveShaderParameter1f("turbidity", 0.005*turbidity);
	shader.UploadActiveShaderParameter3f("sundir", sundir[0], sundir[1], sundir[2]);
	graphics.DrawScreenQuad(shader, NULL, &sky_texture, error_output);
	//sky_texture.Screenshot("sky.bmp", error_output);*/
}

void SKY::Draw(MATRIX4<float> & iviewproj, FBTEXTURE * output)
{/*
	SHADER_GLSL & shader = graphics.GetShader("sky_draw");
	shader.UploadMat16("iviewproj", iviewproj.GetArray());
	graphics.DrawScreenQuad(shader, &sky_texture, output, error_output);*/
}
