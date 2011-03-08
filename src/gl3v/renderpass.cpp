#include "renderpass.h"

#include "glenums.h"

#include <cassert>

#ifdef USE_TR1
#include <tr1/unordered_set>
#endif
//#define USE_EXTERNAL_MODEL_CACHE

const GLEnums GLEnumHelper;

bool RenderPass::render(GLWrapper & gl, unsigned int w, unsigned int h, StringIdMap & stringMap, 
						const std::vector <const std::vector <RenderModelExternal*>*> & externalModels,
						const std::tr1::unordered_map <StringId, RenderTextureEntry, StringId::hash> & sharedTextures,
						std::ostream & errorOutput)
{
	if (!enabled)
		return false;
	
	// ensure we've been successfully configured
	assert(configured);
	
	// determine the size of the framebuffer
	unsigned int width(0), height(0);
	bool changed = framebufferDimensions.get(w, h, width, height);
	
	// if the framebuffer size has changed, we need to recreate it
	if (changed)
	{
		if (!createFramebufferObject(gl, w, h, stringMap, sharedTextures, errorOutput))
		{
			errorOutput << "Unable to create framebuffer object" << std::endl;
			return changed;
		}
	}
	
	// bind framebuffer
	gl.BindFramebuffer(framebufferObject);
	
	// set viewport
	gl.Viewport(width, height);
	
	// bind shader program
	gl.UseProgram(shaderProgram);
	
	// apply state (enable/disable/enablei/disablei/enum)
	for (std::vector <GLenum>::const_iterator s = stateEnable.begin(); s != stateEnable.end(); s++)
	{
		gl.Enable(*s);
	}
	for (std::vector <GLenum>::const_iterator s = stateDisable.begin(); s != stateDisable.end(); s++)
	{
		gl.Disable(*s);
	}
	for (std::vector <std::pair <GLenum,unsigned int> >::const_iterator s = stateEnablei.begin(); s != stateEnablei.end(); s++)
	{
		gl.Enablei(s->first, s->second);
	}
	for (std::vector <std::pair <GLenum,unsigned int> >::const_iterator s = stateDisablei.begin(); s != stateDisablei.end(); s++)
	{
		gl.Disablei(s->first, s->second);
	}
	for (std::vector <RenderState>::const_iterator s = stateEnum.begin(); s != stateEnum.end(); s++)
	{
		s->apply(gl);
	}
	
	// clear
	// we do this here so our write masks will have already been applied
	gl.Clear(clearMask);

	// apply default uniforms
	std::vector <const RenderUniform*> defaultUniforms; // indexed by location, either NULL or a pointer to the RenderUniform bound to the location
	for (std::vector <RenderUniform>::const_iterator u = defaultUniformBindings.begin(); u != defaultUniformBindings.end(); u++)
	{
		defaultUniforms.resize(std::max(defaultUniforms.size(),(size_t)(u->location+1)),NULL);
		defaultUniforms[u->location] = &*u;
		gl.applyUniform(u->location, u->data);
	}
	
	// apply samplers
	for (GLuint tu = 0; tu < samplers.size(); tu++)
	{
		samplers[tu].apply(gl);
	}
	
	// apply default textures, keeping track of which textures are in which TUs
	std::vector <const RenderTexture*> defaultTextures; // indexed by TU, either NULL or a pointer to the RenderTexture bound to the TU
	for (std::vector <RenderTexture>::const_iterator t = defaultTextureBindings.begin(); t != defaultTextureBindings.end(); t++)
	{
		defaultTextures.resize(std::max(defaultTextures.size(),(size_t)(t->tu+1)),NULL);
		defaultTextures[t->tu] = &*t;
		applyTexture(gl, *t);
	}
	
	typedef std::vector <GLuint> override_tracking_type;
	std::vector <const RenderTextureBase*> textureState(defaultTextures.begin(), defaultTextures.end()); //indexed by tu
	textureState.resize(std::max(textureState.size(),samplers.size()),NULL);
	std::vector <const RenderUniformBase*> uniformState(defaultUniforms.begin(), defaultUniforms.end()); //indexed by location

	override_tracking_type overriddenTextures;
	override_tracking_type lastOverriddenTextures;
	override_tracking_type overriddenUniforms;
	override_tracking_type lastOverriddenUniforms;
	
	// for each model
	for (keyed_container <RenderModel>::const_iterator m = models.begin(); m != models.end(); m++)
	{
		// apply texture overrides, keeping track of which TUs we've overridden
		overriddenTextures.clear();
		for (std::vector <RenderTexture>::const_iterator t = m->textureBindingOverrides.begin(); t != m->textureBindingOverrides.end(); t++)
		{
			overriddenTextures.push_back(t->tu);
			
			applyTexture(gl, *t);
		}
		
		// apply uniform overrides, keeping track of which locations we've overridden
		overriddenUniforms.clear();
		for (std::vector <RenderUniform>::const_iterator u = m->uniformOverrides.begin(); u != m->uniformOverrides.end(); u++)
		{
			overriddenUniforms.push_back(u->location);
			
			gl.applyUniform(u->location, u->data);
		}
		
		// draw geometry
		gl.drawGeometry(m->vao, m->elementCount);
		
		// restore overridden uniforms
		for (override_tracking_type::const_iterator location = overriddenUniforms.begin(); location != overriddenUniforms.end(); location++)
		{
			if (*location < defaultUniforms.size())
			{
				const RenderUniform * u = defaultUniforms[*location];
				if (u)
					gl.applyUniform(u->location, u->data);
			}
		}
		
		// restore overridden textures
		for (override_tracking_type::const_iterator tu = overriddenTextures.begin(); tu != overriddenTextures.end(); tu++)
		{
			if (*tu < defaultTextures.size()) // sometimes we override sampler TUs that don't have defaults defined (think of diffuse textures)
			{
				const RenderTexture * t = defaultTextures[*tu];
				if (t)
					applyTexture(gl, *t);
			}
		}
	}
	
	// for each external model
	for (std::vector <const std::vector <RenderModelExternal*>*>::const_iterator i = externalModels.begin(); i != externalModels.end(); i++)
	{
		//std::cout << "renderpass: " << stringMap.getString(i->first) << ": " << i->second.size() << std::endl;
		
		// loop through all models in the draw group
		for (std::vector <RenderModelExternal*>::const_iterator n = (*i)->begin(); n != (*i)->end(); n++)
		{
			RenderModelExternal * m = *n;
			assert(m);
			
			if (m->drawEnabled())
			{
				// restore textures that were overridden the by the previous model
				for (override_tracking_type::const_iterator tu = lastOverriddenTextures.begin(); tu != lastOverriddenTextures.end(); tu++)
				{
					if (*tu < defaultTextures.size()) // sometimes we override sampler TUs that don't have defaults defined (think of diffuse textures)
						textureState[*tu] = defaultTextures[*tu];
					else
						textureState[*tu] = NULL;
				}
				
				// apply texture overrides, keeping track of which TUs we've overridden
				overriddenTextures.clear();
				
				// check if we have cached information and if so use that
#ifdef USE_EXTERNAL_MODEL_CACHE
				if (m->perPassTextureCache.size() > passIndex)
				{
					const std::vector <RenderTexture> & cache = m->perPassTextureCache[passIndex];
					for (std::vector <RenderTexture>::const_iterator t = cache.begin(); t != cache.end(); t++)
					{
						// get the TU associated with this texture name id
						GLuint tu = t->tu;
						overriddenTextures.push_back(tu);
						textureState[tu] = &*t;
					}
				}
				else
#endif
				{
					for (std::vector <RenderTextureEntry>::const_iterator t = m->textures.begin(); t != m->textures.end(); t++)
					{
						// get the TU associated with this texture name id
						std::tr1::unordered_map <StringId, GLuint>::iterator tui = textureNameToTextureUnit.find(t->name);
						if (tui != textureNameToTextureUnit.end()) // if the texture isn't used in this pass, it might not be in textureNameToTextureUnit
						{
							GLuint tu = tui->second;
							overriddenTextures.push_back(tu);
							textureState[tu] = &*t;
#ifdef USE_EXTERNAL_MODEL_CACHE
							m->perPassTextureCache[passIndex].push_back(RenderTexture(tu, *t)); // make cache entry
#endif
						}
					}
				}
				
				// go through and actually apply the textures to the GL
				for (override_tracking_type::const_iterator tu = lastOverriddenTextures.begin(); tu != lastOverriddenTextures.end(); tu++)
				{
					const RenderTextureBase * texture = textureState[*tu];
					if (texture)
						applyTexture(gl, *tu, texture->target, texture->handle);
				}
				for (override_tracking_type::const_iterator tu = overriddenTextures.begin(); tu != overriddenTextures.end(); tu++)
				{
					const RenderTextureBase * texture = textureState[*tu];
					
					// we shouldn't need to null-check texture
					applyTexture(gl, *tu, texture->target, texture->handle);
				}
				
				lastOverriddenTextures.swap(overriddenTextures);
				
				// restore uniforms that were overridden the by the previous model
				for (override_tracking_type::const_iterator location = lastOverriddenUniforms.begin(); location != lastOverriddenUniforms.end(); location++)
				{
					if (*location < defaultUniforms.size())
					{
						const RenderUniform * u = defaultUniforms[*location];
						uniformState[u->location] = u;
					}
				}
				
				// apply uniform overrides, keeping track of which locations we've overridden
				overriddenUniforms.clear();
				
				// check if we have cached information and if so use that
#ifdef USE_EXTERNAL_MODEL_CACHE
				if (m->perPassUniformCache.size() > passIndex)
				{
					const std::vector <RenderUniform> & cache = m->perPassUniformCache[passIndex];
					for (std::vector <RenderUniform>::const_iterator u = cache.begin(); u != cache.end(); u++)
					{
						GLuint location = u->location;
						overriddenUniforms.push_back(location);
						uniformState[location] = &*u;
					}
				}
				else
#endif
				{
					for (std::vector <RenderUniformEntry>::const_iterator u = m->uniforms.begin(); u != m->uniforms.end(); u++)
					{
						std::tr1::unordered_map <StringId, GLuint>::iterator loci = variableNameToUniformLocation.find(u->name);
						if (loci != variableNameToUniformLocation.end()) // if the texture isn't used in this pass, it might not be in variableNameToUniformLocation
						{
							GLuint location = loci->second;
							overriddenUniforms.push_back(location);
							uniformState[location] = &*u;
#ifdef USE_EXTERNAL_MODEL_CACHE
							m->perPassUniformCache[passIndex].push_back(RenderUniform(location, *u)); // make cache entry
#endif
						}
					}
				}
				
				// go through and actually apply the uniforms to the GL
				for (override_tracking_type::const_iterator location = lastOverriddenUniforms.begin(); location != lastOverriddenUniforms.end(); location++)
				{
					const RenderUniformBase * uniform = uniformState[*location];
					if (uniform)
						gl.applyUniform(*location, uniform->data);
				}
				for (override_tracking_type::const_iterator location = overriddenUniforms.begin(); location != overriddenUniforms.end(); location++)
				{
					const RenderUniformBase * uniform = uniformState[*location];
					//if (uniform)
						gl.applyUniform(*location, uniform->data);
				}
				
				lastOverriddenUniforms.swap(overriddenUniforms);
				
				// draw geometry
				m->draw(gl);
			}
		}
	}
	
	// unbind framebuffer
	gl.unbindFramebuffer();
	
	// TODO: we only want to do this if the next pass is going to use these and not write to these
	// if autoMipmap then build mipmaps
	for (std::vector <RenderTexture>::const_iterator t = autoMipMapRenderTargets.begin(); t != autoMipMapRenderTargets.end(); t++)
	{
		gl.generateMipmaps(t->target, t->handle);
	}
	
	// unbind samplers
	for (unsigned int tu = 0; tu < samplers.size(); tu++)
	{
		gl.unbindSampler(tu);
	}
	
	return changed;
}

