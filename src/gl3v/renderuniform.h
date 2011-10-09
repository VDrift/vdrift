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

#ifndef _RENDERUNIFORM
#define _RENDERUNIFORM

#include "renderuniformentry.h"
#include "renderuniformvector.h"
#include "glew.h"
#include "gl.h"

/// The bare minimum required to update uniforms.
struct RenderUniform : public RenderUniformBase
{
	RenderUniform();
	RenderUniform(GLint loc, const RenderUniformEntry & entry);
	RenderUniform(GLint loc, const std::vector <float> & newdata);

	GLuint location;
};

#endif
