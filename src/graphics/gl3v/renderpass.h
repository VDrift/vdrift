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

#ifndef _RENDERPASS
#define _RENDERPASS

#include "renderpassinfo.h"
#include "glwrapper.h"
#include "stringidmap.h"
#include "keyed_container.h"
#include "renderdimensions.h"
#include "rendermodelentry.h"
#include "renderstate.h"
#include "rendersampler.h"
#include "rendertexture.h"
#include "rendermodel.h"
#include "renderuniform.h"
#include "rendershader.h"
#include "rendertextureentry.h"
#include "renderuniformentry.h"
#include "renderstatusverbosity.h"
#include "rendermodelext.h"

#include <unordered_map>
#include <vector>
#include <iosfwd>
#include <string>
#include <map>
#include <set>

typedef std::unordered_map <StringId, RenderTextureEntry, StringId::hash> NameTexMap;
typedef std::unordered_map <StringId, unsigned int, StringId::hash> NameIdMap;

class RenderPass
{
public:
	// Constructor.
	RenderPass();

	/// Initialize the renderer with the configuration in the provided renderpassinfos.
	/// The passes will be rendered in the order they appear in the vector.
	/// The provided GLWrapper will be used for OpenGL context.
	/// The provided StringIdMap will be used to convert strings into unique numeric IDs.
	/// w and h are the width and height of the application's window and will be used to initialize FBOs.
	bool initialize(int passCount, const RealtimeExportPassInfo & config, StringIdMap & stringMap, GLWrapper & gl, RenderShader & vertexShader, RenderShader & fragmentShader, const std::unordered_map <StringId, RenderTextureEntry, StringId::hash> & sharedTextures, unsigned int w, unsigned int h, std::ostream & errorOutput);

	/// Prepare for destruction by cleaning up any resources that we are using.
	void clear(GLWrapper & gl);

	/// Render the pass.
	/// w and h are the width and height of the application's window.
	/// Returns true if the framebuffer dimensions have changed, which is a signal that the render targets have been recreated.
	/// externalModels is a map of draw group name ID to a vector array of pointers to external models to be drawn along with models that have been added to the pass with addModel.
	bool render(GLWrapper & gl, unsigned int w, unsigned int h, StringIdMap & stringMap, const std::vector <const std::vector <RenderModelExt*>*> & externalModels, const std::unordered_map <StringId, RenderTextureEntry, StringId::hash> & sharedTextures, std::ostream & errorOutput);

	// These functions handle modifications to the models container.
	void addModel(const RenderModelEntry & entry, RenderModelHandle handle);
	void removeModel(RenderModelHandle handle);
	void setModelTexture(RenderModelHandle model, const RenderTextureEntry & texture);
	void removeModelTexture(RenderModelHandle model, StringId name);
	void setModelUniform(RenderModelHandle model, const RenderUniformEntry & uniform);
	void removeModelUniform(RenderModelHandle model, StringId name);

	// These handle modifications to our defaultTextureBindings.
	void setDefaultTexture(StringId name, const RenderTextureEntry & texture);
	void removeDefaultTexture(StringId name);
	bool getDefaultUniform(StringId uniformName, RenderUniform & out);

	// These handle modifications to our defaultUniformBindings.
	// Returns true if the pass uses this uniform.
	bool setDefaultUniform(const RenderUniformEntry & uniform);
	void removeDefaultUniform(StringId name);

	const std::string & getName() const;
	StringId getNameId() const;

	const std::map <StringId, RenderTexture> & getRenderTargets() const;

	void setEnabled(bool val);
	bool getEnabled() const;

	void printRendererStatus(RendererStatusVerbosity verbosity, const StringIdMap & stringMap, std::ostream & out) const;

	const std::set <StringId> & getDrawGroups() const;

	const std::map <std::string, std::string> & getUserDefinedFields() const;

	float getLastTime() const;

private:
	/// Returns true on success.
	bool createFramebufferObject(GLWrapper & gl, unsigned int w, unsigned int h, StringIdMap & stringMap, const NameTexMap & sharedTextures, std::ostream & errorOutput);
	void deleteFramebufferObject(GLWrapper & gl);

	/// Returns true on success.
	bool createShaderProgram(GLWrapper & gl, const std::vector <std::string> & shaderAttributeBindings, const RenderShader & vertexShader, const RenderShader & fragmentShader, const std::map <std::string, RealtimeExportPassInfo::RenderTargetInfo> & renderTargets, std::ostream & errorOutput);
	void deleteShaderProgram(GLWrapper & gl);

	/// Switches to the texture's TU and binds the texture.
	void applyTexture(GLWrapper & gl, const RenderTexture & texture);
	/// Switches to the texture's TU and binds the texture.
	void applyTexture(GLWrapper & gl, GLuint tu, GLenum target, GLuint handle);

	bool configured;
	bool enabled;

	/// The original, verbose configuration data.
	RealtimeExportPassInfo originalConfiguration;

	// All of the models we'll be rendering in this pass.
	// We keep two data structures, one for fast iteration during rendering which holds the actual RenderModel data, and another that is used to speed up updates and which simply holds handles to the keyed_container.
	keyed_container <RenderModel> models;
	typedef std::unordered_map <RenderModelHandle, keyed_container <RenderModel>::handle, keyed_container_hash> ModelHandleMap;
	ModelHandleMap modelHandles;

	// Theese fields are used to remember mappings so we can look them up when we get an update.
	/// This is used to remember how variable names correspond to uniform locations.
	NameIdMap variableNameToUniformLocation;
	/// This is used to remember how texture names correspond to texture unit numbers.
	NameIdMap textureNameToTextureUnit;

	/// Which bitfields to clear when we start the pass.
	GLbitfield clearMask;

	// Values to clear to.
	float clearColor[4];
	float clearDepth;
	int clearStencil;

	/// The shader program.
	GLuint shaderProgram;

	// Variables that can be overridden (or not) by specific models.
	std::vector <RenderUniform> defaultUniformBindings;
	// Texture bindings that can be overridden (or not) by specific models.
	// There's no particular indexing; the tu is in the RenderTexture.
	std::vector <RenderTexture> defaultTextureBindings;

	// Render states.
	std::vector <GLenum> stateEnable;
	std::vector <GLenum> stateDisable;
	std::vector <std::pair<GLenum,unsigned int> > stateEnablei;
	std::vector <std::pair<GLenum,unsigned int> > stateDisablei;
	std::vector <RenderState> stateEnum;

	// Render target information.
	GLuint framebufferObject, renderbuffer;
	RenderDimensions framebufferDimensions;
	std::vector <RenderTexture> autoMipMapRenderTargets; // This is a subset of the textures below.
	std::map <StringId, RenderTexture> renderTargets; // The key is the render target name loaded from the RealtimeExportPassInfo.
	std::map <StringId, RenderTexture> externalRenderTargets; // The key is the render target name loaded from the RealtimeExportPassInfo.

	/// Samplers.
	std::vector <RenderSampler> samplers;

	/// Draw groups.
	std::set <StringId> drawGroups;

	/// Our index in the renderer's list of passes.
	unsigned int passIndex;

	/// Our stringId-ified name.
	StringId passName;

	/// Timing query object.
	GLuint timerQuery;
	/// Timing query object.
	float lastTime;
};

#endif