bool RenderPass::initialize(int passCount,
	const RealtimeExportPassInfo & config,
	StringIdMap & stringMap,
	GLWrapper & gl,
	RenderShader & vertexShader,
	RenderShader & fragmentShader,
	const std::tr1::unordered_map <StringId, RenderTextureEntry, StringId::hash> & sharedTextures,
	unsigned int w, unsigned int h,
	std::ostream & errorOutput)
{
	originalConfiguration = config;
	
	passIndex = passCount;
	
	passName = stringMap.addStringId(config.name);
	
	// which bitfields to clear when we start the pass
	clearMask = 0;
	clearMask |= config.clearColor ? GL_COLOR_BUFFER_BIT : 0;
	clearMask |= config.clearDepth ? GL_DEPTH_BUFFER_BIT : 0;
	clearMask |= config.clearStencil ? GL_STENCIL_BUFFER_BIT : 0;
	
	// remember which draw groups we're interested in
	for (std::vector <std::string>::const_iterator i = config.drawGroups.begin(); i != config.drawGroups.end(); i++)
	{
		drawGroups.insert(stringMap.addStringId(*i));
	}
	
	// the shader program
	if (!createShaderProgram(gl, config.shaderAttributeBindings, vertexShader, fragmentShader, errorOutput))
	{
		errorOutput << "Unable to create shader program" << std::endl;
		return false;
	}
	
	// uniforms
	for (std::map <std::string, RealtimeExportPassInfo::UniformData>::const_iterator i = config.uniforms.begin(); i != config.uniforms.end(); i++)
	{
		// attempt to find a location for the uniform
		std::string uniformName = i->first;
		GLint uniformLocation = gl.GetUniformLocation(shaderProgram, uniformName);
		if (uniformLocation != -1)
		{
			variableNameToUniformLocation[stringMap.addStringId(uniformName)] = uniformLocation;
			
			// if the uniform data in the renderpassinfo isn't empty, then that means it has default data
			// and we need to add it to our defaultUniformBindings
			const std::vector <float> & uniformData = i->second.data;
			if (!uniformData.empty())
			{
				defaultUniformBindings.push_back(RenderUniform(uniformLocation,uniformData));
			}
		}
		else
		{
			// this is not an error because the uniform could have been optimized out
		}
	}
	
	// render states
	for (unsigned int i = 0; i < config.stateEnable.size(); i++)
	{
		stateEnable.push_back(GLEnumHelper.getEnum(config.stateEnable[i]));
	}
	for (unsigned int i = 0; i < config.stateDisable.size(); i++)
	{
		stateDisable.push_back(GLEnumHelper.getEnum(config.stateDisable[i]));
	}
	for (unsigned int i = 0; i < config.stateEnablei.size(); i++)
	{
		stateEnablei.push_back(std::make_pair(GLEnumHelper.getEnum(config.stateEnablei[i].first),config.stateEnablei[i].second));
	}
	for (unsigned int i = 0; i < config.stateDisablei.size(); i++)
	{
		stateDisablei.push_back(std::make_pair(GLEnumHelper.getEnum(config.stateDisablei[i].first),config.stateDisablei[i].second));
	}
	for (std::map <std::string, RealtimeExportPassInfo::RenderState>::const_iterator i = config.stateEnum.begin(); i != config.stateEnum.end(); i++)
	{
		stateEnum.push_back(RenderState(GLEnumHelper.getEnum(i->first), i->second, GLEnumHelper));
	}
	
	// we must get the uniform location for the sampler name, then upload a uniform corresponding to the TU we want to use
	for (std::map <std::string, RealtimeExportPassInfo::Sampler>::const_iterator i = config.samplers.begin(); i != config.samplers.end(); i++)
	{
		std::string samplerName = i->first;
		std::string textureName = i->second.textureName;
		GLuint tu = samplers.size();
		
		GLint maxTUs;
		gl.GetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &maxTUs);
		if (tu > (unsigned int) maxTUs)
		{
			errorOutput << "Maximum supported texture unit count exceeded: " << maxTUs << std::endl;
			return false;
		}
		
		// remember sampler texture name binding to the TU
		textureNameToTextureUnit[stringMap.addStringId(textureName)] = tu;
		
		// save the sampler state
		RenderSampler sampler(tu, gl.GenSampler());
		for (std::map <std::string, RealtimeExportPassInfo::RenderState>::const_iterator s = i->second.state.begin(); s != i->second.state.end(); s++)
		{
			sampler.addState(RenderState(GLEnumHelper.getEnum(s->first), s->second, GLEnumHelper));
		}
		samplers.push_back(sampler);
		
		// find the sampler's uniform location, then upload the TU
		GLint samplerLocation = gl.GetUniformLocation(shaderProgram, samplerName);
		if (samplerLocation != -1)
		{
			gl.UseProgram(shaderProgram);
			std::vector <int> tuvec;
			tuvec.push_back(tu);
			gl.applyUniform(samplerLocation, tuvec);
			//std::cout << "applying program " << shaderProgram << " sampler " << samplerName << " handle " << samplerLocation  << ", TU " << tu << std::endl;
		}
		else
		{
			// this is not an error; samplers can be optimized out
		}
		
		// fill default textures from passed-in shared textures
		// texture bindings that can be overridden (or not) by specific models
		std::tr1::unordered_map <StringId, RenderTextureEntry>::const_iterator defaultTexIter = sharedTextures.find(stringMap.addStringId(textureName));
		if (defaultTexIter != sharedTextures.end())
		{
			defaultTextureBindings.push_back(RenderTexture(tu, defaultTexIter->second));
		}
	}
	
	// by default, assume we're rendering to the framebuffer at the window resolution
	framebufferDimensions = RenderDimensions(1,1,true);
	
	// look at the depth attachment or, failing that, the first color framebuffer attachment to find the render viewport
	std::map <std::string, RealtimeExportPassInfo::RenderTargetInfo>::const_iterator attachIter = config.renderTargets.find("GL_DEPTH_ATTACHMENT");
	if (attachIter != config.renderTargets.end())
	{
		const RealtimeExportPassInfo::RenderTargetInfo & targetInfo = attachIter->second;
		framebufferDimensions = RenderDimensions(targetInfo.width, targetInfo.height, targetInfo.widthHeightAreMultiples);
	}
	else
	{
		attachIter = config.renderTargets.begin();
		if (attachIter != config.renderTargets.end())
		{
			const RealtimeExportPassInfo::RenderTargetInfo & targetInfo = attachIter->second;
			framebufferDimensions = RenderDimensions(targetInfo.width, targetInfo.height, targetInfo.widthHeightAreMultiples);
		}
	}
	
	// attempt to create the FBO
	if (!createFramebufferObject(gl, w, h, stringMap, sharedTextures, errorOutput))
	{
		errorOutput << "Unable to create framebuffer object" << std::endl;
		return false;
	}
	
	configured = true;
	
	return true;
}

