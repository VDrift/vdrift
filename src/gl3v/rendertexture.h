#ifndef _RENDERTEXTURE
#define _RENDERTEXTURE

#include "rendertextureentry.h"

/// The bare minimum required to bind a texture
struct RenderTexture : RenderTextureBase
{
	GLuint tu;

	RenderTexture(GLint newTu, const RenderTextureEntry & entry) : RenderTextureBase(entry.handle, entry.target), tu(newTu) {}
	RenderTexture(GLenum newtarget, GLuint newhandle) : RenderTextureBase(newhandle, newtarget) {}
};

#endif
