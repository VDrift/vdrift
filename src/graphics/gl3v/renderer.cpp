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

#include <algorithm>
#include "renderer.h"
#include "utils.h"

Renderer::Renderer(GLWrapper & glwrapper) : gl(glwrapper)
{
	// Constructor.
}

template <typename T>
static std::set <T> mergeSets(const std::set <T> & set1, const std::set <T> & set2)
{
	std::set <T> result = set1;
	for (const auto & item : set2)
		result.insert(item);
	return result;
}

bool Renderer::initialize(const std::vector <RealtimeExportPassInfo> & config, StringIdMap & stringMap, const std::string & shaderPath, unsigned int w,unsigned int h, const std::set <std::string> & globalDefines, std::ostream & errorOutput)
{
	// Clear existing passes.
	clear();

	// Add new passes.
	int passCount = 0;
	for (const auto & passInfo : config)
	{
		// Create unique names based on the path and define list.
		std::string vertexShaderName = passInfo.vertexShader;
		if (!passInfo.vertexShaderDefines.empty())
			vertexShaderName += " "+Utils::implode(passInfo.vertexShaderDefines," ");
		std::string fragmentShaderName = passInfo.fragmentShader;
		if (!passInfo.fragmentShaderDefines.empty())
			fragmentShaderName += " "+Utils::implode(passInfo.fragmentShaderDefines," ");

		// Load shaders from the pass if necessary.
		if ((shaders.find(vertexShaderName) == shaders.end()) && (!loadShader(shaderPath.empty() ? passInfo.vertexShader : shaderPath+"/"+passInfo.vertexShader, vertexShaderName, mergeSets(passInfo.vertexShaderDefines, globalDefines), GL_VERTEX_SHADER, errorOutput)))
			return false;
		if ((shaders.find(fragmentShaderName) == shaders.end()) && (!loadShader(shaderPath.empty() ? passInfo.fragmentShader : shaderPath+"/"+passInfo.fragmentShader, fragmentShaderName, mergeSets(passInfo.fragmentShaderDefines, globalDefines), GL_FRAGMENT_SHADER, errorOutput)))
			return false;

		// Note which draw groups the pass uses.
		for (const auto & dg : passInfo.drawGroups)
			drawGroupToPasses[stringMap.addStringId(dg)].push_back(passes.size());

		// Initialize the pass.
		int passIdx = passes.size();
		passes.push_back(RenderPass());
		if (!passes.back().initialize(passCount, passInfo, stringMap, gl, shaders.find(vertexShaderName)->second, shaders.find(fragmentShaderName)->second, sharedTextures, w, h, errorOutput))
			return false;

		// Put the pass's output render targets into a map so we can feed them to subsequent passes.
		for (const auto & rt : passes.back().getRenderTargets())
		{
			StringId nameId = rt.first;
			sharedTextures.insert(std::make_pair(nameId, RenderTextureEntry(nameId, rt.second.handle, rt.second.target)));
		}

		// Remember the pass index.
		passIndexMap[stringMap.addStringId(passInfo.name)] = passIdx;
		passCount++;
	}

	return true;
}

void Renderer::clear()
{
	// Destroy shaders.
	for (auto & shader : shaders)
		gl.DeleteShader(shader.second.handle);
	shaders.clear();

	// Tell each pass to clean itself up.
	for (auto & pass : passes)
		pass.clear(gl);

	// Remove all passes.
	passes.clear();
	sharedTextures.clear();
	passIndexMap.clear();

	// Management of GL memory associated with the models is external.
	models.clear();

	drawGroupToPasses.clear();
}

void Renderer::render(unsigned int w, unsigned int h, StringIdMap & stringMap, std::ostream & errorOutput)
{
	render(w, h, stringMap, std::map <StringId, std::vector <RenderModelExt*> >(), errorOutput);
}

