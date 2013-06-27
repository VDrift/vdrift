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

#include "sky.h"
#include "graphics_gl2.h"
#include "shader.h"
//#include "config.h"
#include <time.h>

#ifdef _WIN32
#include "strptime.h"
#endif

#define pi M_PI

const unsigned texture_size = 512;
const unsigned tiles_u = 4;
const unsigned tiles_v = 4;
const unsigned tiles_num = tiles_u * tiles_v;
const unsigned sides_num = 5;

const FBTEXTURE::CUBE_SIDE side_enum[sides_num] = {
	FBTEXTURE::POSX,
	FBTEXTURE::NEGX,
	FBTEXTURE::POSY,
	FBTEXTURE::NEGY,
	FBTEXTURE::POSZ,
};

typedef MATHVECTOR<float, 3> VEC3;
const VEC3 side_xyz[sides_num][3] = {
	{VEC3( 0,  0, -1), VEC3( 0, -1,  0), VEC3( 1,  0,  0)},	// POSX
	{VEC3( 0,  0,  1), VEC3( 0, -1,  0), VEC3(-1,  0,  0)},	// NEGX
	{VEC3( 1,  0,  0), VEC3( 0,  0,  1), VEC3( 0,  1,  0)},	// POSY
	{VEC3( 1,  0,  0), VEC3( 0,  0, -1), VEC3( 0, -1,  0)},	// NEGY
	{VEC3( 1,  0,  0), VEC3( 0, -1,  0), VEC3( 0,  0,  1)},	// POSZ
};

SKY::SKY(GRAPHICS_GL2 & gfx, std::ostream & error) :
	error_output(error),
	graphics(gfx)
{
	assert(graphics.GetUsingShaders());

	sky_textures[0].Init(texture_size, texture_size, FBTEXTURE::CUBEMAP, FBTEXTURE::RGB8, true, false, error_output);
	sky_textures[1].Init(texture_size, texture_size, FBTEXTURE::CUBEMAP, FBTEXTURE::RGB8, true, false, error_output);

	sky_fbos[0].Init(graphics.GetState(), std::vector<FBTEXTURE*>(1, &sky_textures[0]), error_output);
	sky_fbos[1].Init(graphics.GetState(), std::vector<FBTEXTURE*>(1, &sky_textures[1]), error_output);

	texture_active = 0;
	side_updated = 0;
	tile_updated = 0;

	wavelength.Set(0.65f, 0.57f, 0.475f);
	turbidity = 4;
	exposure = 1;
	timezone = -8;
	longitude = -121;
	latitude = 36;

	time_t seconds = time(NULL);
	struct tm datetime = *localtime(&seconds);
	SetTime(datetime);
}

SKY::~SKY()
{
	// dtor
}

bool SKY::Load(const std::string & path)
{
/*
	std::string cpath = path + "/sky.txt";
	CONFIG c;
	if (!c.Load(cpath))
	{
		error_output << "Can't find configfile: " << cpath << std::endl;
		return false;
	}

	c.GetParam("", "longitude", longitude);
	c.GetParam("", "latitude", latitude);
	c.GetParam("", "timezone", timezone);
	c.GetParam("", "azimuth", azdelta);
	azdelta = azdelta / 180 * pi;
	c.GetParam("", "turbidity", turbidity);
	c.GetParam("", "exposure", exposure);

	std::string timestr;
	if(c.GetParam("", "time", timestr))
	{
		strptime(timestr.c_str(), "%Y-%m-%d %H:%M:%S", &datetime);
		mktime(&datetime);
	}
	else
	{
		time_t seconds = time(NULL);
		datetime = *localtime(&seconds);
	}
*/
	return true;
}

void SKY::SetTime(const struct tm & value)
{
	day = float(value.tm_yday + 1);
	hour = float(value.tm_hour);
	minute = float(value.tm_min);
	second = float(value.tm_sec);
}

void SKY::SetTurbidity(float value)
{
	turbidity = value;
}

void SKY::SetExposure(float value)
{
	exposure = value;
}

