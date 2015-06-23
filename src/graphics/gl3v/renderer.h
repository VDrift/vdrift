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

#ifndef _RENDERER
#define _RENDERER

#include "stringidmap.h"
#include "renderpass.h"

#include <vector>
#include <iosfwd>
#include <string>
#include <map>

/// StringIds are used to speed up use of the friendly names for texture samplers, uniform locations, and draw groups.
class Renderer
{
public:
	/// The provided GLWrapper will be used for OpenGL context.
	/// A reference will be kept to the glwrapper, so it must stay valid for the lifetime of this object.
	Renderer(GLWrapper & glwrapper);

	/// Initialize the renderer with the configuration in the provided renderpassinfos.
	/// The passes will be rendered in the order they appear in the vector.
	/// The provided StringIdMap will be used to convert strings into unique numeric IDs.
	/// w and h are the width and height of the application's window and will be used to initialize FBOs.
	bool initialize(const std::vector <RealtimeExportPassInfo> & config, StringIdMap & stringMap, const std::string & shaderPath, unsigned int w, unsigned int h, const std::set <std::string> & globalDefines, std::ostream & errorOutput);

	/// Render all passes.
	/// w and h are the width and height of the application's window.
	/// Models previously added with addModel are drawn.
	void render(unsigned int w, unsigned int h, StringIdMap & stringMap, std::ostream & errorOutput);

	/// Render all passes.
	/// w and h are the width and height of the application's window.
	/// ExternalModels is a map of draw group name ID to a vector array of pointers to external models to be drawn along with models that have been added to the pass with addModel.
	void render(unsigned int w, unsigned int h, StringIdMap & stringMap, const std::map <StringId, std::vector <RenderModelExt*> > & externalModels, std::ostream & errorOutput);

	/// Render all passes.
	/// w and h are the width and height of the application's window.
	/// externalModels is a map of pass ID to a map of draw group name ID and a pointer to a vector array of pointers to external models to be drawn along with models that have been added to the pass with addModel.
	void render(unsigned int w, unsigned int h, StringIdMap & stringMap, const std::map <StringId, std::map <StringId, std::vector <RenderModelExt*> *> > & externalModels, std::ostream & errorOutput);

	/// Cleanup all data.
	void clear();

	/// Pass an externally created piece of geometry to our renderer.
	/// The returned handle can be used to update the values of the model and ask for its removal.
	/// The caller is still responsible for memory management of the OpenGL data and should not delete the data before the model has been removed from the Renderer.
	RenderModelHandle addModel(const RenderModelEntry & entry);

	/// This only removes the model from rendering; the caller is still responsible for the OpenGL data and they must eventually delete it.
	/// After this call, the RenderModelHandle should not be used again.
	void removeModel(RenderModelHandle handle);

	/// Sets a texture on the provided model. If the model already has a texture with the same name StringId, the texture is replaced with the new one. If not, the texture is added.
	/// The caller is still responsible for memory management of the OpenGL data in RenderTextureEntry and should not delete the data before any models using it have been removed from the Renderer.
	void setModelTexture(RenderModelHandle model, const RenderTextureEntry & texture);

	/// This only removes the texture from the model; the caller is still responsible for the OpenGL data and they must eventually delete it.
	void removeModelTexture(RenderModelHandle model, StringId name);

	/// Sets a uniform on the provided model. If the model already has a uniform with the same name StringId, the old uniform data is replaced with the new data. If not, the uniform is added.
	void setModelUniform(RenderModelHandle model, const RenderUniformEntry & uniform);

	/// This only removes the uniform from the model; the caller is still responsible for the OpenGL data and they must eventually delete it.
	void removeModelUniform(RenderModelHandle model, StringId name);

	/// This sets a global texture mapping that can be used by passes to fill in default texture bindings.
	/// The mapping is keyed on the name of the texture. If the name already exists, the existing RenderTextureEntry is overridden with the new one.
	/// The caller is still responsible for memory management of the OpenGL data in RenderTextureEntry and should not delete the data before removeDefaultTexture is called.
	void setGlobalTexture(StringId name, const RenderTextureEntry & texture);

	/// This only removes the texture's entry; the caller is still responsible for the OpenGL data and they must eventually delete it.
	void removeGlobalTexture(StringId name);

	/// This sets a texture mapping that can be used by the specified pass to fill in default texture bindings.
	/// The mapping is keyed on the name of the texture. If the name already exists, the existing RenderTextureEntry is overridden with the new one.
	/// The caller is still responsible for memory management of the OpenGL data in RenderTextureEntry and should not delete the data before removePassTexture is called.
	void setPassTexture(StringId passName, StringId textureName, const RenderTextureEntry & texture);

	/// This only removes the texture's entry; the caller is still responsible for the OpenGL data and they must eventually delete it.
	void removePassTexture(StringId passName, StringId textureName);

	/// This sets a global uniform mapping that can be used by passes to fill in default uniform bindings.
	/// The mapping is keyed on the name of the uniform. If the name already exists, the existing RenderUniformEntry is overridden with the new one.
	/// Returns the number of passes that were affected by the change.
	int setGlobalUniform(const RenderUniformEntry & uniform);

	/// Remove a uniform mapping previously set.
	void removeGlobalUniform(StringId name);

	/// This sets a texture mapping that can be used by the specified pass to fill in default uniform bindings.
	/// The mapping is keyed on the name of the uniform. If the name already exists, the existing RenderUniformEntry is overridden with the new one.
	void setPassUniform(StringId passName, const RenderUniformEntry & uniform);

	/// Remove a uniform mapping previously set.
	void removePassUniform(StringId passName, StringId uniformName);

	/// Get a pass uniform to the provided RenderUniform struct.
	/// Returns true if there is a corresponding uniform for the pass, otherwise returns false.
	bool getPassUniform(StringId passName, StringId uniformName, RenderUniform & out);

	/// Turn a specific pass on or off.
	void setPassEnabled(StringId passName, bool enable);
	bool getPassEnabled(StringId passName) const;

	/// Get user-defined fields for a pass.
	const std::map <std::string, std::string> & getUserDefinedFields(StringId passName) const;

	/// Get the draw groups that are used by the specified pass.
	const std::set <StringId> & getDrawGroups(StringId passName) const;

	/// Get a vector of pass name string ids.
	std::vector <StringId> getPassNames() const;

	/// Print some human readable text showing renderer status information.
	void printRendererStatus(RendererStatusVerbosity verbosity, const StringIdMap & stringMap, std::ostream & out) const;

	/// Print some human readable profiling information.
	void printProfilingInfo(std::ostream & out) const;

private:
	bool loadShader(const std::string & path, const std::string & name, const std::set <std::string> & defines, GLenum shaderType, std::ostream & errorOutput);

	GLWrapper & gl;

	std::vector <RenderPass> passes;

	/// Maps shared texture names to a RenderTexture; this is a copy of the RenderTexture that is bookkept either by the pass (for rendertargets) or externally (for shared textures).
	NameTexMap sharedTextures;

	/// Maps pass names to indexes into the "passes" vector.
	NameIdMap passIndexMap;

	/// Map of draw group string id to a list of pass indices that use the draw group.
	typedef std::unordered_map <StringId, std::vector <unsigned int>, StringId::hash> NameIdVecMap;
	NameIdVecMap drawGroupToPasses;

	/// Internally shared resources: map of shader name to shader handle.
	std::map <std::string, RenderShader> shaders;

	/// Externally created but internally tracked models.
	keyed_container <RenderModelEntry> models;
};

#endif
