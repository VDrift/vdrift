#include "renderer.h"
#include "utils.h"

#include <algorithm>

void Renderer::clear()
{
	// destroy shaders
	for (std::map <std::string, RenderShader>::iterator i = shaders.begin(); i != shaders.end(); i++)
	{
		gl.DeleteShader(i->second.handle);
	}
	shaders.clear();
	
	// tell each pass to clean itself up
	for (std::vector <RenderPass>::iterator i = passes.begin(); i != passes.end(); i++)
	{
		i->clear(gl);
	}
	
	// remove all passes
	passes.clear();
	sharedTextures.clear();
	passIndexMap.clear();
	
	// management of GL memory associated with the models is external
	models.clear();
	
	drawGroupToPasses.clear();
}

void Renderer::render(unsigned int w, unsigned int h, StringIdMap & stringMap, 
					  std::ostream & errorOutput)
{
	render(w, h, stringMap, std::map <StringId, std::vector <RenderModelExternal*> >(), errorOutput);
}

void Renderer::render(unsigned int w, unsigned int h, StringIdMap & stringMap, 
					  const std::map <StringId, std::vector <RenderModelExternal*> > & externalModels, 
					  std::ostream & errorOutput)
{
	for (std::vector <RenderPass>::iterator i = passes.begin(); i != passes.end(); i++)
	{
		if (i->render(gl, w, h, stringMap, externalModels, errorOutput))
		{
			// render targets have been recreated due to display dimension change
			// call setGlobalTexture to update sharedTextures and let downstream passes know
			const std::map <StringId, RenderTexture> & passRTs = passes.back().getRenderTargets();
			for (std::map <StringId, RenderTexture>::const_iterator rt = passRTs.begin(); rt != passRTs.end(); rt++)
			{
				setGlobalTexture(rt->first, RenderTextureEntry(rt->first, rt->second.handle, rt->second.target));
			}
		}
	}
}

bool Renderer::initialize(const std::vector <RealtimeExportPassInfo> & config, StringIdMap & stringMap,
	const std::string & shaderPath, unsigned int w, unsigned int h, std::ostream & errorOutput)
{
	// clear existing passes
	clear();
	
	// add new passes
	int passCount = 0;
	for (std::vector <RealtimeExportPassInfo>::const_iterator i = config.begin(); i != config.end(); i++,passCount++)
	{
		// create unique names based on the path and define list
		std::string vertexShaderName = i->vertexShader+" "+UTILS::implode(i->vertexShaderDefines," ");
		std::string fragmentShaderName = i->fragmentShader+" "+UTILS::implode(i->fragmentShaderDefines," ");
		
		// load shaders from the pass if necessary
		if (shaders.find(vertexShaderName) == shaders.end())
		{
			if (!loadShader(shaderPath.empty() ? i->vertexShader : shaderPath+"/"+i->vertexShader, 
				vertexShaderName, 
				i->vertexShaderDefines, 
				GL_VERTEX_SHADER, 
				errorOutput))
			{
				return false;
			}
		}
		if (shaders.find(fragmentShaderName) == shaders.end())
		{
			if (!loadShader(shaderPath.empty() ? i->fragmentShader : shaderPath+"/"+i->fragmentShader, 
				fragmentShaderName, 
				i->fragmentShaderDefines, 
				GL_FRAGMENT_SHADER, 
				errorOutput))
			{
				return false;
			}
		}
		
		// note which draw groups the pass uses
		for (std::vector <std::string>::const_iterator g = i->drawGroups.begin(); g != i->drawGroups.end(); g++)
		{
			drawGroupToPasses[stringMap.addStringId(*g)].push_back(passes.size());
		}
		
		// initialize the pass
		int passIdx = passes.size();
		passes.push_back(RenderPass());
		if (!passes.back().initialize(passCount, *i, stringMap, gl, shaders.find(vertexShaderName)->second, shaders.find(fragmentShaderName)->second, sharedTextures, w, h, errorOutput))
			return false;
		
		// put the pass's output render targets into a map so we can feed them to subsequent passes
		const std::map <StringId, RenderTexture> & passRTs = passes.back().getRenderTargets();
		for (std::map <StringId, RenderTexture>::const_iterator rt = passRTs.begin(); rt != passRTs.end(); rt++)
		{
			StringId nameId = rt->first;
			sharedTextures.insert(std::make_pair(nameId, 
				RenderTextureEntry(nameId, rt->second.handle, rt->second.target)));
		}
		
		// remember the pass index
		passIndexMap[stringMap.addStringId(i->name)] = passIdx;
	}
	
	return true;
}