void SKY::Update()
{
	assert(tile_updated < tiles_num);
	assert(side_updated < sides_num);

	unsigned texture_updated = (texture_active + 1) % 2;

	// tile corners in local space, (0, 0) top left and (1, 1) bottom right
	float v0 = float(tile_updated / tiles_v) / tiles_v;
	float u0 = float(tile_updated % tiles_v) / tiles_u;
	float v1 = v0 + 1.0f / tiles_v;
	float u1 = u0 + 1.0f / tiles_u;

	// tile corners in clip space, (-1, 1) top left and (1, -1) bottom right
	float y[2] = { 1 - v0 * 2, 1 - v1 * 2 };
	float x[2] = { u0 * 2 - 1, u1 * 2 - 1 };

	// tile corners in world space
	const VEC3 * xyz = side_xyz[side_updated];
	VEC3 v00 = xyz[0] * x[0] + xyz[1] * y[0] + xyz[2];
	VEC3 v10 = xyz[0] * x[1] + xyz[1] * y[0] + xyz[2];
	VEC3 v01 = xyz[0] * x[0] + xyz[1] * y[1] + xyz[2];
	VEC3 v11 = xyz[0] * x[1] + xyz[1] * y[1] + xyz[2];

	// tile quad
	const unsigned faces[2 * 3] = {
		0, 1, 2,
		2, 3, 0,
	};
	const float pos[4 * 3] = {
		x[0], y[0], 0,
		x[1], y[0], 0,
		x[1], y[1], 0,
		x[0], y[1], 0,
	};
	const float tco[4 * 3] = {
		v00[0],  v00[1], v00[2],
		v10[0],  v10[1], v10[2],
		v11[0],  v11[1], v11[2],
		v01[0],  v01[1], v01[2],
	};

	// set shader
	SHADER_GLSL * shader = graphics.GetShader("skygen");
	if (!shader)
	{
		assert(0);
		return;
	}
	shader->Enable();
	shader->UploadActiveShaderParameter3f("uLightDirection", sundir[0], sundir[1], sundir[2]);

	// disable blending, depth write
	graphics.GetState().Disable(GL_ALPHA_TEST);
	graphics.GetState().Disable(GL_BLEND);
	graphics.GetState().Disable(GL_DEPTH_TEST);
	graphics.GetState().SetDepthMask(false);

	// bind fbo
	FBOBJECT & fbo = sky_fbos[texture_updated];
	fbo.SetCubeSide(side_enum[side_updated]);
	fbo.Begin(graphics.GetState(), error_output);

	// draw
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glEnableClientState(GL_VERTEX_ARRAY);

	glVertexPointer(3, GL_FLOAT, 0, pos);
	glTexCoordPointer(3, GL_FLOAT, 0, tco);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, faces);

	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);

	fbo.End(graphics.GetState(), error_output);

	// move to next tile
	tile_updated += 1;
	if (tile_updated == tiles_num)
	{
		// reset tile counter and move to next cube side
		tile_updated = 0;
		side_updated += 1;

		if (side_updated == sides_num)
		{
			// set fully updated texture as active
			texture_active = texture_updated;

			// update sky properties
			UpdateSunDir();
			UpdateSunColor();

			// reset side counter
			side_updated = 0;
		}
	}
}

void SKY::UpdateComplete()
{
	side_updated = 0;
	tile_updated = 0;
    for (unsigned i = 0; i < tiles_num * sides_num; ++i)
	{
		Update();
	}
}

void SKY::UpdateSunDir()
{
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

	sundir.Set(1, 1, 1); // override sun direction for testing
}

void SKY::UpdateSunColor()
{
	// relative optical mass (Kasten and Young)
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

const SKY::VEC3 & SKY::GetSunColor() const
{
	return suncolor;
}

const SKY::VEC3 & SKY::GetSunDirection() const
{
	return sundir;
}

float SKY::GetZenith() const
{
	return ze;
}

float SKY::GetAzimuth() const
{
	return az;
}

bool SKY::Loaded() const
{
	return sky_textures[texture_active].Loaded();
}

void SKY::Activate() const
{
	return sky_textures[texture_active].Activate();
}

void SKY::Deactivate() const
{
	return sky_textures[texture_active].Deactivate();
}

unsigned SKY::GetW() const
{
	return sky_textures[texture_active].GetW();
}

unsigned SKY::GetH() const
{
	return sky_textures[texture_active].GetH();
}