void RenderPass::clear(GLWrapper & gl)
{
	// delete the FBO, renderbuffers, and render targets
	deleteFramebufferObject(gl);
	
	// delete the linked shader program (but not the individually compiler shaders, which are owned by the renderer)
	deleteShaderProgram(gl);
	
	// clear out external models
	models.clear();
	modelHandles.clear();
	
	// clear out the various mappings
	textureNameToTextureUnit.clear();
	variableNameToUniformLocation.clear();
	
	// delete sampler objects
	for (GLuint tu = 0; tu < samplers.size(); tu++)
	{
		gl.DeleteSampler(samplers[tu].getHandle());
	}
	samplers.clear();
	
	// default bindings
	defaultUniformBindings.clear();
	defaultTextureBindings.clear(); // any GL state that we own in defaultTextureBindings were deleted when we deleted our render targets
	
	// render states
	stateEnable.clear();
	stateDisable.clear();
	stateEnablei.clear();
	stateDisablei.clear();
	stateEnum.clear();
	
	drawGroups.clear();
	
	// reset our configuration status
	configured = false;
}

void RenderPass::deleteFramebufferObject(GLWrapper & gl)
{
	if (framebufferObject != 0)
		gl.deleteFramebufferObject(framebufferObject);
	framebufferObject = 0;
	
	if (renderbuffer != 0)
		gl.deleteRenderbuffer(renderbuffer);
	renderbuffer = 0;

	autoMipMapRenderTargets.clear();
	
	// delete render target textures
	for (std::map <StringId, RenderTexture>::iterator i = renderTargets.begin(); i != renderTargets.end(); i++)
	{
		gl.DeleteTexture(i->second.handle);
	}
	renderTargets.clear();
	
	externalRenderTargets.clear();
}

