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

#include "renderstate.h"
#include "glenums.h"
#include <cassert>

void RenderState::apply(GLWrapper & gl) const
{
	static GLenum last_blendequation_rgb = GL_FUNC_ADD;
	static GLenum last_blendequation_alpha = GL_FUNC_ADD;
	static GLenum last_blend_src_rgb = GL_ONE;
	static GLenum last_blend_src_alpha = GL_ONE;
	static GLenum last_blend_dst_rgb = GL_ZERO;
	static GLenum last_blend_dst_alpha = GL_ZERO;

	switch (pname)
	{
		case GL_DEPTH_FUNC:
		gl.DepthFunc(param[0]);
		break;

		case GL_DEPTH_WRITEMASK:
		gl.DepthMask(param[0]);
		break;

		case GL_CULL_FACE_MODE:
		gl.CullFace(param[0]);
		break;

		case GL_FRONT_FACE:
		gl.FrontFace(param[0]);
		break;

		case GL_POLYGON_MODE:
		gl.PolygonMode(param[0]);
		break;

		case GL_POLYGON_OFFSET_FACTOR:
		gl.PolygonOffset(fparam[0], fparam[1]);
		break;

		case GL_SAMPLE_COVERAGE_VALUE:
		gl.SampleCoverage(fparam[0], (fparam[1] != 0));
		break;

		case GL_SAMPLE_MASK_VALUE:
		gl.SampleMaski(0, param[0]);
		break;

		case GL_LINE_SMOOTH_HINT:
		case GL_POLYGON_SMOOTH_HINT:
		case GL_FRAGMENT_SHADER_DERIVATIVE_HINT:
		gl.Hint(pname, param[0]);
		break;

		case GL_BLEND_EQUATION_RGB:
		last_blendequation_rgb = param[0];
		gl.BlendEquationSeparate(last_blendequation_rgb, last_blendequation_alpha);
		break;

		case GL_BLEND_EQUATION_ALPHA:
		last_blendequation_alpha = param[0];
		gl.BlendEquationSeparate(last_blendequation_rgb, last_blendequation_alpha);
		break;

		case GL_BLEND_SRC_RGB:
		last_blend_src_rgb = param[0];
		gl.BlendFuncSeparate(last_blend_src_rgb, last_blend_dst_rgb, last_blend_src_alpha, last_blend_dst_alpha);
		break;

		case GL_BLEND_SRC_ALPHA:
		last_blend_src_alpha = param[0];
		gl.BlendFuncSeparate(last_blend_src_rgb, last_blend_dst_rgb, last_blend_src_alpha, last_blend_dst_alpha);
		break;

		case GL_BLEND_DST_RGB:
		last_blend_dst_rgb = param[0];
		gl.BlendFuncSeparate(last_blend_src_rgb, last_blend_dst_rgb, last_blend_src_alpha, last_blend_dst_alpha);
		break;

		case GL_BLEND_DST_ALPHA:
		last_blend_dst_alpha = param[0];
		gl.BlendFuncSeparate(last_blend_src_rgb, last_blend_dst_rgb, last_blend_src_alpha, last_blend_dst_alpha);
		break;

		default:
		assert(0 && "Don't know how to apply state variable");
		break;
	}
}

void RenderState::applySampler(GLWrapper & gl, GLuint samplerHandle) const
{
	// abort if we don't support anisotropic texture filtering and that's what we're about to set
	if (pname == GL_TEXTURE_MAX_ANISOTROPY_EXT && !GLC_EXT_texture_filter_anisotropic)
		return;

	switch (type)
	{
		case SS_ENUM:
		case SS_INT:
		gl.SamplerParameteri(samplerHandle, pname, param[0]);
		break;

		case SS_FLOAT:
		gl.SamplerParameterf(samplerHandle, pname, fparam[0]);
		break;

		case SS_FLOAT4:
		gl.SamplerParameterfv(samplerHandle, pname, fparam);
		break;

		default:
		assert(!"Invalid sampler state type");
	}
}

void RenderState::debugPrint(std::ostream & out, const GLEnums & GLEnumHelper) const
{
	out << GLEnumHelper.getEnum(pname) << ": ";
	switch (type)
	{
		case SS_ENUM:
		out << GLEnumHelper.getEnum(param[0]);
		break;

		case SS_INT:
		out << param[0];
		break;

		case SS_FLOAT4:
		out << fparam[3] << ",";
		out << fparam[2] << ",";

		case SS_FLOAT2:
		out << fparam[1] << ",";

		case SS_FLOAT:
		out << fparam[0];
		break;
	}
}

RenderState::RenderState(GLenum parameter, RealtimeExportPassInfo::RenderState s, const GLEnums & GLEnumHelper) : pname(parameter)
{
	if (s.type == "enum")
		type = SS_ENUM;
	else if (s.type == "int")
		type = SS_INT;
	else
	{
		switch (s.floatdata.size())
		{
			case 2:
			type = SS_FLOAT2;
			break;

			case 4:
			type = SS_FLOAT4;
			break;

			default:
			type = SS_FLOAT;
			break;
		}
	}

	for (unsigned int i = 0; i < s.intdata.size() && i < 4; i++)
	{
		param[i] = s.intdata[i];
	}

	if (s.type == "enum")
	{
		param[0] = GLEnumHelper.getEnum(s.enumdata);
	}

	for (unsigned int i = 0; i < s.floatdata.size() && i < 4; i++)
	{
		fparam[i] = s.floatdata[i];
	}
}
