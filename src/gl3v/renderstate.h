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

#ifndef _RENDERSTATE
#define _RENDERSTATE

#include "glwrapper.h"
#include "renderpassinfo.h"
#include "glenums.h"

class RenderState
{
	public:
		void apply(GLWrapper & gl) const;
		void applySampler(GLWrapper & gl, GLuint samplerHandle) const;
		void debugPrint(std::ostream & out, const GLEnums & GLEnumHelper) const;

		RenderState() {}
		RenderState(GLenum parameter, RealtimeExportPassInfo::RenderState s, const GLEnums & GLEnumHelper);

	private:
		GLenum pname;
		enum RenderStateType
		{
			SS_ENUM,
			SS_FLOAT,
			SS_FLOAT2,
			SS_FLOAT4, //(color)
			SS_INT
		} type;
		GLint param[4];
		GLfloat fparam[4];
};

#endif
