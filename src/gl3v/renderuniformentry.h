#ifndef _RENDERUNIFORMENTRY
#define _RENDERUNIFORMENTRY

#include <vector>

#include "stringidmap.h"

struct RenderUniformEntry
{
	StringId name;
	std::vector <float> data;
	
	RenderUniformEntry(StringId newName, float * dataPtr, size_t dataSize) : name(newName), data(dataPtr, dataPtr+dataSize) {}
};

#endif
