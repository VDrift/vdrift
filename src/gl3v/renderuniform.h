#ifndef _RENDERUNIFORM
#define _RENDERUNIFORM

#include "renderuniformentry.h"
#include "renderuniformvector.h"

/// The bare minimum required to update uniforms
struct RenderUniform : public RenderUniformBase
{
	GLuint location;
	
	RenderUniform() {}
	RenderUniform(GLint loc, const RenderUniformEntry & entry) : RenderUniformBase(entry.data), location(loc) {}
	RenderUniform(GLint loc, const std::vector <float> & newdata) : RenderUniformBase(newdata), location(loc) {}
};

#endif
