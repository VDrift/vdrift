#include "drawable.h"

#include "texture.h"

#include <cmath>

void DRAWABLE::SetDiffuseMap(std::tr1::shared_ptr<TEXTURE> value)
{
	diffuse_map = value;
	texturesChanged = true;
}

void DRAWABLE::SetMiscMap1(std::tr1::shared_ptr<TEXTURE> value)
{
	misc_map1 = value;
	texturesChanged = true;
}

void DRAWABLE::SetMiscMap2(std::tr1::shared_ptr<TEXTURE> value)
{
	misc_map2 = value;
	texturesChanged = true;
}

void DRAWABLE::SetVertArray(const VERTEXARRAY* value)
{
	vert_array = value;
	renderModel.SetVertArray(vert_array);
}

void DRAWABLE::setVertexArrayObject(GLuint vao, unsigned int elementCount)
{
	renderModel.setVertexArrayObject(vao, elementCount);
}

void DRAWABLE::SetTransform(const MATRIX4 <float> & value)
{
	transform = value;
	uniformsChanged = true;
}

void DRAWABLE::SetColor(float nr, float ng, float nb, float na)
{
	r = nr;
	g = ng;
	b = nb;
	a = na;
	uniformsChanged = true;
}
void DRAWABLE::SetColor(float nr, float ng, float nb)
{
	r = nr;
	g = ng;
	b = nb;
	uniformsChanged = true;
}
void DRAWABLE::SetAlpha(float na)
{
	a = na;
	uniformsChanged = true;
}

RenderModelExternal & DRAWABLE::generateRenderModelData(GLWrapper & gl, StringIdMap & stringMap)
{
	// copy data over to the GL3V renderModel object
	// eventually this should only be done when we update the values, but for now
	// we call this every time we draw the drawable

	// cache off the stringId values
	static StringId diffuseId = stringMap.addStringId("diffuseTexture");
	static StringId misc1Id = stringMap.addStringId("misc1Texture");
	static StringId misc2Id = stringMap.addStringId("normalMapTexture");
	static StringId transformId = stringMap.addStringId("modelMatrix");
	static StringId colorId = stringMap.addStringId("colorTint");
	static StringId depthOffsetId = stringMap.addStringId("depthOffset");

	// textures
	if (texturesChanged)
	{
		renderModel.clearTextureCache();
		renderModel.textures.clear();
		if (diffuse_map && !diffuse_map->IsCube()) // for right now, restrict the diffuse map to 2d textures
		{
			renderModel.textures.push_back(RenderTextureEntry(diffuseId, diffuse_map->GetID(), GL_TEXTURE_2D));
		}
		if (misc_map1 && !misc_map1->IsCube())
		{
			renderModel.textures.push_back(RenderTextureEntry(misc1Id, misc_map1->GetID(), GL_TEXTURE_2D));
		}
		if (misc_map2 && !misc_map2->IsCube())
		{
			renderModel.textures.push_back(RenderTextureEntry(misc2Id, misc_map2->GetID(), GL_TEXTURE_2D));
		}

		texturesChanged = false;
	}

	// uniforms
	if (uniformsChanged)
	{
		renderModel.clearUniformCache();
		renderModel.uniforms.clear();
		if (transform != MATRIX4<float>()) // only add it if it's not the identity matrix
			renderModel.uniforms.push_back(RenderUniformEntry(transformId, transform.GetArray(), 16));
		if (r != 1 || g != 1 || b != 1 || a != 1) // only add it if it's not the default
		{
			float srgb_alpha[4];
			srgb_alpha[0] = r < 1 ? pow(r, 2.2f) : r;
			srgb_alpha[1] = g < 1 ? pow(g, 2.2f) : g;
			srgb_alpha[2] = b < 1 ? pow(b, 2.2f) : b;
			srgb_alpha[3] = a;
			renderModel.uniforms.push_back(RenderUniformEntry(colorId, srgb_alpha, 4));
		}
		/*if (decal) // only add it if it's not the default of zero
		{
			float depthOffset = -0.005;
			renderModel.uniforms.push_back(RenderUniformEntry(depthOffsetId, &depthOffset, 1));
		}*/

		uniformsChanged = false;
	}

	return renderModel;
}

void DRAWABLE::SetModel(const MODEL & model)
{
	if (model.HaveListID())
	{
		AddDrawList(model.GetListID());
	}

	if (model.HaveVertexArrayObject())
	{
		GLuint vao;
		unsigned int elementCount;
		bool haveVao = model.GetVertexArrayObject(vao, elementCount);
		if (haveVao)
			setVertexArrayObject(vao, elementCount);
	}
}
