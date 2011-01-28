#ifndef _RENDERTEXTURE
#define _RENDERTEXTURE

#include "rendertextureentry.h"

/// The bare minimum required to bind a texture
struct RenderTexture
{
	GLuint tu;
	GLuint handle;
	GLenum target;
	
	RenderTexture(GLint newTu, const RenderTextureEntry & entry) : tu(newTu), handle(entry.handle), target(entry.target) {}
	RenderTexture(GLenum newtarget, GLuint newhandle) : handle(newhandle), target(newtarget) {}
};

#endif
