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