bool RenderPass::createFramebufferObject(GLWrapper & gl, unsigned int w, unsigned int h, StringIdMap & stringMap, const std::tr1::unordered_map <StringId, RenderTextureEntry, StringId::hash> & sharedTextures, std::ostream & errorOutput)
{
	deleteFramebufferObject(gl);
	
	const RealtimeExportPassInfo & config = originalConfiguration;
	
	// determine dimensions
	unsigned int width(0), height(0);
	framebufferDimensions.get(w, h, width, height);
	
	// find how many draw buffers and attachments are supported
	GLint maxDrawBuffers, maxAttachments;
	gl.GetIntegerv(GL_MAX_DRAW_BUFFERS, &maxDrawBuffers);
	gl.GetIntegerv(GL_MAX_COLOR_ATTACHMENTS, &maxAttachments);
	
	// count attachments
	int drawAttachments = 0; // non-depth attachments
	int depthAttachments = 0; // depth attachments
	for (std::map <std::string, RealtimeExportPassInfo::RenderTargetInfo>::const_iterator i = config.renderTargets.begin(); i != config.renderTargets.end(); i++)
	{
		if (i->first == "GL_DEPTH_ATTACHMENT")
			depthAttachments++;
		else if (i->first.substr(0,19) == "GL_COLOR_ATTACHMENT")
			drawAttachments++;
	}
	
	if (depthAttachments > 1)
	{
		errorOutput << "Multiple depth attachments are not supported: " << depthAttachments << std::endl;
		return false;
	}
	
	if (drawAttachments > maxDrawBuffers || drawAttachments > maxAttachments)
	{
		errorOutput << "Pass has " << drawAttachments << " draw buffers, but only " << maxDrawBuffers << " are supported" << std::endl;
		return false;
	}
	
	// if we have no attachments, we use the default framebuffer object
	if (config.renderTargets.empty())
	{
		framebufferObject = 0;
		return true;
	}
	else if (drawAttachments + depthAttachments == 0)
	{
		errorOutput << "Pass has no color or depth attachments, but render targets were specified in the RenderPassInfo." << std::endl;
		return false;
	}
	
	// bind render target variable names to frag data locations, then relink the shader program
	for (std::map <std::string, RealtimeExportPassInfo::RenderTargetInfo>::const_iterator i = config.renderTargets.begin(); i != config.renderTargets.end(); i++)
	{
		// we only bind names for color attachments
		if (i->first.substr(0,19) == "GL_COLOR_ATTACHMENT")
		{
			// find the render target attachment point
			GLenum attachmentPoint = GLEnumHelper.getEnum(i->first);
		
			// find the color attachment location
			int colorNumber = attachmentPoint - GL_COLOR_ATTACHMENT0;
		
			gl.BindFragDataLocation(shaderProgram, colorNumber, i->second.variable.c_str());
		}
	}
	if (!gl.relinkShaderProgram(shaderProgram, errorOutput))
		return false;
	
	// generate a framebuffer object
	framebufferObject = gl.GenFramebuffer();
	gl.BindFramebufferWithoutValidation(framebufferObject);
	
	// tell GL how many draw buffer attachments we have
	std::vector <GLenum> drawBuffers(maxAttachments, GL_NONE);
	for (int i = 0; i < drawAttachments; i++)
		drawBuffers[i] = GL_COLOR_ATTACHMENT0+i;
	gl.DrawBuffers(drawBuffers.size(), &drawBuffers[0]);
	
	// TODO: re-use these from a common pool?
	// create a depth renderbuffer if we have no depth attachment
	if (depthAttachments == 0)
	{
		// generate the renderbuffer
		renderbuffer = gl.GenRenderbuffer();
		gl.BindRenderbuffer(GL_RENDERBUFFER, renderbuffer);
		gl.RenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width, height);
		
		// attach the renderbuffer
		gl.FramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, renderbuffer);
	}
	
	// create and attach color and depth render targets
	for (std::map <std::string, RealtimeExportPassInfo::RenderTargetInfo>::const_iterator i = config.renderTargets.begin(); i != config.renderTargets.end(); i++)
	{
		// find the render target attachment point
		GLenum attachmentPoint = GLEnumHelper.getEnum(i->first);
		
		// compute the width and height of the render target
		float rtWidthF = i->second.width;
		float rtHeightF = i->second.width;
		if (i->second.widthHeightAreMultiples)
		{
			rtWidthF *= w;
			rtHeightF *= h;
		}
		unsigned int rtWidth = (unsigned int) rtWidthF;
		unsigned int rtHeight = (unsigned int) rtHeightF;
		
		// determine the format for the texture
		// the internal format is the detailed format that is specified in the configuration,
		// but we need to determine the more general format and the component type ourselves
		std::string formatstr = i->second.format;
		GLenum internalFormat = GLEnumHelper.getEnum(formatstr);
		
		// we make sure we find a match for our format or error out
		GLenum format = GL_NONE;
		if (formatstr.find("GL_RGBA") == 0)
			format = GL_RGBA;
		else if (formatstr.find("GL_RG") == 0)
			format = GL_RG;
		else if (formatstr.find("GL_R") == 0)
			format = GL_RED;
		else if (formatstr.find("GL_DEPTH_COMPONENT") == 0)
			format = GL_DEPTH_COMPONENT;
		if (format == GL_NONE)
		{
			 errorOutput << "Unhandled render target format: " << formatstr << std::endl;
			 deleteFramebufferObject(gl);
			 return false;
		}
		
		// we default to the unsigned byte type and do not bother to error out
		GLenum type = GL_UNSIGNED_BYTE;
		if (formatstr.find("32F") != std::string::npos)
			type = GL_FLOAT;
		else if (formatstr.find("16F") != std::string::npos)
			type = GL_HALF_FLOAT;
		else if (formatstr.find("DEPTH_COMPONENT") != std::string::npos)
			type = GL_UNSIGNED_INT;
		
		// only 2d render targets are supported
		GLenum target = GL_TEXTURE_2D;

		StringId renderTargetNameId = stringMap.addStringId(i->second.name);
		
		// either use an existing render target texture or create a new one
		std::tr1::unordered_map <StringId, RenderTextureEntry, StringId::hash>::const_iterator sharedRenderTarget = 
			sharedTextures.find(renderTargetNameId);
		if (sharedRenderTarget != sharedTextures.end())
		{
			// this render target texture has already been created by a previous pass, we just want to use it
			RenderTexture texture(target, sharedRenderTarget->second.handle);
			
			// store the dependency
			externalRenderTargets.insert(std::make_pair(renderTargetNameId,texture));
			
			if (i->second.autoMipmap)
				autoMipMapRenderTargets.push_back(texture);
			
			// attach the render target to the framebuffer
			gl.FramebufferTexture2D(GL_DRAW_FRAMEBUFFER, attachmentPoint, texture.target, texture.handle, 0);
		}
		else
		{
			// create the render target texture
			RenderTexture texture(target, gl.GenTexture());
			
			// tell GL to allocate texture storage
			gl.BindTexture(texture.target, texture.handle);
			gl.TexImage2D(texture.target, 0, internalFormat, rtWidth, rtHeight, 0, format, type, NULL);
			
			// set some default texture parameters for now.
			// when we actually sample the texture we should set parameters that
			// match what we want in our texture, however...
			// this is only here to work around a problem with my geforce 7
			// drivers, where they will say the framebuffer setup is unsupported
			// unless they know some texture parameters from when we originally
			// create the texture.
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
			
			gl.unbindTexture(texture.target);
			
			// store the texture we created
			renderTargets.insert(std::make_pair(renderTargetNameId,texture));
			if (i->second.autoMipmap)
				autoMipMapRenderTargets.push_back(texture);
			
			// attach the render target to the framebuffer
			gl.FramebufferTexture2D(GL_DRAW_FRAMEBUFFER, attachmentPoint, texture.target, texture.handle, 0);
		}
	}
	
	// calling BindFramebuffer will do validation for us
	if (!gl.BindFramebuffer(framebufferObject))
	{
		errorOutput << "Framebuffer creation failed" << std::endl;
		gl.unbindFramebuffer();
		deleteFramebufferObject(gl);
		return false;
	}
	
	gl.unbindFramebuffer();
	
	return true;
}

