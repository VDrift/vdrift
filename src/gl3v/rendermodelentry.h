#ifndef _RENDERMODELHANDLE
#define _RENDERMODELHANDLE

#include "keyed_container.h"
#include "stringidmap.h"

struct RenderModelEntry
{
	StringId group;
	GLuint vao;
	int elementCount;
};

typedef keyed_container<RenderModelEntry>::handle RenderModelHandle;

#endif