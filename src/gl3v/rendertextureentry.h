#ifndef _RENDERTEXTUREENTRY
#define _RENDERTEXTUREENTRY

#include "stringidmap.h"
#include "glwrapper.h"

struct RenderTextureBase
{
	GLuint handle;
	GLenum target;
	
	RenderTextureBase() {}
	RenderTextureBase(GLuint newhandle, GLenum newtarget) : handle(newhandle), target(newtarget) {}
};

struct RenderTextureEntry : public RenderTextureBase
{
	StringId name;
	
	RenderTextureEntry() {}
	RenderTextureEntry(StringId newname, GLuint newhandle, GLenum newtarget) : RenderTextureBase(newhandle,newtarget), name(newname) {}
};

#endif