void RenderPass::applyTexture(GLWrapper & gl, GLuint tu, GLenum target, GLuint handle)
{
	gl.ActiveTexture(tu);
	gl.BindTexture(target, handle);
}

bool RenderPass::createShaderProgram(GLWrapper & gl, const std::vector <std::string> & shaderAttributeBindings, const RenderShader & vertexShader, const RenderShader & fragmentShader, std::ostream & errorOutput)
{
	deleteShaderProgram(gl);
	
	std::vector <GLuint> shaderHandles;
	shaderHandles.push_back(vertexShader.handle);
	shaderHandles.push_back(fragmentShader.handle);
	
	return gl.linkShaderProgram(shaderAttributeBindings, shaderHandles, shaderProgram, errorOutput);
}

void RenderPass::deleteShaderProgram(GLWrapper & gl)
{
	if (shaderProgram != 0)
		gl.DeleteProgram(shaderProgram);
	shaderProgram = 0;
}

void RenderPass::addModel(const RenderModelEntry & entry, RenderModelHandle handle)
{
	// simply add a new model based on the entry, then remember the association with the handle
	RenderModel newModel(entry);
	modelHandles[handle] = models.insert(newModel);
}

void RenderPass::removeModel(RenderModelHandle handle)
{
	// find the handle in our models container and erase it
	modelHandleMap::iterator iter = modelHandles.find(handle);
	if (iter != modelHandles.end())
	{
		models.erase(iter->second);
	}
	else
		assert(!"removeModel: missing RenderModel");
}