void Renderer::render(unsigned int w, unsigned int h, StringIdMap & stringMap, const std::map <StringId, std::vector <RenderModelExt*> > & externalModels, std::ostream & errorOutput)
{
	for (auto & pass : passes)
	{
		// Extract the appropriate map and generate a drawlist.
		// A vector of pointers to vectors of RenderModelExternal pointers.
		std::vector <const std::vector <RenderModelExt*>*> drawList;

		// For each draw group that this pass uses, add its models to the draw list.
		for (auto dg : pass.getDrawGroups())
		{
			auto drawGroupIter = externalModels.find(dg);
			if (drawGroupIter != externalModels.end())
				drawList.push_back(&drawGroupIter->second);
		}

		if (pass.render(gl, w, h, stringMap, drawList, sharedTextures, errorOutput))
		{
			// Render targets have been recreated due to display dimension change.
			// Call setGlobalTexture to update sharedTextures and let downstream passes know.
			for (const auto & rt : passes.back().getRenderTargets())
				setGlobalTexture(rt.first, RenderTextureEntry(rt.first, rt.second.handle, rt.second.target));
		}
	}
}

void Renderer::render(unsigned int w, unsigned int h, StringIdMap & stringMap, const std::map <StringId, std::map <StringId, std::vector <RenderModelExt*> *> > & externalModels, std::ostream & errorOutput)
{
	for (auto & pass : passes)
	{
		// Extract the appropriate map and generate a drawlist.
		// A vector of pointers to vectors of RenderModelExternal pointers.
		std::vector <const std::vector <RenderModelExt*>*> drawList;

		// Find the map appropriate to this pass.
		auto drawMapIter = externalModels.find(pass.getNameId());
		if (drawMapIter != externalModels.end())
			// For each draw group that this pass uses, add its models to the draw list.
			for (auto dg : pass.getDrawGroups())
			{
				auto drawGroupIter = drawMapIter->second.find(dg);
				if (drawGroupIter != drawMapIter->second.end())
					drawList.push_back(drawGroupIter->second);
			}

		if (pass.render(gl, w, h, stringMap, drawList, sharedTextures, errorOutput))
		{
			// Render targets have been recreated due to display dimension change.
			// Call setGlobalTexture to update sharedTextures and let downstream passes know.
			for (const auto & rt : passes.back().getRenderTargets())
				setGlobalTexture(rt.first, RenderTextureEntry(rt.first, rt.second.handle, rt.second.target));
		}
	}
}

RenderModelHandle Renderer::addModel(const RenderModelEntry & entry)
{
	RenderModelHandle handle = models.insert(entry);

	// For all passes that use this draw group, send them the model.
	auto iter = drawGroupToPasses.find(models.get(handle).group);
	if (iter != drawGroupToPasses.end())
	{
		const std::vector <unsigned int> & passesWithDrawGroup = iter->second;

		for (unsigned int i : passesWithDrawGroup)
			passes[i].addModel(entry, handle);
	}

	return handle;
}

void Renderer::removeModel(RenderModelHandle handle)
{
	models.erase(handle);

	// For all passes that use this draw group, tell them to remove the model.
	auto iter = drawGroupToPasses.find(models.get(handle).group);
	if (iter != drawGroupToPasses.end())
	{
		const std::vector <unsigned int> & passesWithDrawGroup = iter->second;

		for (unsigned int i : passesWithDrawGroup)
			passes[i].removeModel(handle);
	}
}

void Renderer::setModelTexture(RenderModelHandle handle, const RenderTextureEntry & texture)
{
	// For all passes that use this draw group, pass on the message.
	auto iter = drawGroupToPasses.find(models.get(handle).group);
	if (iter != drawGroupToPasses.end())
	{
		const std::vector <unsigned int> & passesWithDrawGroup = iter->second;

		for (unsigned int i : passesWithDrawGroup)
			passes[i].setModelTexture(handle, texture);
	}
}

void Renderer::removeModelTexture(RenderModelHandle handle, StringId name)
{
	// For all passes that use this draw group, pass on the message.
	auto iter = drawGroupToPasses.find(models.get(handle).group);
	if (iter != drawGroupToPasses.end())
	{
		const std::vector <unsigned int> & passesWithDrawGroup = iter->second;

		for (unsigned int i : passesWithDrawGroup)
			passes[i].removeModelTexture(handle, name);
	}
}

