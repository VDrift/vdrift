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

#include "drawable.h"
#include "texture.h"
#include "model.h"
#include <cmath>

Drawable::Drawable() :
	list_id(0),
	vert_array(NULL),
	linesize(0),
	objcenter(0),
	radius(0),
	color(1),
	draw_order(0),
	decal(false),
	drawenabled(true),
	cull(false),
	cull_front(false),
	texturesChanged(true),
	uniformsChanged(true)
{
	// ctor
}

void Drawable::SetDiffuseMap(const std::tr1::shared_ptr<Texture> & value)
{
	diffuse_map = value;
	texturesChanged = true;
}

void Drawable::SetMiscMap1(const std::tr1::shared_ptr<Texture> & value)
{
	misc_map1 = value;
	texturesChanged = true;
}

void Drawable::SetMiscMap2(const std::tr1::shared_ptr<Texture> & value)
{
	misc_map2 = value;
	texturesChanged = true;
}

void Drawable::SetVertArray(const VertexArray* value)
{
	vert_array = value;
	renderModel.SetVertArray(vert_array);
}

void Drawable::SetVertexArrayObject(GLuint vao, unsigned int elementCount)
{
	renderModel.setVertexArrayObject(vao, elementCount);
}

void Drawable::SetLineSize(float size)
{
	linesize = size;
	renderModel.SetLineSize(size);
}

void Drawable::SetTransform(const Matrix4 <float> & value)
{
	transform = value;
	uniformsChanged = true;
}

void Drawable::SetObjectCenter(const Vec3 & value)
{
	objcenter = value;
}

void Drawable::SetRadius(float value)
{
	radius = value;
}

void Drawable::SetColor(float r, float g, float b, float a)
{
	color[0] = r;
	color[1] = g;
	color[2] = b;
	color[3] = a;
	uniformsChanged = true;
}

void Drawable::SetColor(float r, float g, float b)
{
	color[0] = r;
	color[1] = g;
	color[2] = b;
	uniformsChanged = true;
}

void Drawable::SetAlpha(float a)
{
	color[3] = a;
	uniformsChanged = true;
}

void Drawable::SetDrawOrder(float value)
{
	draw_order = value;
}

void Drawable::SetDecal(bool value)
{
	decal = value;
	uniformsChanged = true;
}

void Drawable::SetCull(bool newcull, bool newcullfront)
{
	cull = newcull;
	cull_front = newcullfront;
}

RenderModelExt & Drawable::GenRenderModelData(StringIdMap & stringMap)
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

		// only add it if it's not the identity matrix
		if (transform != Matrix4<float>())
			renderModel.uniforms.push_back(RenderUniformEntry(transformId, transform.GetArray(), 16));

		// only add it if it's not the default
		if (color != Vec4(1))
		{
			float srgba[4];
			srgba[0] = color[0] < 1 ? pow(color[0], 2.2f) : color[0];
			srgba[1] = color[1] < 1 ? pow(color[1], 2.2f) : color[1];
			srgba[2] = color[2] < 1 ? pow(color[2], 2.2f) : color[2];
			srgba[3] = color[3];
			renderModel.uniforms.push_back(RenderUniformEntry(colorId, srgba, 4));
		}

		uniformsChanged = false;
	}

	return renderModel;
}

void Drawable::SetModel(const Model & model)
{
	if (model.HaveListID())
	{
		list_id = model.GetListID();
	}

	if (model.HaveVertexArrayObject())
	{
		GLuint vao;
		unsigned int elementCount;
		bool haveVao = model.GetVertexArrayObject(vao, elementCount);
		if (haveVao)
			SetVertexArrayObject(vao, elementCount);
	}
}
