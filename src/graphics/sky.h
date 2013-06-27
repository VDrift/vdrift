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

#ifndef SKY_H
#define SKY_H

#include "fbobject.h"
#include "fbtexture.h"
#include "mathvector.h"

class GRAPHICS_GL2;
struct tm;

// Sky is double buffered to spread texture updates over multiple frames.
// Default parameters are for laguna seca raceway.
class SKY: public TEXTURE_INTERFACE
{
public:
	typedef MATHVECTOR<float, 3> VEC3;

	SKY(GRAPHICS_GL2 & gfx, std::ostream & error);

	~SKY();

	bool Load(const std::string & path);

	void SetTime(const struct tm & value);

	void SetTurbidity(float value);

	void SetExposure(float value);

	// A call to update renders a tile into current backbuffer.
	// Buffers are swapped when all tiles have been rendered.
	void Update();

	// Force full buffer update and swap
	void UpdateComplete();

	const VEC3 & GetSunColor() const;

	const VEC3 & GetSunDirection() const;

	float GetZenith() const;

	float GetAzimuth() const;

	bool Loaded() const;

	void Activate() const;

	void Deactivate() const;

	unsigned GetW() const;

	unsigned GetH() const;

private:
	std::ostream & error_output;
	GRAPHICS_GL2 & graphics;

	FBOBJECT sky_fbos[2];
	FBTEXTURE sky_textures[2];	// double buffered sky cube map
	unsigned texture_active;	// curent frontbuffer id [0, 1]
	unsigned side_updated;		// currently updated cube map [0, 4]
	unsigned tile_updated;		// currently updated tile [0, tiles_num)

	// scattering params
	VEC3 suncolor;
	VEC3 wavelength;
	float turbidity;
	float exposure;

	// sun position
	VEC3 sundir;
	float ze;			// solar zenith angle
	float az;			// solar azimuth angle (clockwise from north)
	float azdelta;		// local orientation offset relative to north(z-axis)
	float timezone;		// timezone in hours from UTC
	float longitude;	// in degrees
	float latitude;		// in degrees
	float day;
	float hour;
	float minute;
	float second;

	void UpdateSunDir();

	void UpdateSunColor();
};

#endif // SKY_H