void RenderPass::setModelTexture(RenderModelHandle handle, const RenderTextureEntry & texture)
{
	// find the model from the handle
	modelHandleMap::iterator iter = modelHandles.find(handle);
	if (iter != modelHandles.end())
	{
		RenderModel & model = models.get(iter->second);
		
		// first, see if there's an existing texture override with this name
		std::tr1::unordered_map <StringId, keyed_container <RenderTexture>::handle>::iterator existing = 
			model.textureNameToTextureOverride.find(texture.name);
		if (existing != model.textureNameToTextureOverride.end())
		{
			// there is an existing override. change it!
			RenderTexture & override = model.textureBindingOverrides.get(existing->second);
			override = RenderTexture(override.tu, texture);
		}
		else
		{
			// this is a new override
			std::tr1::unordered_map <StringId, GLuint>::iterator tui = textureNameToTextureUnit.find(texture.name);
			assert(tui != textureNameToTextureUnit.end()); // textureNameToTextureUnit should have been populated when we loaded the sampler
			GLuint tu = tui->second;
			
			// insert the override and store the mapping to the texture name so we can do updates later
			model.textureNameToTextureOverride[texture.name] = model.textureBindingOverrides.insert(RenderTexture(tu,texture));
		}
	}
	else
		assert(!"setModelTexture: missing RenderModel");
}

void RenderPass::removeModelTexture(RenderModelHandle handle, StringId name)
{
	// find the model from the handle
	modelHandleMap::iterator iter = modelHandles.find(handle);
	if (iter != modelHandles.end())
	{
		RenderModel & model = models.get(iter->second);
		
		// find the existing texture override with this name
		std::tr1::unordered_map <StringId, keyed_container <RenderTexture>::handle>::iterator existing = 
			model.textureNameToTextureOverride.find(name);
		if (existing != model.textureNameToTextureOverride.end())
		{
			// there is an existing override. remove it, then remove it from the mapping.
			model.textureBindingOverrides.erase(existing->second);
			model.textureNameToTextureOverride.erase(existing);
		}
		else
		{
			assert(!"removeModelTexture: missing override");
		}
	}
	else
		assert(!"removeModelTexture: missing RenderModel");
}

void RenderPass::setModelUniform(RenderModelHandle handle, const RenderUniformEntry & uniform)
{
	// find the model from the handle
	modelHandleMap::iterator iter = modelHandles.find(handle);
	if (iter != modelHandles.end())
	{
		RenderModel & model = models.get(iter->second);
		
		// first, see if there's an existing override with this name
		std::tr1::unordered_map <StringId, keyed_container <RenderUniform>::handle>::iterator existing = 
			model.variableNameToUniformOverride.find(uniform.name);
		if (existing != model.variableNameToUniformOverride.end())
		{
			// there is an existing override. change it!
			RenderUniform & override = model.uniformOverrides.get(existing->second);
			override = RenderUniform(override.location, uniform);
		}
		else
		{
			// this is a new override
			std::tr1::unordered_map <StringId, GLuint>::iterator loci = variableNameToUniformLocation.find(uniform.name);
			assert(loci != variableNameToUniformLocation.end()); // variableNameToUniformLocation should have been populated when we initialized
			GLuint loc = loci->second;
			
			// insert the override and store the mapping to the variable name so we can do updates later
			model.variableNameToUniformOverride[uniform.name] = model.uniformOverrides.insert(RenderUniform(loc,uniform));
		}
	}
	else
		assert(!"setModelUniform: missing RenderUniform");
}

void RenderPass::removeModelUniform(RenderModelHandle handle, StringId name)
{
	// find the model from the handle
	modelHandleMap::iterator iter = modelHandles.find(handle);
	if (iter != modelHandles.end())
	{
		RenderModel & model = models.get(iter->second);
		
		// find the existing uniform override with this name
		std::tr1::unordered_map <StringId, keyed_container <RenderUniform>::handle>::iterator existing = 
			model.variableNameToUniformOverride.find(name);
		if (existing != model.variableNameToUniformOverride.end())
		{
			// there is an existing override. remove it, then remove it from the mapping.
			model.uniformOverrides.erase(existing->second);
			model.variableNameToUniformOverride.erase(existing);
		}
		else
		{
			assert(!"removeModelUniform: missing override");
		}
	}
	else
		assert(!"removeModelUniform: missing RenderUniform");
}

void RenderPass::setDefaultTexture(StringId name, const RenderTextureEntry & texture)
{
	// see if we have a mapping for this name id
	// if we don't that's fine, just ignore the change
	std::tr1::unordered_map <StringId, GLuint>::const_iterator tuIter = textureNameToTextureUnit.find(name);
	if (tuIter != textureNameToTextureUnit.end())
	{
		GLuint tu = tuIter->second;
		
		// scan defaultTextureBindings to see if there's an existing texture with a tu that corresponds to this name id
		for (std::vector <RenderTexture>::iterator i = defaultTextureBindings.begin(); i != defaultTextureBindings.end(); i++)
		{
			if (i->tu == tu)
			{
				// overwrite the existing entry, then return
				*i = RenderTexture(tu, texture);
				return;
			}
		}
		
		// if we've gotten here, we don't have an existing entry. make one!
		defaultTextureBindings.push_back(RenderTexture(tu, texture));
	}
}

