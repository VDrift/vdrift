#ifndef _RENDERMODEL
#define _RENDERMODEL

#include "renderuniform.h"
#include "rendertexture.h"
#include "keyed_container.h"
#include "rendermodelentry.h"
#include "unordered_map.h"

/// The bare minimum required to draw geometry
struct RenderModel
{
	GLuint vao;
	int elementCount;
	
	// This contains per-model overrides for texture data but could just as well be empty
	keyed_container <RenderTexture> textureBindingOverrides;
	
	// This contains per-model overrides for uniform data but could just as well be empty
	keyed_container <RenderUniform> uniformOverrides;
	
	// these are used when updating values and allow us to quickly look up existing overrides
	std::tr1::unordered_map <StringId, keyed_container <RenderUniform>::handle, StringId::hash> variableNameToUniformOverride;
	std::tr1::unordered_map <StringId, keyed_container <RenderTexture>::handle, StringId::hash> textureNameToTextureOverride;
	
	RenderModel(const RenderModelEntry & entry) : vao(entry.vao), elementCount(entry.elementCount) {}
};

#endif
