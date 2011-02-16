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

#include <vector>
#include <tr1/unordered_map>
#include <iostream>
#include <map>
#include <set>

class RenderPass
{
	public:
		/// Initialize the renderer with the configuration in the provided renderpassinfos.
		/// The passes will be rendered in the order they appear in the vector.
		/// The provided GLWrapper will be used for OpenGL context.
		/// The provided StringIdMap will be used to convert strings into unique numeric IDs.
		/// w and h are the width and height of the application's window and will be used to initialize FBOs.
		bool initialize(int passCount,
			const RealtimeExportPassInfo & config, 
			StringIdMap & stringMap,
			GLWrapper & gl,
			RenderShader & vertexShader,
			RenderShader & fragmentShader,
			const std::tr1::unordered_map <StringId, RenderTextureEntry, StringId::hash> & sharedTextures,
			unsigned int w, unsigned int h,
			std::ostream & errorOutput);
		
		/// prepare for destruction by cleaning up any resources that we are using
		void clear(GLWrapper & gl);
		
		/// render the pass
		/// w and h are the width and height of the application's window
		/// returns true if the framebuffer dimensions have changed, which is a signal that the render targets have been recreated
		/// externalModels is a map of draw group name ID to a vector array of pointers to external models to be drawn along with models that have been added to the pass with addModel
		bool render(GLWrapper & gl, unsigned int w, unsigned int h, StringIdMap & stringMap, 
					const std::vector <const std::vector <RenderModelExternal*>*> & externalModels, 
					std::ostream & errorOutput);
		
		// these functions handle modifications to the models container
		void addModel(const RenderModelEntry & entry, RenderModelHandle handle);
		void removeModel(RenderModelHandle handle);
		void setModelTexture(RenderModelHandle model, const RenderTextureEntry & texture);
		void removeModelTexture(RenderModelHandle model, StringId name);
		void setModelUniform(RenderModelHandle model, const RenderUniformEntry & uniform);
		void removeModelUniform(RenderModelHandle model, StringId name);
		
		// these handle modifications to our defaultTextureBindings
		void setDefaultTexture(StringId name, const RenderTextureEntry & texture);
		void removeDefaultTexture(StringId name);
		bool getDefaultUniform(StringId uniformName, RenderUniform & out);
		
		// these handle modifications to our defaultUniformBindings
		void setDefaultUniform(const RenderUniformEntry & uniform);
		void removeDefaultUniform(StringId name);
		
		const std::string & getName() const {return originalConfiguration.name;}
		StringId getNameId() const {return passName;}
		
		const std::map <StringId, RenderTexture> & getRenderTargets() const {return renderTargets;}
		
		void setEnabled(bool val) {enabled = val;}
		bool getEnabled() const {return enabled;}
		
		void printRendererStatus(RendererStatusVerbosity verbosity, const StringIdMap & stringMap, std::ostream & out) const;
		
		const std::set <StringId> & getDrawGroups() const {return drawGroups;}
		
		const std::map <std::string, std::string> & getUserDefinedFields() const {return originalConfiguration.userDefinedFields;}
		
		RenderPass() : configured(false),enabled(true),shaderProgram(0),framebufferObject(0),renderbuffer(0),passIndex(0) {}
		
	private:
		/// returns true on success
		bool createFramebufferObject(GLWrapper & gl, unsigned int w, unsigned int h, StringIdMap & stringMap, std::ostream & errorOutput);
		void deleteFramebufferObject(GLWrapper & gl);
		
		/// returns true on success
		bool createShaderProgram(GLWrapper & gl, const std::vector <std::string> & shaderAttributeBindings, const RenderShader & vertexShader, const RenderShader & fragmentShader, std::ostream & errorOutput);
		void deleteShaderProgram(GLWrapper & gl);
		
		/// switches to the texture's TU and binds the texture
		void applyTexture(GLWrapper & gl, const RenderTexture & texture) {applyTexture(gl, texture.tu, texture.target, texture.handle);}
		void applyTexture(GLWrapper & gl, GLuint tu, GLenum target, GLuint handle);
		
		bool configured;
		bool enabled;
		
		// the original, verbose configuration data
		RealtimeExportPassInfo originalConfiguration;
		
		// all of the models we'll be rendering in this pass
		// we keep two data structures, one for fast iteration during rendering
		// which holds the actual RenderModel data, and another that is used to 
		// speed up updates and which simply holds handles to the keyed_container
		keyed_container <RenderModel> models;
		typedef std::tr1::unordered_map <RenderModelHandle, keyed_container <RenderModel>::handle, keyed_container_hash> modelHandleMap;
		modelHandleMap modelHandles;
		
		// these fields are used to remember mappings so we can look them up when we get an update
		// this is used to remember how variable names correspond to uniform locations
		std::tr1::unordered_map <StringId, GLuint, StringId::hash> variableNameToUniformLocation;
		// this is used to remember how texture names correspond to texture unit numbers
		std::tr1::unordered_map <StringId, GLuint, StringId::hash> textureNameToTextureUnit;
		
		// which bitfields to clear when we start the pass
		GLbitfield clearMask;
		
		// the shader program
		GLuint shaderProgram;
		
		// variables that can be overridden (or not) by specific models
		std::vector <RenderUniform> defaultUniformBindings;
		// texture bindings that can be overridden (or not) by specific models
		// there's no particular indexing; the tu is in the RenderTexture
		std::vector <RenderTexture> defaultTextureBindings;
		
		// render states
		std::vector <GLenum> stateEnable;
		std::vector <GLenum> stateDisable;
		std::vector <std::pair<GLenum,unsigned int> > stateEnablei;
		std::vector <std::pair<GLenum,unsigned int> > stateDisablei;
		std::vector <RenderState> stateEnum;
		
		// render target information
		GLuint framebufferObject, renderbuffer;
		RenderDimensions framebufferDimensions;
		std::vector <RenderTexture> autoMipMapRenderTargets; // this is a subset of the textures below
		std::map <StringId, RenderTexture> renderTargets; // the key is the render target name loaded from the RealtimeExportPassInfo
		
		// samplers
		std::vector <RenderSampler> samplers;
		
		// draw groups
		std::set <StringId> drawGroups;
		
		// our index in the renderer's list of passes
		unsigned int passIndex;
		
		// our stringId-ified name
		StringId passName;
};

#endif