void RenderPass::removeDefaultTexture(StringId name)
{
	// see if we have a mapping for this name id
	// if we don't that's fine, just ignore the change
	std::tr1::unordered_map <StringId, GLuint>::const_iterator tuIter = textureNameToTextureUnit.find(name);
	if (tuIter != textureNameToTextureUnit.end())
	{
		GLuint tu = tuIter->second;
		
		// scan defaultTextureBindings to see if there's an existing texture with a tu that corresponds to this name id
		unsigned int indexToErase = 0;
		bool foundIndex = false;
		for (unsigned int i = 0; i < defaultTextureBindings.size(); i++)
		{
			if (defaultTextureBindings[i].tu == tu)
			{
				assert(!foundIndex); // this assert will fail if there is a double entry in defaultTextureBindings (i.e., two RenderTextures with the same tu entry)
				indexToErase = i;
				foundIndex = true;
				// could potentially break out of the for loop here, although then our assert above wouldn't ever fail, even with corrupt data
			}
		}
	
		// we have an entry, let's remove it
		if (foundIndex)
			UTILS::eraseVectorUseSwapAndPop(indexToErase, defaultTextureBindings);
	}
}

void RenderPass::setDefaultUniform(const RenderUniformEntry & uniform)
{
	// see if we have a mapping for this name id
	// if we don't that's fine, just ignore the change
	std::tr1::unordered_map <StringId, GLuint>::const_iterator locIter = variableNameToUniformLocation.find(uniform.name);
	if (locIter != variableNameToUniformLocation.end())
	{
		GLuint location = locIter->second;
		
		// scan defaultUniformBindings to see if there's an existing uniform with a location that corresponds to this name id
		for (std::vector <RenderUniform>::iterator i = defaultUniformBindings.begin(); i != defaultUniformBindings.end(); i++)
		{
			if (i->location == location)
			{
				// overwrite the existing entry, then return
				*i = RenderUniform(location, uniform);
				return;
			}
		}
		
		// if we've gotten here, we don't have an existing entry. make one!
		defaultUniformBindings.push_back(RenderUniform(location, uniform));
	}
}

void RenderPass::removeDefaultUniform(StringId name)
{
	// see if we have a mapping for this name id
	// if we don't that's fine, just ignore the change
	std::tr1::unordered_map <StringId, GLuint>::const_iterator locIter = variableNameToUniformLocation.find(name);
	if (locIter != variableNameToUniformLocation.end())
	{
		GLuint location = locIter->second;
		
		// scan defaultUniformBindings to see if there's an existing uniform with a location that corresponds to this name id
		unsigned int indexToErase = 0;
		bool foundIndex = false;
		for (unsigned int i = 0; i < defaultUniformBindings.size(); i++)
		{
			if (defaultUniformBindings[i].location == location)
			{
				assert(!foundIndex); // this assert will fail if there is a double entry in defaultUniformBindings (i.e., two RenderUniforms with the same location entry)
				indexToErase = i;
				foundIndex = true;
				// could potentially break out of the for loop here, although then our assert above wouldn't ever fail, even with corrupt data
			}
		}
	
		// we have an entry, let's remove it
		if (foundIndex)
			UTILS::eraseVectorUseSwapAndPop(indexToErase, defaultUniformBindings);
	}
}

bool RenderPass::getDefaultUniform(StringId uniformName, RenderUniform & out)
{
	std::tr1::unordered_map <StringId, GLuint>::const_iterator locIter = variableNameToUniformLocation.find(uniformName);
	if (locIter != variableNameToUniformLocation.end())
	{
		GLuint location = locIter->second;
		
		// scan defaultUniformBindings to see if there's a uniform with a location that corresponds to this name id
		for (std::vector <RenderUniform>::iterator i = defaultUniformBindings.begin(); i != defaultUniformBindings.end(); i++)
		{
			if (i->location == location)
			{
				out = *i;
				return true;
			}
		}
	}
	
	return false;
}



// helper functions for printRendererStatus

std::string printContextPrefix = "\t";
std::ostream * printContextOut = NULL;
const StringIdMap * printContextStringMap = NULL;

template <typename T>
std::ostream & operator<<(std::ostream & out, const std::vector <T> & vector)
{
	for (typename std::vector <T>::const_iterator i = vector.begin(); i != vector.end(); i++)
	{
		if (i != vector.begin())
			out << ", ";
		out << *i;
	}
	return out;
}

template <typename T>
std::ostream & operator<<(std::ostream & out, const RenderUniformVector <T> & vector)
{
	for (typename RenderUniformVector <T>::const_iterator i = vector.begin(); i != vector.end(); i++)
	{
		if (i != vector.begin())
			out << ", ";
		out << *i;
	}
	return out;
}

std::ostream & operator<<(std::ostream & out, const RenderTexture & tex)
{
	out << tex.handle;
	return out;
}

template <typename T, typename F>
void forEachInContainer(T & container, F f)
{
	std::for_each(container.begin(), container.end(), f);
}

void printRenderTextureHelper(const RenderTexture & texture)
{
	assert(printContextOut);
	*printContextOut << printContextPrefix << "TU: " << texture.tu << ", target: " << GLEnumHelper.getEnum(texture.target) << ", handle: " << texture.handle << std::endl;
}

void printRenderUniformHelper(const RenderUniform & uniform)
{
	assert(printContextOut);
	*printContextOut << printContextPrefix << "location: " << uniform.location << ", data: " << uniform.data << std::endl;
}

