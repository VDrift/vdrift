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
#include "vertexattrib.h"
//#include "config.h"

#include <time.h>

#ifdef _MSC_VER
#include "strptime.h"
#endif

#define pi M_PI

const unsigned texture_size = 512;
const unsigned tiles_u = 4;
const unsigned tiles_v = 4;
const unsigned tiles_num = tiles_u * tiles_v;
const unsigned sides_num = 5;

const FrameBufferTexture::CubeSide side_enum[sides_num] = {
	FrameBufferTexture::POSX,
	FrameBufferTexture::NEGX,
	FrameBufferTexture::POSY,
	FrameBufferTexture::NEGY,
	FrameBufferTexture::POSZ,
};

const Vec3 side_xyz[sides_num][3] = {
	{Vec3( 0,  0, -1), Vec3( 0, -1,  0), Vec3( 1,  0,  0)},	// POSX
	{Vec3( 0,  0,  1), Vec3( 0, -1,  0), Vec3(-1,  0,  0)},	// NEGX
	{Vec3( 1,  0,  0), Vec3( 0,  0,  1), Vec3( 0,  1,  0)},	// POSY
	{Vec3( 1,  0,  0), Vec3( 0,  0, -1), Vec3( 0, -1,  0)},	// NEGY
	{Vec3( 1,  0,  0), Vec3( 0, -1,  0), Vec3( 0,  0,  1)},	// POSZ
};

Sky::Sky(GraphicsGL2 & gfx, std::ostream & error) :
	error_output(error),
	graphics(gfx),
	sky_shader(NULL),
	sundir_uniform(0),
	texture_active(0),
	side_updated(0),
	tile_updated(0),
	sundir(0, 0, 1),
	suncolor(1, 1, 1),
	wavelength(0.65, 0.57, 0.475),
	turbidity(4),
	exposure(1),
	ze(0),
	az(0),
	azdelta(0),
	timezone(-8),
	longitude(-121),
	latitude(36),
	time_multiplier(1),
	time_delta(0),
	need_update(false)
{
	sky_shader = graphics.GetShader("skygen");
	assert(sky_shader);

	sundir_uniform = sky_shader->RegisterUniform("uLightDirection");

	target = FrameBufferTexture::CUBEMAP;
	width = texture_size;
	height = texture_size;

	sky_textures[0].Init(width, height, FrameBufferTexture::CUBEMAP, FrameBufferTexture::RGB8, true, false, error_output);
	sky_textures[1].Init(width, height, FrameBufferTexture::CUBEMAP, FrameBufferTexture::RGB8, true, false, error_output);

	sky_fbos[0].Init(graphics.GetState(), std::vector<FrameBufferTexture*>(1, &sky_textures[0]), error_output);
	sky_fbos[1].Init(graphics.GetState(), std::vector<FrameBufferTexture*>(1, &sky_textures[1]), error_output);

	time_t seconds = time(NULL);
	struct tm datetime = *localtime(&seconds);
	SetTime(datetime);
}

Sky::~Sky()
{
	// dtor
}

bool Sky::Load(const std::string & /*path*/)
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

void Sky::SetTimeSpeed(float value)
{
	time_multiplier = value;
}

void Sky::SetTime(float hours)
{
	hour = hours;
	minute = 0;
	second = 0;
	ResetSky();
}

void Sky::SetTime(const struct tm & value)
{
	day = float(value.tm_yday + 1);
	hour = float(value.tm_hour);
	minute = float(value.tm_min);
	second = float(value.tm_sec);
	ResetSky();
}

void Sky::SetTurbidity(float value)
{
	turbidity = value;
	ResetSky();
}

void Sky::SetExposure(float value)
{
	exposure = value;
	ResetSky();
}

void Sky::Update(float dt)
{
	if (need_update || dt * time_multiplier > 0)
	{
		time_delta += dt * time_multiplier;
		UpdateTime();
		UpdateSunDir();
		UpdateSunColor();
		UpdateSky();
	}
}

void Sky::UpdateComplete()
{
	side_updated = 0;
	tile_updated = 0;
	for (unsigned i = 0; i < tiles_num * sides_num; ++i)
		UpdateSky();
}