void Renderer::setModelUniform(RenderModelHandle handle, const RenderUniformEntry & uniform)
{
	// For all passes that use this draw group, pass on the message.
	auto iter = drawGroupToPasses.find(models.get(handle).group);
	if (iter != drawGroupToPasses.end())
	{
		const std::vector <unsigned int> & passesWithDrawGroup = iter->second;

		for (unsigned int i : passesWithDrawGroup)
			passes[i].setModelUniform(handle, uniform);
	}
}

void Renderer::removeModelUniform(RenderModelHandle handle, StringId name)
{
	// For all passes that use this draw group, pass on the message.
	auto iter = drawGroupToPasses.find(models.get(handle).group);
	if (iter != drawGroupToPasses.end())
	{
		const std::vector <unsigned int> & passesWithDrawGroup = iter->second;

		for (unsigned int i : passesWithDrawGroup)
			passes[i].removeModelUniform(handle, name);
	}
}

void Renderer::setGlobalTexture(StringId name, const RenderTextureEntry & texture)
{
	sharedTextures.insert(std::make_pair(name,texture));

	// Inform the passes.
	for (auto & pass : passes)
		pass.setDefaultTexture(name, texture);
}

void Renderer::removeGlobalTexture(StringId name)
{
	sharedTextures.erase(name);

	// Inform the passes.
	for (auto & pass : passes)
		pass.removeDefaultTexture(name);
}

void Renderer::setPassTexture(StringId passName, StringId textureName, const RenderTextureEntry & texture)
{
	auto i = passIndexMap.find(passName);
	if (i != passIndexMap.end())
	{
		assert(i->second < passes.size());
		passes[i->second].setDefaultTexture(textureName, texture);
	}
}

void Renderer::removePassTexture(StringId passName, StringId textureName)
{
	auto i = passIndexMap.find(passName);
	if (i != passIndexMap.end())
	{
		assert(i->second < passes.size());
		passes[i->second].removeDefaultTexture(textureName);
	}
}

int Renderer::setGlobalUniform(const RenderUniformEntry & uniform)
{
	int passesAffected = 0;

	// Inform the passes.
	for (auto & pass : passes)
		if (pass.setDefaultUniform(uniform))
			passesAffected++;

	return passesAffected;
}

void Renderer::removeGlobalUniform(StringId name)
{
	// Inform the passes.
	for (auto & pass : passes)
		pass.removeDefaultUniform(name);
}

void Renderer::setPassUniform(StringId passName, const RenderUniformEntry & uniform)
{
	auto i = passIndexMap.find(passName);
	if (i != passIndexMap.end())
	{
		assert(i->second < passes.size());
		passes[i->second].setDefaultUniform(uniform);
	}
}

void Renderer::removePassUniform(StringId passName, StringId uniformName)
{
	auto i = passIndexMap.find(passName);
	if (i != passIndexMap.end())
	{
		assert(i->second < passes.size());
		passes[i->second].removeDefaultUniform(uniformName);
	}
}

bool Renderer::getPassUniform(StringId passName, StringId uniformName, RenderUniform & out)
{
	auto i = passIndexMap.find(passName);
	if (i != passIndexMap.end())
	{
		assert(i->second < passes.size());
		return passes[i->second].getDefaultUniform(uniformName, out);
	}
	else
		return false;
}

void Renderer::setPassEnabled(StringId passName, bool enable)
{
	auto i = passIndexMap.find(passName);
	if (i != passIndexMap.end())
	{
		assert(i->second < passes.size());
		passes[i->second].setEnabled(enable);
	}
}

bool Renderer::getPassEnabled(StringId passName) const
{
	auto i = passIndexMap.find(passName);
	if (i != passIndexMap.end())
	{
		assert(i->second < passes.size());
		return passes[i->second].getEnabled();
	}
	return false;
}

static const std::map <std::string, std::string> emptyStringMap;
const std::map <std::string, std::string> & Renderer::getUserDefinedFields(StringId passName) const
{
	auto i = passIndexMap.find(passName);
	if (i != passIndexMap.end())
	{
		assert(i->second < passes.size());
		return passes[i->second].getUserDefinedFields();
	}
	return emptyStringMap;
}

