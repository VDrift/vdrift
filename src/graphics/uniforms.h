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

#ifndef _UNIFORMS_H
#define _UNIFORMS_H

/// gl2 renderer built-in uniforms
namespace Uniforms
{
	enum Enum
	{
		ModelViewProjMatrix,
		ModelViewMatrix,
		ProjectionMatrix,
		ReflectionMatrix,
		ShadowMatrix,
		LightDirection,
		ColorTint,
		Contrast,
		ZNear,
		FrustumCornerBL,
		FrustumCornerBRDelta,
		FrustumCornerTLDelta,
		UniformNum
	};

	static const char * const str[] =
	{
		"ModelViewProjMatrix",
		"ModelViewMatrix",
		"ProjectionMatrix",
		"ReflectionMatrix",
		"ShadowMatrix",
		"light_direction",
		"color_tint",
		"contrast",
		"znear",
		"frustum_corner_bl",
		"frustum_corner_br_delta",
		"frustum_corner_tl_delta"
	};
}

#endif
