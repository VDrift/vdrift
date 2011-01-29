#include "drawable.h"

#include "texture.h"

void DRAWABLE::SetDiffuseMap(std::tr1::shared_ptr<TEXTURE> value)
{
	diffuse_map = value;
}

void DRAWABLE::SetMiscMap1(std::tr1::shared_ptr<TEXTURE> value)
{
	misc_map1 = value;
}

void DRAWABLE::SetMiscMap2(std::tr1::shared_ptr<TEXTURE> value)
{
	misc_map2 = value;
}

void DRAWABLE::SetVertArray(const VERTEXARRAY* value)
{
	vert_array = value;
}

RenderModelExternal & DRAWABLE::generateRenderModelData(GLWrapper & gl, StringIdMap & stringMap)
{
	// copy data over to the GL3V renderModel object
	// eventually this should only be done when we update the values, but for now
	// we call this every time we draw the drawable
	
	// cache off the stringId values
	static StringId diffuseId = stringMap.addStringId("diffuseTexture");
	static StringId transformId = stringMap.addStringId("modelMatrix");
	static StringId colorId = stringMap.addStringId("colorTint");
	
	if (vert_array)
	{
		// geometry
		renderModel.SetVertArray(vert_array);
	}
	else
	{
		renderModel.SetVertArray(NULL);
	}
	
	// textures
	renderModel.textures.clear();
	if (diffuse_map && !diffuse_map->IsCube()) // for right now, restrict the diffuse map to 2d textures
	{
		renderModel.textures.push_back(RenderTextureEntry(diffuseId, diffuse_map->GetID(), GL_TEXTURE_2D));
	}
	
	// uniforms
	renderModel.uniforms.clear();
	renderModel.uniforms.push_back(RenderUniformEntry(transformId, transform.GetArray(), 16));
	renderModel.uniforms.push_back(RenderUniformEntry(colorId, &r, 4));
	
	return renderModel;
}