bool Renderer::loadShader(const std::string & path, const std::string & name, const std::vector <std::string> & defines, GLenum shaderType, std::ostream & errorOutput)
{
	std::string shaderSource = UTILS::LoadFileIntoString(path, errorOutput);
	if (shaderSource.empty())
	{
		errorOutput << "Couldn't open shader file: "+path << std::endl;
		return false;
	}
	
	// create a block of #defines
	std::stringstream blockstream;
	for (unsigned int i = 0; i < defines.size(); i++)
	{
		if (!defines[i].empty())
		{
			blockstream << "#define " << defines[i] << std::endl;
		}
	}
	
	// insert the define block at the top of the shader, but after the "#version" line, if it exists
	if (shaderSource.substr(0,8) == "#version")
	{
		if (shaderSource.find('\n') != std::string::npos && shaderSource.find('\n') + 1 < shaderSource.size())
			shaderSource.insert(shaderSource.find('\n')+1, blockstream.str());
	}
	else
	{
		shaderSource = blockstream.str() + shaderSource;
	}
	
	GLuint handle(0);
	std::stringstream shaderOutput;
	bool success = gl.createAndCompileShader(shaderSource, shaderType, handle, shaderOutput);
	
	if (success)
	{
		RenderShader shader;
		shader.handle = handle;
		shaders.insert(std::make_pair(name, shader));
	}
	else
	{
		errorOutput << "Unable to compile shader " << name << " from file " << path << ":\n" << shaderOutput.str() << std::endl;
	}
	
	return success;
}

RenderModelHandle Renderer::addModel(const RenderModelEntry & entry)
{
	RenderModelHandle handle = models.insert(entry);
	
	bool unused = true;
	
	// for all passes that use this draw group, send them the model
	std::tr1::unordered_map <StringId, std::vector <unsigned int> >::iterator iter = drawGroupToPasses.find(models.get(handle).group);
	if (iter != drawGroupToPasses.end())
	{
		const std::vector <unsigned int> & passesWithDrawGroup = iter->second;
		
		for (std::vector <unsigned int>::const_iterator i = passesWithDrawGroup.begin(); i != passesWithDrawGroup.end(); i++)
		{
			unused = false;
			passes[*i].addModel(entry, handle);
		}
	}
	
	if (unused)
	{
		//errorOutput << "Warning: a model was added to draw group " << entry.group << ", which is currently unused by any passes." << std::endl;
	}
	
	return handle;
}

void Renderer::removeModel(RenderModelHandle handle)
{
	models.erase(handle);
	
	// for all passes that use this draw group, tell them to remove the model
	std::tr1::unordered_map <StringId, std::vector <unsigned int> >::iterator iter = drawGroupToPasses.find(models.get(handle).group);
	if (iter != drawGroupToPasses.end())
	{
		const std::vector <unsigned int> & passesWithDrawGroup = iter->second;
		
		for (std::vector <unsigned int>::const_iterator i = passesWithDrawGroup.begin(); i != passesWithDrawGroup.end(); i++)
		{
			passes[*i].removeModel(handle);
		}
	}
}

void Renderer::setModelTexture(RenderModelHandle handle, const RenderTextureEntry & texture)
{
	// for all passes that use this draw group, pass on the message
	std::tr1::unordered_map <StringId, std::vector <unsigned int> >::iterator iter = drawGroupToPasses.find(models.get(handle).group);
	if (iter != drawGroupToPasses.end())
	{
		const std::vector <unsigned int> & passesWithDrawGroup = iter->second;
		
		for (std::vector <unsigned int>::const_iterator i = passesWithDrawGroup.begin(); i != passesWithDrawGroup.end(); i++)
		{
			passes[*i].setModelTexture(handle, texture);
		}
	}
}

void Renderer::removeModelTexture(RenderModelHandle handle, StringId name)
{
	// for all passes that use this draw group, pass on the message
	std::tr1::unordered_map <StringId, std::vector <unsigned int> >::iterator iter = drawGroupToPasses.find(models.get(handle).group);
	if (iter != drawGroupToPasses.end())
	{
		const std::vector <unsigned int> & passesWithDrawGroup = iter->second;
		
		for (std::vector <unsigned int>::const_iterator i = passesWithDrawGroup.begin(); i != passesWithDrawGroup.end(); i++)
		{
			passes[*i].removeModelTexture(handle, name);
		}
	}
}

void Renderer::setModelUniform(RenderModelHandle handle, const RenderUniformEntry & uniform)
{
	// for all passes that use this draw group, pass on the message
	std::tr1::unordered_map <StringId, std::vector <unsigned int> >::iterator iter = drawGroupToPasses.find(models.get(handle).group);
	if (iter != drawGroupToPasses.end())
	{
		const std::vector <unsigned int> & passesWithDrawGroup = iter->second;
		
		for (std::vector <unsigned int>::const_iterator i = passesWithDrawGroup.begin(); i != passesWithDrawGroup.end(); i++)
		{
			passes[*i].setModelUniform(handle, uniform);
		}
	}
}

void Renderer::removeModelUniform(RenderModelHandle handle, StringId name)
{
	// for all passes that use this draw group, pass on the message
	std::tr1::unordered_map <StringId, std::vector <unsigned int> >::iterator iter = drawGroupToPasses.find(models.get(handle).group);
	if (iter != drawGroupToPasses.end())
	{
		const std::vector <unsigned int> & passesWithDrawGroup = iter->second;
		
		for (std::vector <unsigned int>::const_iterator i = passesWithDrawGroup.begin(); i != passesWithDrawGroup.end(); i++)
		{
			passes[*i].removeModelUniform(handle, name);
		}
	}
}

