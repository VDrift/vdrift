#ifndef _RENDERUNIFORMENTRY
#define _RENDERUNIFORMENTRY

#include "renderuniformvector.h"

#include "stringidmap.h"

struct RenderUniformBase
{
	RenderUniformVector <float> data;

	RenderUniformBase() {}
	RenderUniformBase(const float * newData, int dataSize) : data(newData, dataSize) {}
	RenderUniformBase(const std::vector <float> & newdata) : data(newdata) {}
	RenderUniformBase(const RenderUniformVector <float> & newdata) : data(newdata) {}
};

struct RenderUniformEntry : public RenderUniformBase
{
	StringId name;

	RenderUniformEntry() {}
	RenderUniformEntry(StringId newName, const float * newData, int dataSize) : RenderUniformBase(newData, dataSize), name(newName) {}
};

#endif
