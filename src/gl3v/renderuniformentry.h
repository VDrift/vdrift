#ifndef _RENDERUNIFORMENTRY
#define _RENDERUNIFORMENTRY

#include "renderuniformvector.h"

#include "stringidmap.h"

struct RenderUniformEntry
{
	StringId name;
	RenderUniformVector <float> data;
	
	RenderUniformEntry() {}
	RenderUniformEntry(StringId newName, float * newData, int dataSize) : name(newName), data(newData, dataSize) {}
};

#endif