void Renderer::setGlobalTexture(StringId name, const RenderTextureEntry & texture)
{
	sharedTextures.insert(std::make_pair(name,texture));
	
	// inform the passes
	for (std::vector <RenderPass>::iterator i = passes.begin(); i != passes.end(); i++)
	{
		i->setDefaultTexture(name, texture);
	}
}

void Renderer::removeGlobalTexture(StringId name)
{
	sharedTextures.erase(name);
	
	// inform the passes
	for (std::vector <RenderPass>::iterator i = passes.begin(); i != passes.end(); i++)
	{
		i->removeDefaultTexture(name);
	}
}

void Renderer::setPassTexture(StringId passName, StringId textureName, const RenderTextureEntry & texture)
{
	std::tr1::unordered_map <StringId, int>::iterator i = passIndexMap.find(passName);
	if (i != passIndexMap.end())
	{
		assert((unsigned int)i->second < passes.size());
		passes[i->second].setDefaultTexture(textureName, texture);
	}
}

void Renderer::removePassTexture(StringId passName, StringId textureName)
{
	std::tr1::unordered_map <StringId, int>::iterator i = passIndexMap.find(passName);
	if (i != passIndexMap.end())
	{
		assert((unsigned int)i->second < passes.size());
		passes[i->second].removeDefaultTexture(textureName);
	}
}

void Renderer::setGlobalUniform(const RenderUniformEntry & uniform)
{
	// inform the passes
	for (std::vector <RenderPass>::iterator i = passes.begin(); i != passes.end(); i++)
	{
		i->setDefaultUniform(uniform);
	}
}

void Renderer::removeGlobalUniform(StringId name)
{
	// inform the passes
	for (std::vector <RenderPass>::iterator i = passes.begin(); i != passes.end(); i++)
	{
		i->removeDefaultUniform(name);
	}
}

void Renderer::setPassUniform(StringId passName, const RenderUniformEntry & uniform)
{
	std::tr1::unordered_map <StringId, int>::iterator i = passIndexMap.find(passName);
	if (i != passIndexMap.end())
	{
		assert((unsigned int)i->second < passes.size());
		passes[i->second].setDefaultUniform(uniform);
	}
}

void Renderer::removePassUniform(StringId passName, StringId uniformName)
{
	std::tr1::unordered_map <StringId, int>::iterator i = passIndexMap.find(passName);
	if (i != passIndexMap.end())
	{
		assert((unsigned int)i->second < passes.size());
		passes[i->second].removeDefaultUniform(uniformName);
	}
}

bool Renderer::getPassUniform(StringId passName, StringId uniformName, RenderUniform & out)
{
	std::tr1::unordered_map <StringId, int>::iterator i = passIndexMap.find(passName);
	if (i != passIndexMap.end())
	{
		assert((unsigned int)i->second < passes.size());
		return passes[i->second].getDefaultUniform(uniformName, out);
	}
	else
		return false;
}

void Renderer::setPassEnabled(StringId passName, bool enable)
{
	std::tr1::unordered_map <StringId, int>::iterator i = passIndexMap.find(passName);
	if (i != passIndexMap.end())
	{
		assert((unsigned int)i->second < passes.size());
		passes[i->second].setEnabled(enable);
	}
}

void Renderer::printRendererStatus(RendererStatusVerbosity verbosity, const StringIdMap & stringMap, std::ostream & out) const
{
	out << "Renderer status" << std::endl;
	out << "---------------" << std::endl;
	out << "Model count: " << models.size() << std::endl;
	out << "Shaders: ";
	for (std::map <std::string, RenderShader>::const_iterator i = shaders.begin(); i != shaders.end(); i++)
	{
		if (i != shaders.begin())
			out << ", ";
		out << i->first;
	}
	out << std::endl;
	
	std::string passPrefix = "   ";
	
	// print shared textures
	if (verbosity >= VERBOSITY_MODELS_TEXTURES)
	{
		out << "Global textures: " << sharedTextures.size() << std::endl;
		for (std::tr1::unordered_map <StringId, RenderTextureEntry>::const_iterator i = sharedTextures.begin(); i != sharedTextures.end(); i++)
		{
			out << passPrefix << stringMap.getString(i->first) << ", handle " << i->second.handle << std::endl;
		}
	}
	
	out << "Passes: " << passes.size() << std::endl;
	int passcount = 0;
	for (std::vector <RenderPass>::const_iterator i = passes.begin(); i != passes.end(); i++,passcount++)
	{
		out << "Pass \"" << i->getName() << "\"" << std::endl;
		
		out << passPrefix << "Draw groups: ";
		int dgcount = 0;
		for (std::tr1::unordered_map <StringId, std::vector <unsigned int> >::const_iterator dg = drawGroupToPasses.begin(); dg != drawGroupToPasses.end(); dg++)
		{
			if (std::find(dg->second.begin(), dg->second.end(), passcount) != dg->second.end())
			{
				if (dgcount != 0)
					out << ", ";
				out << stringMap.getString(dg->first);
				dgcount++;
			}
		}
		out << std::endl;
		
		i->printRendererStatus(verbosity, stringMap, out);
	}
}

