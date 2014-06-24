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
	for (typename std::set <T>::const_iterator i = set2.begin(); i != set2.end(); i++)
		result.insert(*i);
	return result;
}

bool Renderer::initialize(const std::vector <RealtimeExportPassInfo> & config, StringIdMap & stringMap, const std::string & shaderPath, unsigned int w,unsigned int h, const std::set <std::string> & globalDefines, std::ostream & errorOutput)
{
	// Clear existing passes.
	clear();

	// Add new passes.
	int passCount = 0;
	for (std::vector <RealtimeExportPassInfo>::const_iterator i = config.begin(); i != config.end(); i++, passCount++)
	{
		// Create unique names based on the path and define list.
		std::string vertexShaderName = i->vertexShader;
		if (!i->vertexShaderDefines.empty())
			vertexShaderName += " "+Utils::implode(i->vertexShaderDefines," ");
		std::string fragmentShaderName = i->fragmentShader;
		if (!i->fragmentShaderDefines.empty())
			fragmentShaderName += " "+Utils::implode(i->fragmentShaderDefines," ");

		// Load shaders from the pass if necessary.
		if ((shaders.find(vertexShaderName) == shaders.end()) && (!loadShader(shaderPath.empty() ? i->vertexShader : shaderPath+"/"+i->vertexShader, vertexShaderName, mergeSets(i->vertexShaderDefines, globalDefines), GL_VERTEX_SHADER, errorOutput)))
			return false;
		if ((shaders.find(fragmentShaderName) == shaders.end()) && (!loadShader(shaderPath.empty() ? i->fragmentShader : shaderPath+"/"+i->fragmentShader, fragmentShaderName, mergeSets(i->fragmentShaderDefines, globalDefines), GL_FRAGMENT_SHADER, errorOutput)))
			return false;

		// Note which draw groups the pass uses.
		for (std::vector <std::string>::const_iterator g = i->drawGroups.begin(); g != i->drawGroups.end(); g++)
			drawGroupToPasses[stringMap.addStringId(*g)].push_back(passes.size());

		// Initialize the pass.
		int passIdx = passes.size();
		passes.push_back(RenderPass());
		if (!passes.back().initialize(passCount, *i, stringMap, gl, shaders.find(vertexShaderName)->second, shaders.find(fragmentShaderName)->second, sharedTextures, w, h, errorOutput))
			return false;

		// Put the pass's output render targets into a map so we can feed them to subsequent passes.
		const std::map <StringId, RenderTexture> & passRTs = passes.back().getRenderTargets();
		for (std::map <StringId, RenderTexture>::const_iterator rt = passRTs.begin(); rt != passRTs.end(); rt++)
		{
			StringId nameId = rt->first;
			sharedTextures.insert(std::make_pair(nameId, RenderTextureEntry(nameId, rt->second.handle, rt->second.target)));
		}

		// Remember the pass index.
		passIndexMap[stringMap.addStringId(i->name)] = passIdx;
	}

	return true;
}

void Renderer::clear()
{
	// Destroy shaders.
	for (std::map <std::string, RenderShader>::iterator i = shaders.begin(); i != shaders.end(); i++)
		gl.DeleteShader(i->second.handle);
	shaders.clear();

	// Tell each pass to clean itself up.
	for (std::vector <RenderPass>::iterator i = passes.begin(); i != passes.end(); i++)
		i->clear(gl);

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
	for (std::vector <RenderPass>::iterator i = passes.begin(); i != passes.end(); i++)
	{
		// Extract the appropriate map and generate a drawlist.
		// A vector of pointers to vectors of RenderModelExternal pointers.
		std::vector <const std::vector <RenderModelExt*>*> drawList;

		// For each draw group that this pass uses, add its models to the draw list.
		for (std::set <StringId>::const_iterator dg = i->getDrawGroups().begin(); dg != i->getDrawGroups().end(); dg++)
		{
			std::map <StringId, std::vector <RenderModelExt*> >::const_iterator drawGroupIter = externalModels.find(*dg);
			if (drawGroupIter != externalModels.end())
				drawList.push_back(&drawGroupIter->second);
		}

		if (i->render(gl, w, h, stringMap, drawList, sharedTextures, errorOutput))
		{
			// Render targets have been recreated due to display dimension change.
			// Call setGlobalTexture to update sharedTextures and let downstream passes know.
			const std::map <StringId, RenderTexture> & passRTs = passes.back().getRenderTargets();
			for (std::map <StringId, RenderTexture>::const_iterator rt = passRTs.begin(); rt != passRTs.end(); rt++)
				setGlobalTexture(rt->first, RenderTextureEntry(rt->first, rt->second.handle, rt->second.target));
		}
	}
}

