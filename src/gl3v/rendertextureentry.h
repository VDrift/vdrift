#ifndef _RENDERTEXTUREENTRY
#define _RENDERTEXTUREENTRY

#include "stringidmap.h"
#include "glwrapper.h"

struct RenderTextureEntry
{
	StringId name;
	GLuint handle;
	GLenum target;
	
	RenderTextureEntry(StringId newname, GLuint newhandle, GLenum newtarget) : name(newname), handle(newhandle), target(newtarget) {}
};

#endif
