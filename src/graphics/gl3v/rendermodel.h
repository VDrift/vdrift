/************************************************************************/
/*                                                                      */
/* This file is part of VDrift.                                         */
/*                                                                      */
/* VDrift is free software: you can redistribute it and/or modify       */
/* it under the terms of the GNU General Public License as published by */
/* the Free Software Foundation, either version 3 of the License, or    */
/* (at your option) any later version.                                  */
/*                                                                      */
/* VDrift is distributed in the hope that it will be useful,            */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of       */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the        */
/* GNU General Public License for more details.                         */
/*                                                                      */
/* You should have received a copy of the GNU General Public License    */
/* along with VDrift.  If not, see <http://www.gnu.org/licenses/>.      */
/*                                                                      */
/************************************************************************/

#ifndef _RENDERMODEL
#define _RENDERMODEL

#include "renderuniform.h"
#include "rendertexture.h"
#include "keyed_container.h"
#include "rendermodelentry.h"

#include <unordered_map>

/// The bare minimum required to draw geometry.
struct RenderModel
{
	RenderModel(const RenderModelEntry & entry);

	GLuint vao;
	int elementCount;

	// This contains per-model overrides for texture data but could just as well be empty.
	keyed_container <RenderTexture> textureBindingOverrides;

	// This contains per-model overrides for uniform data but could just as well be empty.
	keyed_container <RenderUniform> uniformOverrides;

	// these are used when updating values and allow us to quickly look up existing overrides.
	typedef std::unordered_map <StringId, keyed_container <RenderUniform>::handle, StringId::hash> UniformMap;
	typedef std::unordered_map <StringId, keyed_container <RenderUniform>::handle, StringId::hash> TextureMap;
	UniformMap variableNameToUniformOverride;
	TextureMap textureNameToTextureOverride;
};

#endif
