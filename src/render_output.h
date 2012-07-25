#ifndef _RENDER_OUTPUT_H
#define _RENDER_OUTPUT_H

#include "fbobject.h"
#include <ostream>

class GLSTATEMANAGER;

class RENDER_OUTPUT
{
public:
	RENDER_OUTPUT();

	~RENDER_OUTPUT();

	/// returns the FBO that the user should set up as necessary
	FBOBJECT & RenderToFBO();

	void RenderToFramebuffer();

	bool IsFBO() const;

	void Begin(GLSTATEMANAGER & glstate, std::ostream & error_output);

	void End(GLSTATEMANAGER & glstate, std::ostream & error_output);

private:
	FBOBJECT fbo;
	enum
	{
		RENDER_TO_FBO,
		RENDER_TO_FRAMEBUFFER
	} target;
};

#endif // _RENDER_OUTPUT_H