void Renderer::render(unsigned int w, unsigned int h, StringIdMap & stringMap, const std::map <StringId, std::map <StringId, std::vector <RenderModelExt*> *> > & externalModels, std::ostream & errorOutput)
{
	for (std::vector <RenderPass>::iterator i = passes.begin(); i != passes.end(); i++)
	{
		// Extract the appropriate map and generate a drawlist.
		// A vector of pointers to vectors of RenderModelExternal pointers.
		std::vector <const std::vector <RenderModelExt*>*> drawList;

		// Find the map appropriate to this pass.
		std::map <StringId, std::map <StringId, std::vector <RenderModelExt*> *> >::const_iterator drawMapIter = externalModels.find(i->getNameId());
		if (drawMapIter != externalModels.end())
			// For each draw group that this pass uses, add its models to the draw list.
			for (std::set <StringId>::const_iterator dg = i->getDrawGroups().begin(); dg != i->getDrawGroups().end(); dg++)
			{
				std::map <StringId, std::vector <RenderModelExt*> *>::const_iterator drawGroupIter = drawMapIter->second.find(*dg);
				if (drawGroupIter != drawMapIter->second.end())
					drawList.push_back(drawGroupIter->second);
			}

		if (i->render(gl, w, h, stringMap, drawList, sharedTextures, errorOutput))
		{
			// Render targets have been recreated due to display dimension change.
			// Call setGlobalTexture to update sharedTextures and let downstream passes know.
			const std::map <StringId, RenderTexture> & passRTs = passes.back().getRenderTargets();
			for (std::map <StringId, RenderTexture>::const_iterator rt = passRTs.begin(); rt != passRTs.end(); rt++)
				setGlobalTexture(rt->first, RenderTextureEntry(rt->first, rt->second.handle, rt->second.target));
		}
	}
}

RenderModelHandle Renderer::addModel(const RenderModelEntry & entry)
{
	RenderModelHandle handle = models.insert(entry);

	// For all passes that use this draw group, send them the model.
	NameIdVecMap::const_iterator iter = drawGroupToPasses.find(models.get(handle).group);
	if (iter != drawGroupToPasses.end())
	{
		const std::vector <unsigned int> & passesWithDrawGroup = iter->second;

		for (std::vector <unsigned int>::const_iterator i = passesWithDrawGroup.begin(); i != passesWithDrawGroup.end(); i++)
			passes[*i].addModel(entry, handle);
	}

	return handle;
}

void Renderer::removeModel(RenderModelHandle handle)
{
	models.erase(handle);

	// For all passes that use this draw group, tell them to remove the model.
	NameIdVecMap::const_iterator iter = drawGroupToPasses.find(models.get(handle).group);
	if (iter != drawGroupToPasses.end())
	{
		const std::vector <unsigned int> & passesWithDrawGroup = iter->second;

		for (std::vector <unsigned int>::const_iterator i = passesWithDrawGroup.begin(); i != passesWithDrawGroup.end(); i++)
			passes[*i].removeModel(handle);
	}
}

void Renderer::setModelTexture(RenderModelHandle handle, const RenderTextureEntry & texture)
{
	// For all passes that use this draw group, pass on the message.
	NameIdVecMap::const_iterator iter = drawGroupToPasses.find(models.get(handle).group);
	if (iter != drawGroupToPasses.end())
	{
		const std::vector <unsigned int> & passesWithDrawGroup = iter->second;

		for (std::vector <unsigned int>::const_iterator i = passesWithDrawGroup.begin(); i != passesWithDrawGroup.end(); i++)
			passes[*i].setModelTexture(handle, texture);
	}
}

void Renderer::removeModelTexture(RenderModelHandle handle, StringId name)
{
	// For all passes that use this draw group, pass on the message.
	NameIdVecMap::const_iterator iter = drawGroupToPasses.find(models.get(handle).group);
	if (iter != drawGroupToPasses.end())
	{
		const std::vector <unsigned int> & passesWithDrawGroup = iter->second;

		for (std::vector <unsigned int>::const_iterator i = passesWithDrawGroup.begin(); i != passesWithDrawGroup.end(); i++)
			passes[*i].removeModelTexture(handle, name);
	}
}

void Renderer::setModelUniform(RenderModelHandle handle, const RenderUniformEntry & uniform)
{
	// For all passes that use this draw group, pass on the message.
	NameIdVecMap::const_iterator iter = drawGroupToPasses.find(models.get(handle).group);
	if (iter != drawGroupToPasses.end())
	{
		const std::vector <unsigned int> & passesWithDrawGroup = iter->second;

		for (std::vector <unsigned int>::const_iterator i = passesWithDrawGroup.begin(); i != passesWithDrawGroup.end(); i++)
			passes[*i].setModelUniform(handle, uniform);
	}
}

