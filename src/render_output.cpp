#include "render_output.h"
#include "glstatemanager.h"

RENDER_OUTPUT::RENDER_OUTPUT() :
	target(RENDER_TO_FRAMEBUFFER)
{
	// ctor
}

RENDER_OUTPUT::~RENDER_OUTPUT()
{
	// ctor
}

FBOBJECT & RENDER_OUTPUT::RenderToFBO()
{
	target = RENDER_TO_FBO;
	return fbo;
}

void RENDER_OUTPUT::RenderToFramebuffer()
{
	target = RENDER_TO_FRAMEBUFFER;
}

bool RENDER_OUTPUT::IsFBO() const
{
	return target == RENDER_TO_FBO;
}

void RENDER_OUTPUT::Begin(GLSTATEMANAGER & glstate, std::ostream & error_output)
{
	if (target == RENDER_TO_FBO)
		fbo.Begin(glstate, error_output);
	else if (target == RENDER_TO_FRAMEBUFFER)
		glstate.BindFramebuffer(0);
}

void RENDER_OUTPUT::End(GLSTATEMANAGER & glstate, std::ostream & error_output)
{
	if (target == RENDER_TO_FBO)
		fbo.End(glstate, error_output);
}