void printPairHelper(const std::pair <StringId, GLuint> & pair)
{
	assert(printContextStringMap);
	assert(printContextOut);
	*printContextOut << printContextPrefix << printContextStringMap->getString(pair.first) << ": " << pair.second << std::endl;
}

void printPairEnumHelper(const std::pair <GLenum, unsigned int> & pair)
{
	assert(printContextOut);
	*printContextOut << GLEnumHelper.getEnum(pair.first) << ":" << pair.second << " ";
}

void printEnumHelper(GLenum value)
{
	assert(printContextOut);
	*printContextOut << GLEnumHelper.getEnum(value) << " ";
}

void printPairRenderTargetHelper(const std::pair <StringId, RenderTexture> & pair)
{
	assert(printContextStringMap);
	assert(printContextOut);
	*printContextOut << printContextPrefix << printContextStringMap->getString(pair.first) << ": handle " << pair.second.handle << std::endl;
}

void printRenderStateHelper(const RenderState & state)
{
	assert(printContextOut);
	*printContextOut << printContextPrefix;
	state.debugPrint(*printContextOut, GLEnumHelper);
	*printContextOut << std::endl;
}

void RenderPass::printRendererStatus(RendererStatusVerbosity verbosity, const StringIdMap & stringMap, std::ostream & out) const
{
	/*VERBOSITY_PASSBRIEF = 0,
	VERBOSITY_PASSDETAIL,
	VERBOSITY_MODELS,
	VERBOSITY_MODELS_TEXTURES,
	VERBOSITY_MODELS_TEXTURES_UNIFORMS*/
	
	std::string prefix = "   ";
	
	printContextOut = &out;
	printContextPrefix = prefix;
	printContextStringMap = &stringMap;
	
	//out << prefix << << std::endl;
	
	{
		out << prefix << "Clear mask:";
		if (clearMask == 0)
			out << " (empty)";
		else
		{
			if (originalConfiguration.clearColor) out << " GL_COLOR_BUFFER_BIT";
			if (originalConfiguration.clearDepth) out << " GL_DEPTH_BUFFER_BIT";
			if (originalConfiguration.clearStencil) out << " GL_STENCIL_BUFFER_BIT";
		}
		out << std::endl;
	}
	
	{
		out << prefix << "Shader program: " << shaderProgram << std::endl;
		out << prefix << prefix << "Vertex shader: " << originalConfiguration.vertexShader << std::endl;
		out << prefix << prefix << "Fragment shader: " << originalConfiguration.fragmentShader << std::endl;
	}
	
	out << prefix << "Samplers: " << samplers.size() << " TU(s)" << std::endl;
	if (verbosity >= VERBOSITY_PASSDETAIL)
	{
		for (unsigned int i = 0; i < samplers.size(); i++)
		{
			out << prefix+prefix << "Sampler handle TU " << i << ": " << samplers[i].getHandle() << std::endl;
			out << prefix+prefix << "Sampler state TU " << i << ": " << std::endl;
			printContextPrefix = prefix+prefix+prefix;
			forEachInContainer(samplers[i].getStateVector(), printRenderStateHelper);
		}
		
		out << prefix+prefix << "Texture name assignments to TU:" << std::endl;
		printContextPrefix = prefix+prefix+prefix;
		forEachInContainer(textureNameToTextureUnit, printPairHelper);
		
		out << prefix << "Default textures: " << defaultTextureBindings.size() << std::endl;
		printContextPrefix = prefix+prefix;
		forEachInContainer(defaultTextureBindings, printRenderTextureHelper);
	}
	
	out << prefix << "Uniforms: " << variableNameToUniformLocation.size() << std::endl;
	if (verbosity >= VERBOSITY_PASSDETAIL)
	{
		printContextPrefix = prefix+prefix;
		forEachInContainer(variableNameToUniformLocation, printPairHelper);
		
		out << prefix << "Default uniforms: " << defaultUniformBindings.size() << std::endl;
		printContextPrefix = prefix+prefix;
		forEachInContainer(defaultUniformBindings, printRenderUniformHelper);
	}
	
	out << prefix << "Render dimensions: " << framebufferDimensions << std::endl;
	out << prefix << "Framebuffer object: " << (framebufferObject == 0 ? "default" : UTILS::tostr(framebufferObject)) << std::endl;
	out << prefix << "Using depth renderbuffer: " << (renderbuffer == 0 ? "no" : "yes") << std::endl;
	out << prefix << "Created render targets: " << renderTargets.size() << std::endl;
	printContextPrefix = prefix+prefix;
	forEachInContainer(renderTargets, printPairRenderTargetHelper);
	out << prefix << "External render targets: " << externalRenderTargets.size() << std::endl;
	printContextPrefix = prefix+prefix;
	forEachInContainer(externalRenderTargets, printPairRenderTargetHelper);
	out << prefix << "Auto-mipmapped render targets: " << (autoMipMapRenderTargets.empty() ? "none" : "") << autoMipMapRenderTargets << std::endl;
	
	if (verbosity >= VERBOSITY_PASSDETAIL)
	{
		out << prefix << "Enabled states: " << (stateEnable.empty() && stateEnablei.empty() ? "default" : "" );
		forEachInContainer(stateEnable, printEnumHelper);
		forEachInContainer(stateEnablei, printPairEnumHelper);
		out << std::endl;
		
		out << prefix << "Disabled states: " << (stateDisable.empty() && stateDisablei.empty() ? "default" : "" );
		forEachInContainer(stateDisable, printEnumHelper);
		forEachInContainer(stateDisablei, printPairEnumHelper);
		out << std::endl;
		
		out << prefix << "Additional state: " << (stateEnum.empty() ? "default" : "" ) << std::endl;
		printContextPrefix = prefix+prefix;
		forEachInContainer(stateEnum, printRenderStateHelper);
	}
	
	// TODO: models and their associated textures and uniforms
}