void Sky::Draw(unsigned elems, const unsigned faces[], const float pos[], const float tco[])
{
	using namespace VertexAttrib;

	// disable blending, depth write
	graphics.GetState().DepthTest(GL_ALWAYS, false);
	graphics.GetState().Blend(false);

	// bind fbo
	unsigned texture_updated = (texture_active + 1) % 2;
	FrameBufferObject & fbo = sky_fbos[texture_updated];
	fbo.SetCubeSide(side_enum[side_updated]);
	fbo.Begin(graphics.GetState(), error_output);

	// reset vertex state
	graphics.GetState().ResetVertexObject();

	// draw
	glEnableVertexAttribArray(VertexPosition);
	glEnableVertexAttribArray(VertexTexCoord);

	glVertexAttribPointer(VertexPosition, 3, GL_FLOAT, GL_FALSE, 0, pos);
	glVertexAttribPointer(VertexTexCoord, 3, GL_FLOAT, GL_FALSE, 0, tco);

	glDrawElements(GL_TRIANGLES, elems, GL_UNSIGNED_INT, faces);

	glDisableVertexAttribArray(VertexPosition);
	glDisableVertexAttribArray(VertexTexCoord);

	// unbind fbo
	fbo.End(graphics.GetState(), error_output);
}

void Sky::ResetSky()
{
	side_updated = 0;
	tile_updated = 0;
	need_update = true;
}

void Sky::UpdateSky()
{
	assert(sky_shader);
	assert(tile_updated < tiles_num);
	assert(side_updated < sides_num);

	// tile corners in local space, (0, 0) top left and (1, 1) bottom right
	float v0 = float(tile_updated / tiles_v) / tiles_v;
	float u0 = float(tile_updated % tiles_v) / tiles_u;
	float v1 = v0 + 1.0f / tiles_v;
	float u1 = u0 + 1.0f / tiles_u;

	// tile corners in clip space, (-1, 1) top left and (1, -1) bottom right
	float y[2] = { 1 - v0 * 2, 1 - v1 * 2 };
	float x[2] = { u0 * 2 - 1, u1 * 2 - 1 };

	// tile corners in world space
	const Vec3 * xyz = side_xyz[side_updated];
	Vec3 v00 = xyz[0] * x[0] + xyz[1] * y[0] + xyz[2];
	Vec3 v10 = xyz[0] * x[1] + xyz[1] * y[0] + xyz[2];
	Vec3 v01 = xyz[0] * x[0] + xyz[1] * y[1] + xyz[2];
	Vec3 v11 = xyz[0] * x[1] + xyz[1] * y[1] + xyz[2];

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

	sky_shader->Enable();
	sky_shader->SetUniform3f(sundir_uniform, sundir);

	// draw tile
	Draw(6, faces, pos, tco);

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
			unsigned texture_updated = (texture_active + 1) % 2;
			texture_active = texture_updated;
			texid = sky_textures[texture_active].GetId();

			// reset side counter and update flag
			side_updated = 0;
			need_update = false;
		}
	}
}

void Sky::UpdateTime()
{
	// handle seconds and minutes overflow, clamp hours for now
	second += time_delta;
	if (second >= 60)
	{
		float minutes_delta = floorf(second / 60);
		second -= minutes_delta * 60;
		minute += minutes_delta;
		if (minute >= 60)
		{
			float hours_delta = floorf(minute / 60);
			minute -= hours_delta * 60;
			hour += hours_delta;
			if (hour >= 24)
				hour = 0;
		}
	}
	//error_output << "dt " << time_delta << "  h " << hour << "  m " << minute << "  s " << second << std::endl;
	time_delta = 0;
}

void Sky::UpdateSunDir()
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
	sundir[1] = cos(az) * sin(ze);   // north
	sundir[2] = cos(ze);             // up

	//error_output << "az " << az << "  ze " << ze << std::endl;
}

void Sky::UpdateSunColor()
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

const Vec3 & Sky::GetSunColor() const
{
	return suncolor;
}

const Vec3 & Sky::GetSunDirection() const
{
	return sundir;
}

float Sky::GetZenith() const
{
	return ze;
}

float Sky::GetAzimuth() const
{
	return az;
}

bool Sky::Loaded() const
{
	return sky_textures[texture_active].Loaded();
}