void Renderer::removeModelUniform(RenderModelHandle handle, StringId name)
{
	// For all passes that use this draw group, pass on the message.
	NameIdVecMap::const_iterator iter = drawGroupToPasses.find(models.get(handle).group);
	if (iter != drawGroupToPasses.end())
	{
		const std::vector <unsigned int> & passesWithDrawGroup = iter->second;

		for (std::vector <unsigned int>::const_iterator i = passesWithDrawGroup.begin(); i != passesWithDrawGroup.end(); i++)
			passes[*i].removeModelUniform(handle, name);
	}
}

void Renderer::setGlobalTexture(StringId name, const RenderTextureEntry & texture)
{
	sharedTextures.insert(std::make_pair(name,texture));

	// Inform the passes.
	for (std::vector <RenderPass>::iterator i = passes.begin(); i != passes.end(); i++)
		i->setDefaultTexture(name, texture);
}

void Renderer::removeGlobalTexture(StringId name)
{
	sharedTextures.erase(name);

	// Inform the passes.
	for (std::vector <RenderPass>::iterator i = passes.begin(); i != passes.end(); i++)
		i->removeDefaultTexture(name);
}

void Renderer::setPassTexture(StringId passName, StringId textureName, const RenderTextureEntry & texture)
{
	NameIdMap::const_iterator i = passIndexMap.find(passName);
	if (i != passIndexMap.end())
	{
		assert(i->second < passes.size());
		passes[i->second].setDefaultTexture(textureName, texture);
	}
}

void Renderer::removePassTexture(StringId passName, StringId textureName)
{
	NameIdMap::const_iterator i = passIndexMap.find(passName);
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
	for (std::vector <RenderPass>::iterator i = passes.begin(); i != passes.end(); i++)
		if (i->setDefaultUniform(uniform))
			passesAffected++;

	return passesAffected;
}

void Renderer::removeGlobalUniform(StringId name)
{
	// Inform the passes.
	for (std::vector <RenderPass>::iterator i = passes.begin(); i != passes.end(); i++)
		i->removeDefaultUniform(name);
}

void Renderer::setPassUniform(StringId passName, const RenderUniformEntry & uniform)
{
	NameIdMap::const_iterator i = passIndexMap.find(passName);
	if (i != passIndexMap.end())
	{
		assert(i->second < passes.size());
		passes[i->second].setDefaultUniform(uniform);
	}
}

void Renderer::removePassUniform(StringId passName, StringId uniformName)
{
	NameIdMap::const_iterator i = passIndexMap.find(passName);
	if (i != passIndexMap.end())
	{
		assert(i->second < passes.size());
		passes[i->second].removeDefaultUniform(uniformName);
	}
}

bool Renderer::getPassUniform(StringId passName, StringId uniformName, RenderUniform & out)
{
	NameIdMap::const_iterator i = passIndexMap.find(passName);
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
	NameIdMap::const_iterator i = passIndexMap.find(passName);
	if (i != passIndexMap.end())
	{
		assert(i->second < passes.size());
		passes[i->second].setEnabled(enable);
	}
}

bool Renderer::getPassEnabled(StringId passName) const
{
	NameIdMap::const_iterator i = passIndexMap.find(passName);
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
	NameIdMap::const_iterator i = passIndexMap.find(passName);
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
	NameIdMap::const_iterator i = passIndexMap.find(passName);
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

	for (std::vector <RenderPass>::const_iterator i = passes.begin(); i != passes.end(); i++)
		names.push_back(i->getNameId());

	return names;
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
		if (!i->second.defines.empty())
		{
			out << "(";
			for (std::set <std::string>::const_iterator d = i->second.defines.begin(); d != i->second.defines.end(); d++)
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
		for (NameTexMap::const_iterator i = sharedTextures.begin(); i != sharedTextures.end(); i++)
			out << passPrefix << stringMap.getString(i->first) << ", handle " << i->second.handle << std::endl;
	}

	out << "Passes: " << passes.size() << std::endl;
	int passcount = 0;
	for (std::vector <RenderPass>::const_iterator i = passes.begin(); i != passes.end(); i++,passcount++)
	{
		out << "Pass \"" << i->getName() << "\"" << std::endl;

		out << passPrefix << "Draw groups: ";
		int dgcount = 0;
		for (NameIdVecMap::const_iterator dg = drawGroupToPasses.begin(); dg != drawGroupToPasses.end(); dg++)
			if (std::find(dg->second.begin(), dg->second.end(), passcount) != dg->second.end())
			{
				if (dgcount != 0)
					out << ", ";
				out << stringMap.getString(dg->first);
				dgcount++;
			}
		out << std::endl;

		i->printRendererStatus(verbosity, stringMap, out);
	}
}

void Renderer::printProfilingInfo(std::ostream & out) const
{
	for (std::vector <RenderPass>::const_iterator i = passes.begin(); i != passes.end(); i++)
		out << i->getName() << ": " << i->getLastTime()*1e6 << " us" << std::endl;
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
	for (std::set <std::string>::const_iterator i = defines.begin(); i != defines.end(); i++)
		if (!i->empty())
			blockstream << "#define " << *i << std::endl;

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