static const std::set <StringId> emptySet;
const std::set <StringId> & Renderer::getDrawGroups(StringId passName) const
{
	auto i = passIndexMap.find(passName);
	if (i != passIndexMap.end())
	{
		assert(i->second < passes.size());
		return passes[i->second].getDrawGroups();
	}
	return emptySet;
}

std::vector <StringId> Renderer::getPassNames() const
{
	std::vector <StringId> names;
	names.reserve(passes.size());

	for (const auto & pass : passes)
		names.push_back(pass.getNameId());

	return names;
}

void Renderer::printRendererStatus(RendererStatusVerbosity verbosity, const StringIdMap & stringMap, std::ostream & out) const
{
	out << "Renderer status" << std::endl;
	out << "---------------" << std::endl;
	out << "Model count: " << models.size() << std::endl;
	out << "Shaders: ";
	for (auto i = shaders.begin(); i != shaders.end(); i++)
	{
		if (i != shaders.begin())
			out << ", ";
		out << i->first;
		if (!i->second.defines.empty())
		{
			out << "(";
			for (auto d = i->second.defines.begin(); d != i->second.defines.end(); d++)
			{
				if (d != i->second.defines.begin())
					out << "/";
				out << *d;
			}
			out << ")";
		}
	}
	out << std::endl;

	std::string passPrefix = "   ";

	// Print shared textures.
	if (verbosity >= VERBOSITY_MODELS_TEXTURES)
	{
		out << "Global textures: " << sharedTextures.size() << std::endl;
		for (const auto & st : sharedTextures)
			out << passPrefix << stringMap.getString(st.first) << ", handle " << st.second.handle << std::endl;
	}

	out << "Passes: " << passes.size() << std::endl;
	int passcount = 0;
	for (const auto & pass : passes)
	{
		out << "Pass \"" << pass.getName() << "\"" << std::endl;

		out << passPrefix << "Draw groups: ";
		int dgcount = 0;
		for (const auto & dg : drawGroupToPasses)
			if (std::find(dg.second.begin(), dg.second.end(), passcount) != dg.second.end())
			{
				if (dgcount != 0)
					out << ", ";
				out << stringMap.getString(dg.first);
				dgcount++;
			}
		out << std::endl;

		pass.printRendererStatus(verbosity, stringMap, out);
		passcount++;
	}
}

void Renderer::printProfilingInfo(std::ostream & out) const
{
	for (const auto & pass : passes)
		out << pass.getName() << ": " << pass.getLastTime() * 1E6f << " us" << std::endl;
}

bool Renderer::loadShader(const std::string & path, const std::string & name, const std::set <std::string> & defines, GLenum shaderType, std::ostream & errorOutput)
{
	std::string shaderSource = Utils::LoadFileIntoString(path, errorOutput);
	if (shaderSource.empty())
	{
		errorOutput << "Couldn't open shader file: "+path << std::endl;
		return false;
	}

	// Create a block of #defines.
	std::ostringstream blockstream;
	for (const auto & define : defines)
		if (!define.empty())
			blockstream << "#define " << define << std::endl;

	// Insert the define block at the top of the shader, but after the "#version" line, if it exists.
	if ((shaderSource.substr(0,8) == "#version") && (shaderSource.find('\n') != std::string::npos && shaderSource.find('\n') + 1 < shaderSource.size()))
		shaderSource.insert(shaderSource.find('\n')+1, blockstream.str());
	else
		shaderSource = blockstream.str() + shaderSource;

	GLuint handle(0);
	std::ostringstream shaderOutput;
	bool success = gl.createAndCompileShader(shaderSource, shaderType, handle, shaderOutput);

	if (success)
	{
		RenderShader shader;
		shader.handle = handle;
		shader.defines = defines; // for debug only
		shaders.insert(std::make_pair(name, shader));
	}
	else
		errorOutput << "Unable to compile shader " << name << " from file " << path << ":\n" << shaderOutput.str() << std::endl;

	return success;
}
